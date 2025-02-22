// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_AUDIO_INPUT_DELEGATE_H_
#define MEDIA_AUDIO_AUDIO_INPUT_DELEGATE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/base/media_export.h"

namespace base {
class CancelableSyncSocket;
class ReadOnlySharedMemoryRegion;
}  // namespace base

namespace media {

class MEDIA_EXPORT AudioInputDelegate {
 public:
  // An AudioInputDelegate must not call back to its EventHandler in its
  // constructor.
  class MEDIA_EXPORT EventHandler {
   public:
    virtual ~EventHandler() = 0;

    // Called when the underlying stream is ready for recording.
    virtual void OnStreamCreated(
        int stream_id,
        base::ReadOnlySharedMemoryRegion shared_memory_region,
        std::unique_ptr<base::CancelableSyncSocket> socket,
        bool initially_muted) = 0;

    // Called when the microphone is muted/unmuted.
    virtual void OnMuted(int stream_id, bool is_muted) = 0;

    // Called if stream encounters an error and has become unusable.
    virtual void OnStreamError(int stream_id) = 0;
  };

  virtual ~AudioInputDelegate() = 0;

  virtual int GetStreamId() = 0;

  // Stream control:
  virtual void OnRecordStream() = 0;
  virtual void OnPauseStream() = 0;
  virtual void OnResumeStream() = 0;
  virtual void OnSetVolume(double volume) = 0;
  virtual void OnSetOutputDeviceForAec(
      const std::string& raw_output_device_id) = 0;
};

}  // namespace media

#endif  // MEDIA_AUDIO_AUDIO_INPUT_DELEGATE_H_
