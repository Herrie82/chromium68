// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_AUDIO_INPUT_STREAM_H_
#define MEDIA_MOJO_SERVICES_MOJO_AUDIO_INPUT_STREAM_H_

#include <memory>
#include <string>

#include "base/sequence_checker.h"
#include "media/audio/audio_input_delegate.h"
#include "media/mojo/interfaces/audio_data_pipe.mojom.h"
#include "media/mojo/interfaces/audio_input_stream.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

// This class handles IPC for single audio input stream by delegating method
// calls to its AudioInputDelegate.
class MEDIA_MOJO_EXPORT MojoAudioInputStream
    : public mojom::AudioInputStream,
      public AudioInputDelegate::EventHandler {
 public:
  using StreamCreatedCallback =
      base::OnceCallback<void(mojom::AudioDataPipePtr, bool)>;
  using CreateDelegateCallback =
      base::OnceCallback<std::unique_ptr<AudioInputDelegate>(
          AudioInputDelegate::EventHandler*)>;

  // |create_delegate_callback| is used to obtain an AudioInputDelegate for the
  // stream in the constructor. |stream_created_callback| is called when the
  // stream has been initialized. |deleter_callback| is called when this class
  // should be removed (stream ended/error). |deleter_callback| is required to
  // destroy |this| synchronously.
  MojoAudioInputStream(mojom::AudioInputStreamRequest request,
                       mojom::AudioInputStreamClientPtr client,
                       CreateDelegateCallback create_delegate_callback,
                       StreamCreatedCallback stream_created_callback,
                       base::OnceClosure deleter_callback);

  ~MojoAudioInputStream() override;

  void SetOutputDeviceForAec(const std::string& raw_output_device_id);

 private:
  // mojom::AudioInputStream implementation.
  void Record() override;
  void Pause() override;
  void Resume() override;
  void SetVolume(double volume) override;

  // AudioInputDelegate::EventHandler implementation.
  void OnStreamCreated(
      int stream_id,
      base::ReadOnlySharedMemoryRegion shared_memory_region,
      std::unique_ptr<base::CancelableSyncSocket> foreign_socket,
      bool initially_muted) override;
  void OnMuted(int stream_id, bool is_muted) override;
  void OnStreamError(int stream_id) override;

  // Closes connection to client and notifies owner.
  void OnError();

  SEQUENCE_CHECKER(sequence_checker_);

  StreamCreatedCallback stream_created_callback_;
  base::OnceClosure deleter_callback_;
  mojo::Binding<AudioInputStream> binding_;
  mojom::AudioInputStreamClientPtr client_;
  std::unique_ptr<AudioInputDelegate> delegate_;
  base::WeakPtrFactory<MojoAudioInputStream> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoAudioInputStream);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_AUDIO_INPUT_STREAM_H_
