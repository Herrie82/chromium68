// Copyright (c) 2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MEDIA_RENDERERS_NEVA_WEBOS_VIDEO_RENDERER_WEBOS_H_
#define MEDIA_RENDERERS_NEVA_WEBOS_VIDEO_RENDERER_WEBOS_H_

#include <stddef.h>
#include <stdint.h>

#include <deque>
#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/timer/timer.h"
#include "media/base/decryptor.h"
#include "media/base/demuxer_stream.h"
#include "media/base/media_log.h"
#include "media/base/neva/media_platform_api.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_decoder.h"
#include "media/base/video_frame.h"
#include "media/base/video_renderer.h"
#include "media/base/video_renderer_sink.h"
#include "media/filters/decoder_stream.h"
#include "media/filters/video_renderer_algorithm.h"
#include "media/renderers/neva/neva_media_player_renderer_factory.h"
#include "media/video/gpu_memory_buffer_video_frame_pool.h"
#include "media/video/gpu_video_accelerator_factories.h"

namespace base {
class SingleThreadTaskRunner;
class TickClock;
}

namespace media {

// VideoRendererWebOS handles reading from a VideoFrameStream storing the
// results in a queue of decoded frames and executing a callback when a frame is
// ready for rendering.
class MEDIA_EXPORT VideoRendererWebOS
    : public VideoRenderer,
      public VideoRendererSink::RenderCallback {
 public:
  // |decoders| contains the VideoDecoders to use when initializing.
  //
  // Implementors should avoid doing any sort of heavy work in this method and
  // instead post a task to a common/worker thread to handle rendering.  Slowing
  // down the video thread may result in losing synchronization with audio.
  //
  // Setting |drop_frames_| to true causes the renderer to drop expired frames.
  VideoRendererWebOS(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      VideoRendererSink* sink,
      const scoped_refptr<MediaPlatformAPI>& media_platform_api,
      const CreateVideoDecodersCB& create_video_decoders_cb,
      bool drop_frames,
      MediaLog* media_log,
      std::unique_ptr<GpuMemoryBufferVideoFramePool> gmb_pool);
  ~VideoRendererWebOS() override;

  // VideoRenderer implementation.
  void Initialize(DemuxerStream* stream,
                  CdmContext* cdm_context,
                  RendererClient* client,
                  const TimeSource::WallClockTimeCB& wall_clock_time_cb,
                  const PipelineStatusCB& init_cb) override;
  void Flush(const base::Closure& callback) override;
  void StartPlayingFrom(base::TimeDelta timestamp) override;

  // TODO(neva): Implementation was changed on 53 -> 59.
  // OnTimeStateChanged -> OnTimeProgressing, OnTimeStopped
  // Just follow VideoRendererImpl for now.
  void OnTimeProgressing() override;
  void OnTimeStopped() override;

  void SetTickClockForTesting(std::unique_ptr<base::TickClock> tick_clock);
  void SetGpuMemoryBufferVideoForTesting(
      std::unique_ptr<GpuMemoryBufferVideoFramePool> gpu_memory_buffer_pool);
  size_t frames_queued_for_testing() const {
    return algorithm_->frames_queued();
  }
  size_t effective_frames_queued_for_testing() const {
    return algorithm_->effective_frames_queued();
  }

  // VideoRendererSink::RenderCallback implementation.
  scoped_refptr<VideoFrame> Render(base::TimeTicks deadline_min,
                                   base::TimeTicks deadline_max,
                                   bool background_rendering) override;
  void OnFrameDropped() override;

 private:
  // Callback for |video_frame_stream_| initialization.
  void OnVideoFrameStreamInitialized(bool success);

  // Functions to notify certain events to the RendererClient.
  void OnPlaybackError(PipelineStatus error);
  void OnPlaybackEnded();
  void OnStatisticsUpdate(const PipelineStatistics& stats);
  void OnBufferingStateChange(BufferingState state);
  void OnWaitingForDecryptionKey();

  void OnLoaded_Locked();

  // Called by the VideoFrameStream when a config change occurs. Will notify
  // RenderClient of the new config.
  void OnConfigChange(const VideoDecoderConfig& config);

  // Callback for |video_frame_stream_| to deliver decoded video frames and
  // report video decoding status.
  void FrameReady(VideoFrameStream::Status status,
                  const scoped_refptr<VideoFrame>& frame);

  // Helper method for enqueueing a frame to |alogorithm_|.
  void AddReadyFrame_Locked(const scoped_refptr<VideoFrame>& frame);

  // Helper method that schedules an asynchronous read from the
  // |video_frame_stream_| as long as there isn't a pending read and we have
  // capacity.
  void AttemptRead_Locked();

  // Called when VideoFrameStream::Reset() completes.
  void OnVideoFrameStreamResetDone();

  // Returns true if the renderer has enough data for playback purposes.
  // Note that having enough data may be due to reaching end of stream.
  bool HaveEnoughData_Locked();
  void TransitionToHaveEnough_Locked();
  void TransitionToHaveNothing();
  void TransitionToHaveNothing_Locked();

  // Runs |statistics_cb_| with |frames_decoded_| and |frames_dropped_|, resets
  // them to 0.
  void UpdateStats_Locked();

  // Returns true if there is no more room for additional buffered frames.
  bool HaveReachedBufferingCap();

  // Starts or stops |sink_| respectively. Do not call while |lock_| is held.
  void StartSink();
  void StopSink();

  // Fires |ended_cb_| if there are no remaining usable frames and
  // |received_end_of_stream_| is true.  Sets |rendered_end_of_stream_| if it
  // does so.
  //
  // When called from the media thread, |time_progressing| should reflect the
  // value of |time_progressing_|.  When called from Render() on the sink
  // callback thread, |time_progressing| must be true since Render() could not
  // have been called otherwise.
  void MaybeFireEndedCallback_Locked(bool time_progressing);

  // Helper method for converting a single media timestamp to wall clock time.
  base::TimeTicks ConvertMediaTimestamp(base::TimeDelta media_timestamp);

  base::TimeTicks GetCurrentMediaTimeAsWallClockTime();

  // Helper method for checking if a frame timestamp plus the frame's expected
  // duration is before |start_timestamp_|.
  bool IsBeforeStartTime(base::TimeDelta timestamp);

  // Attempts to remove frames which are no longer effective for rendering when
  // |buffering_state_| == BUFFERING_HAVE_NOTHING or |was_background_rendering_|
  // is true.  If the current media time as provided by |wall_clock_time_cb_| is
  // null, no frame expiration will be done.
  //
  // When background rendering the method will expire all frames before the
  // current wall clock time since it's expected that there will be long delays
  // between each Render() call in this state.
  //
  // When in the underflow state the method will first attempt to remove expired
  // frames before the current media time plus duration. If |sink_started_| is
  // true, nothing more can be done. However, if false, and there are still no
  // effective frames in the queue, the entire frame queue will be released to
  // avoid any stalling.
  void RemoveFramesForUnderflowOrBackgroundRendering();

  // Notifies |client_| in the event of frame size or opacity changes. Must be
  // called on |task_runner_|.
  void CheckForMetadataChanges(VideoPixelFormat pixel_format,
                               const gfx::Size& natural_size);

  // Both calls AttemptRead_Locked() and CheckForMetadataChanges(). Must be
  // called on |task_runner_|.
  void AttemptReadAndCheckForMetadataChanges(VideoPixelFormat pixel_format,
                                             const gfx::Size& natural_size);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Sink which calls into VideoRendererWebOS via Render() for video frames.  Do
  // not call any methods on the sink while |lock_| is held or the two threads
  // might deadlock. Do not call Start() or Stop() on the sink directly, use
  // StartSink() and StopSink() to ensure background rendering is started.  Only
  // access these values on |task_runner_|.
  VideoRendererSink* const sink_;
  bool sink_started_;

  // Stores the last decoder config that was passed to
  // RendererClient::OnVideoConfigChange. Used to prevent signaling config
  // to the upper layers when when the new config is the same.
  VideoDecoderConfig current_decoder_config_;

  // Used for accessing data members.
  base::Lock lock_;

  RendererClient* client_;

  // Provides video frames to VideoRendererWebOS.
  std::unique_ptr<VideoFrameStream> video_frame_stream_;

  // Pool of GpuMemoryBuffers and resources used to create hardware frames.
  std::unique_ptr<GpuMemoryBufferVideoFramePool> gpu_memory_buffer_pool_;

  MediaLog* media_log_;

  // Flag indicating low-delay mode.
  bool low_delay_;

  // Keeps track of whether we received the end of stream buffer and finished
  // rendering.
  bool received_end_of_stream_;
  bool rendered_end_of_stream_;

  // Important detail: being in kPlaying doesn't imply that video is being
  // rendered. Rather, it means that the renderer is ready to go. The actual
  // rendering of video is controlled by time advancing via |get_time_cb_|.
  // Video renderer can be reinitialized completely by calling Initialize again
  // when it is in a kFlushed state with video sink stopped.
  //
  //    kUninitialized
  //  +------> | Initialize()
  //  |        |
  //  |        V
  //  |   kInitializing
  //  |        | Decoders initialized
  //  |        |
  //  |        V            Decoders reset
  //  ---- kFlushed <------------------ kFlushing
  //           | StartPlayingFrom()         ^
  //           |                            |
  //           |                            | Flush()
  //           `---------> kPlaying --------'
  enum State {
    kUninitialized,
    kInitializing,
    kFlushing,
    kFlushed,
    kPlaying
  };
  State state_;

  // TODO(servolk): Consider using DecoderFactory here instead of the
  // CreateVideoDecodersCB.
  CreateVideoDecodersCB create_video_decoders_cb_;
  scoped_refptr<base::TaskRunner> worker_task_runner_;

  // Keep track of the outstanding read on the VideoFrameStream. Flushing can
  // only complete once the read has completed.
  bool pending_read_;

  bool drop_frames_;

  BufferingState buffering_state_;

  // Playback operation callbacks.
  PipelineStatusCB init_cb_;
  base::Closure flush_cb_;
  TimeSource::WallClockTimeCB wall_clock_time_cb_;

  base::TimeDelta start_timestamp_;

  // Keeps track of the number of frames decoded and dropped since the
  // last call to |statistics_cb_|. These must be accessed under lock.
  int frames_decoded_;
  int frames_dropped_;

  std::unique_ptr<base::TickClock> tick_clock_;

  // Algorithm for selecting which frame to render; manages frames and all
  // timing related information.
  std::unique_ptr<VideoRendererAlgorithm> algorithm_;

  // Indicates that Render() was called with |background_rendering| set to true,
  // so we've entered a background rendering mode where dropped frames are not
  // counted.  Must be accessed under |lock_| once |sink_| is started.
  bool was_background_rendering_;

  // Indicates whether or not media time is currently progressing or not.  Must
  // only be accessed from |task_runner_|.
  bool time_progressing_;

  // Memory usage of |algorithm_| recorded during the last UpdateStats_Locked()
  // call.
  int64_t last_video_memory_usage_;

  // Indicates if a frame has been processed by CheckForMetadataChanges().
  bool have_renderered_frames_;

  // Tracks last frame properties to detect and notify client of any changes.
  gfx::Size last_frame_natural_size_;
  bool last_frame_opaque_;

  // Indicates if we've painted the first valid frame after StartPlayingFrom().
  bool painted_first_frame_;

  // Current maximum for buffered frames, increases up to a limit upon each
  // call to OnTimeStopped() when we're in the BUFFERING_HAVE_NOTHING state.
  size_t max_buffered_frames_;


  scoped_refptr<MediaPlatformAPI> media_platform_api_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<VideoRendererWebOS> weak_factory_;

  // Weak factory used to invalidate certain queued callbacks on reset().
  // This is useful when doing video frame copies asynchronously since we
  // want to discard video frames that might be received after the stream has
  // been reset.
  base::WeakPtrFactory<VideoRendererWebOS> frame_callback_weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoRendererWebOS);
};

}  // namespace media

#endif  // MEDIA_RENDERERS_NEVA_WEBOS_VIDEO_RENDERER_WEBOS_H_
