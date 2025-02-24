// This file is generated by PAL generator, do not modify.
// To make changes, modify the source file:
// test4.json

#ifndef PAL_PUBLIC_INTERFACES_NATIVE_DIALOGS_INTERFACE_H_
#define PAL_PUBLIC_INTERFACES_NATIVE_DIALOGS_INTERFACE_H_

#include "base/callback.h"
#include "pal/ipc/pal_export.h"

namespace pal {
// Test situation when all methods in the interface have
//  'no_IPC' property = 'true'

class PAL_EXPORT NativeDialogsInterface {
 public:
  virtual ~NativeDialogsInterface(){};

  using RunJavaScriptDialogRespondCallback =
      base::Callback<void(bool success, std::string& user_input)>;

  virtual void RunJavaScriptDialog(int dialog_type,                         //
                                   const std::string& message_text,         //
                                   const std::string& default_prompt_text,  //
                                   const std::string& url,                  //
                                   const RunJavaScriptDialogRespondCallback&
                                       callback) = 0;  // Return callback
};

}  // namespace pal

#endif  // PAL_PUBLIC_INTERFACES_NATIVE_DIALOGS_INTERFACE_H_