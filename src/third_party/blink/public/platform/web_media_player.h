/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_PLAYER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_PLAYER_H_

#include "third_party/blink/public/platform/web_callbacks.h"
#include "third_party/blink/public/platform/web_canvas.h"
#include "third_party/blink/public/platform/web_content_decryption_module.h"
#include "third_party/blink/public/platform/web_media_source.h"
#include "third_party/blink/public/platform/web_set_sink_id_callbacks.h"
#include "third_party/blink/public/platform/web_string.h"

#include "cc/paint/paint_flags.h"

#if defined(USE_NEVA_MEDIA)
#include "third_party/blink/public/platform/neva/web_media_player.h"
#endif

namespace gpu {
namespace gles2 {
class GLES2Interface;
}
}

namespace blink {

class WebAudioSourceProvider;
class WebContentDecryptionModule;
class WebMediaPlayerSource;
class WebString;
class WebURL;
enum class WebFullscreenVideoStatus;
struct WebRect;
struct WebSize;

#if defined(USE_NEVA_MEDIA)
class WebMediaPlayer : public neva::WebMediaPlayer {
#else
class WebMediaPlayer {
#endif
 public:
  enum NetworkState {
    kNetworkStateEmpty,
    kNetworkStateIdle,
    kNetworkStateLoading,
    kNetworkStateLoaded,
    kNetworkStateFormatError,
    kNetworkStateNetworkError,
    kNetworkStateDecodeError,
  };

  enum ReadyState {
    kReadyStateHaveNothing,
    kReadyStateHaveMetadata,
    kReadyStateHaveCurrentData,
    kReadyStateHaveFutureData,
    kReadyStateHaveEnoughData,
  };

  enum Preload {
    kPreloadNone,
    kPreloadMetaData,
    kPreloadAuto,
  };

  enum CORSMode {
    kCORSModeUnspecified,
    kCORSModeAnonymous,
    kCORSModeUseCredentials,
  };

  // Reported to UMA. Do not change existing values.
  enum LoadType {
    kLoadTypeURL = 0,
    kLoadTypeMediaSource = 1,
    kLoadTypeMediaStream = 2,
    kLoadTypeMax = kLoadTypeMediaStream,
  };

  typedef WebString TrackId;
  enum TrackType { kTextTrack, kAudioTrack, kVideoTrack };

  // This must stay in sync with WebGLRenderingContextBase::TexImageFunctionID.
  enum TexImageFunctionID {
    kTexImage2D,
    kTexSubImage2D,
    kTexImage3D,
    kTexSubImage3D
  };

  // For last-uploaded-frame-metadata API. https://crbug.com/639174
  struct VideoFrameUploadMetadata {
    int frame_id = -1;
    gfx::Rect visible_rect = {};
    base::TimeDelta timestamp = {};
    bool skipped = false;
  };

  // Callback to get notified when the Picture-in-Picture window is opened.
  using PipWindowOpenedCallback = base::OnceCallback<void(const WebSize&)>;
  // Callback to get notified when Picture-in-Picture window is closed.
  using PipWindowClosedCallback = base::OnceClosure;
  // Callback to get notified when the Picture-in-Picture window is resized.
  using PipWindowResizedCallback =
      base::RepeatingCallback<void(const WebSize&)>;

  virtual ~WebMediaPlayer() = default;

  virtual void Load(LoadType, const WebMediaPlayerSource&, CORSMode) = 0;

  // Playback controls.
  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void Seek(double seconds) = 0;
  virtual void SetRate(double) = 0;
  virtual void SetVolume(double) = 0;

  // Enter Picture-in-Picture and notifies Blink with window size
  // when video successfully enters Picture-in-Picture.
  virtual void EnterPictureInPicture(PipWindowOpenedCallback) = 0;
  // Exit Picture-in-Picture and notifies Blink when it's done.
  virtual void ExitPictureInPicture(PipWindowClosedCallback) = 0;
  // Register a callback that will be run when the Picture-in-Picture window
  // is resized.
  virtual void RegisterPictureInPictureWindowResizeCallback(
      PipWindowResizedCallback) = 0;

  virtual void RequestRemotePlayback() {}
  virtual void RequestRemotePlaybackControl() {}
  virtual void RequestRemotePlaybackStop() {}
  virtual void RequestRemotePlaybackDisabled(bool disabled) {}
  virtual void FlingingStarted() {}
  virtual void FlingingStopped() {}
  virtual void SetPreload(Preload) {}
  virtual WebTimeRanges Buffered() const = 0;
  virtual WebTimeRanges Seekable() const = 0;

  // Attempts to switch the audio output device.
  // Implementations of SetSinkId take ownership of the WebSetSinkCallbacks
  // object.
  // Note also that SetSinkId implementations must make sure that all
  // methods of the WebSetSinkCallbacks object, including constructors and
  // destructors, run in the same thread where the object is created
  // (i.e., the blink thread).
  virtual void SetSinkId(const WebString& sink_id,
                         WebSetSinkIdCallbacks*) = 0;

  // True if the loaded media has a playable video/audio track.
  virtual bool HasVideo() const = 0;
  virtual bool HasAudio() const = 0;

  // True if the media is being played on a remote device.
  virtual bool IsRemote() const { return false; }

  // Dimension of the video.
  virtual WebSize NaturalSize() const = 0;

  virtual WebSize VisibleRect() const = 0;

  // Getters of playback state.
  virtual bool Paused() const = 0;
  virtual bool Seeking() const = 0;
  virtual double Duration() const = 0;
  virtual double CurrentTime() const = 0;

  // Internal states of loading and network.
  virtual NetworkState GetNetworkState() const = 0;
  virtual ReadyState GetReadyState() const = 0;

  // Returns an implementation-specific human readable error message, or an
  // empty string if no message is available. The message should begin with a
  // UA-specific-error-code (without any ':'), optionally followed by ': ' and
  // further description of the error.
  virtual WebString GetErrorMessage() const = 0;

  virtual bool DidLoadingProgress() = 0;

  virtual bool DidGetOpaqueResponseFromServiceWorker() const = 0;
  virtual bool HasSingleSecurityOrigin() const = 0;
  virtual bool DidPassCORSAccessCheck() const = 0;

  virtual double MediaTimeForTimeValue(double time_value) const = 0;

  virtual unsigned DecodedFrameCount() const = 0;
  virtual unsigned DroppedFrameCount() const = 0;
  virtual unsigned CorruptedFrameCount() const { return 0; }
  virtual size_t AudioDecodedByteCount() const = 0;
  virtual size_t VideoDecodedByteCount() const = 0;

  // |out_metadata|, if set, is used to return metadata about the frame
  //   that is uploaded during this call.
  // |already_uploaded_id| indicates the unique_id of the frame last uploaded
  //   to this destination. It should only be set by the caller if the contents
  //   of the destination are known not to have changed since that upload.
  //   - If |out_metadata| is not null, |already_uploaded_id| is compared with
  //     the unique_id of the frame being uploaded. If it's the same, the
  //     upload may be skipped and considered to be successful.
  virtual void Paint(WebCanvas*,
                     const WebRect&,
                     cc::PaintFlags&,
                     int already_uploaded_id = -1,
                     VideoFrameUploadMetadata* out_metadata = nullptr) = 0;

  // Do a GPU-GPU texture copy of the current video frame to |texture|,
  // reallocating |texture| at the appropriate size with given internal
  // format, format, and type if necessary. If the copy is impossible
  // or fails, it returns false.
  //
  // |out_metadata|, if set, is used to return metadata about the frame
  //   that is uploaded during this call.
  // |already_uploaded_id| indicates the unique_id of the frame last uploaded
  //   to this destination. It should only be set by the caller if the contents
  //   of the destination are known not to have changed since that upload.
  //   - If |out_metadata| is not null, |already_uploaded_id| is compared with
  //     the unique_id of the frame being uploaded. If it's the same, the
  //     upload may be skipped and considered to be successful.
  virtual bool CopyVideoTextureToPlatformTexture(
      gpu::gles2::GLES2Interface*,
      unsigned target,
      unsigned texture,
      unsigned internal_format,
      unsigned format,
      unsigned type,
      int level,
      bool premultiply_alpha,
      bool flip_y,
      int already_uploaded_id = -1,
      VideoFrameUploadMetadata* out_metadata = nullptr) {
    return false;
  }

  // Copy sub video frame texture to |texture|. If the copy is impossible or
  // fails, it returns false.
  virtual bool CopyVideoSubTextureToPlatformTexture(gpu::gles2::GLES2Interface*,
                                                    unsigned target,
                                                    unsigned texture,
                                                    int level,
                                                    int xoffset,
                                                    int yoffset,
                                                    bool premultiply_alpha,
                                                    bool flip_y) {
    return false;
  }

  // Do Tex(Sub)Image2D/3D for current frame. If it is not implemented for given
  // parameters or fails, it returns false.
  // The method is wrapping calls to glTexImage2D, glTexSubImage2D,
  // glTexImage3D and glTexSubImage3D and parameters have the same name and
  // meaning.
  // Texture |texture| needs to be created and bound to active texture unit
  // before this call. In addition, TexSubImage2D and TexSubImage3D require that
  // previous TexImage2D and TexSubImage3D calls, respectively, defined the
  // texture content.
  virtual bool TexImageImpl(TexImageFunctionID function_id,
                            unsigned target,
                            gpu::gles2::GLES2Interface* gl,
                            unsigned texture,
                            int level,
                            int internalformat,
                            unsigned format,
                            unsigned type,
                            int xoffset,
                            int yoffset,
                            int zoffset,
                            bool flip_y,
                            bool premultiply_alpha) {
    return false;
  }

  virtual WebAudioSourceProvider* GetAudioSourceProvider() { return nullptr; }

  virtual void SetContentDecryptionModule(
      WebContentDecryptionModule* cdm,
      WebContentDecryptionModuleResult result) {
    result.CompleteWithError(
        kWebContentDecryptionModuleExceptionNotSupportedError, 0, "ERROR");
  }

  // Sets the poster image URL.
  virtual void SetPoster(const WebURL& poster) {}

  // Whether the WebMediaPlayer supports overlay fullscreen video mode. When
  // this is true, the video layer will be removed from the layer tree when
  // entering fullscreen, and the WebMediaPlayer is responsible for displaying
  // the video in enteredFullscreen().
  virtual bool SupportsOverlayFullscreenVideo() { return false; }
  // Inform WebMediaPlayer when the element has entered/exited fullscreen.
  virtual void EnteredFullscreen() {}
  virtual void ExitedFullscreen() {}

  // Inform WebMediaPlayer when the element starts/stops being the dominant
  // visible content. This will only be called after the monitoring of the
  // intersection with viewport is activated by calling
  // WebMediaPlayerClient::ActivateViewportIntersectionMonitoring().
  virtual void BecameDominantVisibleContent(bool is_dominant) {}

  // Inform WebMediaPlayer when the element starts/stops being the effectively
  // fullscreen video, i.e. being the fullscreen element or child of the
  // fullscreen element, and being dominant in the viewport.
  //
  // TODO(zqzhang): merge with BecameDominantVisibleContent(). See
  // https://crbug.com/696211
  virtual void SetIsEffectivelyFullscreen(WebFullscreenVideoStatus) {}

  virtual void EnabledAudioTracksChanged(
      const WebVector<TrackId>& enabled_track_ids) {}
  // |selected_track_id| is null if no track is selected.
  virtual void SelectedVideoTrackChanged(TrackId* selected_track_id) {}

  // Callback called whenever the media element may have received or last native
  // controls. It might be called twice with the same value: the caller has to
  // check if the value have changed if it only wants to handle this case.
  // This method is not used to say express if the native controls are visible
  // but if the element is using them.
  virtual void OnHasNativeControlsChanged(bool) {}

  enum class DisplayType {
    kInline,
    kFullscreen,
    kPictureInPicture,
  };

  // Callback called whenever the media element display type changes. By
  // default, the display type is `kInline`.
  virtual void OnDisplayTypeChanged(DisplayType) {}

  // Test helper methods for exercising media suspension.
  virtual void ForceStaleStateForTesting(ReadyState target_state) {}
  virtual bool IsSuspendedForTesting() { return false; }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_PLAYER_H_
