// Copyright (c) 2015-2017 LG Electronics, Inc.

#include "media/renderers/neva/neva_media_player_renderer_factory.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "media/base/decoder_factory.h"
#include "media/base/media_log.h"
#include "media/base/neva/media_platform_api.h"
#include "media/filters/gpu_video_decoder.h"
#include "media/filters/neva/webos/audio_decoder_webos.h"
#include "media/filters/neva/webos/video_decoder_webos.h"
#include "media/renderers/audio_renderer_impl.h"
#include "media/renderers/neva/webos/video_renderer_webos.h"
#include "media/renderers/renderer_impl.h"
#include "media/renderers/video_renderer_impl.h"
#include "media/video/gpu_video_accelerator_factories.h"

#if !defined(MEDIA_DISABLE_FFMPEG)
#include "media/filters/ffmpeg_audio_decoder.h"
#if !defined(DISABLE_FFMPEG_VIDEO_DECODERS)
#include "media/filters/ffmpeg_video_decoder.h"
#endif
#endif

#if !defined(MEDIA_DISABLE_LIBVPX)
#include "media/filters/vpx_video_decoder.h"
#endif

namespace media {

NevaMediaPlayerRendererFactory::NevaMediaPlayerRendererFactory(
    MediaLog* media_log,
    DecoderFactory* decoder_factory,
    const GetGpuFactoriesCB& get_gpu_factories_cb)
    : media_log_(media_log),
      decoder_factory_(decoder_factory),
      get_gpu_factories_cb_(get_gpu_factories_cb) {}

NevaMediaPlayerRendererFactory::~NevaMediaPlayerRendererFactory() {
}

// static
bool NevaMediaPlayerRendererFactory::Enabled() {
  return true;
}

std::vector<std::unique_ptr<AudioDecoder>>
NevaMediaPlayerRendererFactory::CreateAudioDecoders(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner) {
  // Create our audio decoders and renderer.
  std::vector<std::unique_ptr<AudioDecoder>> audio_decoders;

  audio_decoders.push_back(std::make_unique<AudioDecoderWebOS>(
      media_task_runner, media_platform_api_));

  decoder_factory_->CreateAudioDecoders(media_task_runner, media_log_,
                                        &audio_decoders);

  return audio_decoders;
}

std::vector<std::unique_ptr<VideoDecoder>>
NevaMediaPlayerRendererFactory::CreateVideoDecoders(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const RequestOverlayInfoCB& request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space,
    GpuVideoAcceleratorFactories* gpu_factories) {
  // Create our video decoders and renderer.
  std::vector<std::unique_ptr<VideoDecoder>> video_decoders;

  // Prefer an external decoder since one will only exist if it is hardware
  // accelerated.
  video_decoders.push_back(std::make_unique<VideoDecoderWebOS>(
      media_task_runner, media_platform_api_));

  decoder_factory_->CreateVideoDecoders(media_task_runner, gpu_factories,
                                        media_log_, request_overlay_info_cb,
                                        target_color_space, &video_decoders);

  return video_decoders;
}

std::unique_ptr<Renderer> NevaMediaPlayerRendererFactory::CreateRenderer(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const scoped_refptr<base::TaskRunner>& worker_task_runner,
    AudioRendererSink* audio_renderer_sink,
    VideoRendererSink* video_renderer_sink,
    const RequestOverlayInfoCB& request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space) {
  DCHECK(audio_renderer_sink);
  DCHECK(media_platform_api_) << "WebOS Media API uninitialized";

  std::unique_ptr<AudioRenderer> audio_renderer(new AudioRendererImpl(
      media_task_runner, audio_renderer_sink,
      // TODO(neva): Ensure following comment. From DefaultRendererFactory.
      // Unretained is safe here, because the RendererFactory is guaranteed to
      // outlive the RendererImpl. The RendererImpl is destroyed when WMPI
      // destructor calls pipeline_controller_.Stop() -> PipelineImpl::Stop() ->
      // RendererWrapper::Stop -> RendererWrapper::DestroyRenderer(). And the
      // RendererFactory is owned by WMPI and gets called after WMPI destructor
      // finishes.
      base::Bind(&NevaMediaPlayerRendererFactory::CreateAudioDecoders,
                 base::Unretained(this), media_task_runner),
      media_log_));

  GpuVideoAcceleratorFactories* gpu_factories = nullptr;
  if (!get_gpu_factories_cb_.is_null())
    gpu_factories = get_gpu_factories_cb_.Run();

  std::unique_ptr<GpuMemoryBufferVideoFramePool> gmb_pool;
  if (gpu_factories && gpu_factories->ShouldUseGpuMemoryBuffersForVideoFrames(
                           false /* for_media_stream */)) {
    gmb_pool = std::make_unique<GpuMemoryBufferVideoFramePool>(
        std::move(media_task_runner), std::move(worker_task_runner),
        gpu_factories);
  }

  std::unique_ptr<VideoRenderer> video_renderer(new VideoRendererWebOS(
      media_task_runner, video_renderer_sink, media_platform_api_,
      // Unretained is safe here, because the RendererFactory is guaranteed to
      // outlive the RendererImpl. The RendererImpl is destroyed when WMPI
      // destructor calls pipeline_controller_.Stop() -> PipelineImpl::Stop() ->
      // RendererWrapper::Stop -> RendererWrapper::DestroyRenderer(). And the
      // RendererFactory is owned by WMPI and gets called after WMPI destructor
      // finishes.
      base::Bind(&NevaMediaPlayerRendererFactory::CreateVideoDecoders,
                 base::Unretained(this), media_task_runner,
                 request_overlay_info_cb, target_color_space, gpu_factories),
      true, media_log_,  std::move(gmb_pool)));

  // Create renderer.
  std::unique_ptr<RendererImpl> renderer(new RendererImpl(
      media_task_runner, std::move(audio_renderer), std::move(video_renderer)));

  renderer->SetMediaPlatformAPI(media_platform_api_);

  return std::move(renderer);
}

void NevaMediaPlayerRendererFactory::SetMediaPlatformAPI(
    const scoped_refptr<MediaPlatformAPI>& media_platform_api) {
  media_platform_api_ = media_platform_api;
}

}  // namespace media
