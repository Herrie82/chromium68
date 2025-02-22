// This file is generated by PAL generator, do not modify.
// To make changes, modify the source file:
// test3.json

#include "remote_pal_ipc/sample/sample_remote_pal_interface_ipc.h"

#include <limits>

#include "base/bind.h"
#include "content/public/renderer/render_frame.h"
#include "pal/ipc/renderer/sample/sample_proxy.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace pal {

SampleRemotePalInterfaceIPC::SampleRemotePalInterfaceIPC() = default;

SampleRemotePalInterfaceIPC::~SampleRemotePalInterfaceIPC() = default;

SampleRemotePalInterfaceIPC::FrameCallbacks::FrameCallbacks() = default;
SampleRemotePalInterfaceIPC::FrameCallbacks::~FrameCallbacks() = default;

content::RenderFrame* SampleRemotePalInterfaceIPC::getFrame() const {
  content::RenderFrame* frame = content::RenderFrame::FromWebFrame(
      blink::WebLocalFrame::FrameForCurrentContext());
  if (!frame) {
    LOG(ERROR) << "Frame isn't available in Remote 'Sample' Interface";
    return nullptr;
  }
  return frame;
}

std::string SampleRemotePalInterfaceIPC::GetValue() const {
  LOG(ERROR) << "PAL method described as 'no_IPC' was called remotely.";
  return *new (std::string);
}

void SampleRemotePalInterfaceIPC::CallFunc(std::string arg1, std::string arg2) {
  content::RenderFrame* frame = getFrame();
  if (!frame)
    return;
  std::unique_ptr<pal::SampleProxy> proxy(new pal::SampleProxy(frame));
  proxy->CallFunc(arg1, arg2);
  return;
}

// static
template <typename _ID, typename _Map>
_ID SampleRemotePalInterfaceIPC::GetAsyncMsgId(_ID& async_msg_id_, _Map& map) {
  _ID max = std::numeric_limits<_ID>::max();
  if (++async_msg_id_ == max)
    async_msg_id_ = 0;
  if (map.size() == (size_t)max) {
    LOG(ERROR) << "Remote PAL Async callback map is full.";
    return async_msg_id_;
  }
  if (map.find(async_msg_id_) != map.end())
    return GetAsyncMsgId<_ID, _Map>(async_msg_id_, map);
  return async_msg_id_;
}

// static
int SampleRemotePalInterfaceIPC::process_data_callback_id_ = 0;

void SampleRemotePalInterfaceIPC::ProcessData(
    std::string data,
    int32_t callback_index,
    const ProcessDataRespondCallback& on_process_data_done) {
  content::RenderFrame* frame = getFrame();
  if (!frame)
    return;

  size_t pal_async_callback_id =
      GetAsyncMsgId<int, ProcessDataRespondCallbackMAP>(
          process_data_callback_id_, process_data_respond_callbacks_);
  process_data_respond_callbacks_[pal_async_callback_id] = on_process_data_done;

  std::unique_ptr<pal::SampleProxy> proxy(new pal::SampleProxy(frame));
  proxy->ProcessData(pal_async_callback_id, data, callback_index);
}

void SampleRemotePalInterfaceIPC::SubscribeToEvent() {
  content::RenderFrame* frame = getFrame();
  if (!frame)
    return;
  std::unique_ptr<pal::SampleProxy> proxy(new pal::SampleProxy(frame));
  proxy->SubscribeToEvent();
  return;
}

std::unique_ptr<SampleInterface::SampleUpdateSubscription>
SampleRemotePalInterfaceIPC::AddCallback(
    const SampleInterface::SampleUpdateCallback& callback) {
  content::RenderFrame* const frame = getFrame();
  if (!frame)
    return nullptr;

  int id = frame->GetRoutingID();
  bool need_subscribe = false;
  FrameCallbacksMAP::iterator it = callbacks_.find(id);
  if (it == callbacks_.end()) {
    need_subscribe = true;
    callbacks_[id] = std::unique_ptr<FrameCallbacks>(new FrameCallbacks);
  }
  std::unique_ptr<SampleInterface::SampleUpdateSubscription> subscr =
      callbacks_[id]->sample_update_callbacks_.Add(callback);

  if (need_subscribe) {
    std::unique_ptr<pal::SampleProxy> proxy(new pal::SampleProxy(frame));
    proxy->Subscribe();
  }

  return subscr;
}

}  // namespace pal
