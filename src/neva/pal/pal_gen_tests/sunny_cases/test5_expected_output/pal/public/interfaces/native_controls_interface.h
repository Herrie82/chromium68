// This file is generated by PAL generator, do not modify.
// To make changes, modify the source file:
// test5.json

#ifndef PAL_PUBLIC_INTERFACES_NATIVE_CONTROLS_INTERFACE_H_
#define PAL_PUBLIC_INTERFACES_NATIVE_CONTROLS_INTERFACE_H_

#include <memory>
#include "base/callback.h"
#include "base/callback_list.h"
#include "pal/ipc/pal_export.h"

namespace pal {
// The callback is called to notify client with recieved service response
using OpenColorChooserRespondCallback = base::Callback<void()>;

// This interface serves to call platform
// implementation of JS controls:
// File Chooser, ...

class PAL_EXPORT NativeControlsInterface {
 public:
  using ColorChosenCallback = base::Callback<void(int color)>;

  using ColorChosenCallbackList = base::CallbackList<void(int color)>;

  using ColorChosenSubscription = ColorChosenCallbackList::Subscription;

  virtual ~NativeControlsInterface(){};

  virtual void OpenColorChooser(
      const std::string& color_params,                      //
      const OpenColorChooserRespondCallback& on_done) = 0;  // Callback to
                                                            // notify client
                                                            // with received
                                                            // service response

  virtual void CloseColorChooser() = 0;

  using RunFileChooserRespondCallback =
      base::Callback<void(std::string selected_files)>;

  virtual void RunFileChooser(
      int mode,                                            //
      const std::string& title,                            //
      const std::string& default_file_name,                //
      const std::vector<std::string>& accept_types,        //
      bool need_local_path,                                //
      const std::string& url,                              //
      const RunFileChooserRespondCallback& callback) = 0;  // Return callback

  virtual std::unique_ptr<ColorChosenSubscription> AddCallback(
      const ColorChosenCallback& callback) = 0;
};

}  // namespace pal

#endif  // PAL_PUBLIC_INTERFACES_NATIVE_CONTROLS_INTERFACE_H_