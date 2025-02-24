// This file is generated by PAL generator, do not modify.
// To make changes, modify the source file:
// test5.json

#ifndef PAL_PUBLIC_BROWSER_NATIVE_CONTROLS_HOST_H_
#define PAL_PUBLIC_BROWSER_NATIVE_CONTROLS_HOST_H_

#include <set>

#include "content/public/browser/browser_message_filter.h"
#include "pal/ipc/pal_export.h"
#include "pal/public/interfaces/native_controls_interface.h"

namespace pal {

class PAL_EXPORT NativeControlsHost : public content::BrowserMessageFilter {
 public:
  NativeControlsHost();

  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  ~NativeControlsHost() override;

  void SendColorChosen(int color);

  std::set<int> route_ids_;
  NativeControlsInterface::ColorChosenCallback mColorChosenCallback;
  std::unique_ptr<NativeControlsInterface::ColorChosenSubscription>
      mColorChosenSubscription;

  void AddCallbacks();

  base::WeakPtrFactory<NativeControlsHost> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(NativeControlsHost);
};

}  // namespace pal

#endif  // PAL_PUBLIC_BROWSER_NATIVE_CONTROLS_HOST_H_