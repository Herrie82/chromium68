// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_javascript_dialog_manager.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/shell/browser/shell_javascript_dialog.h"
#include "content/shell/browser/shell_javascript_dialog_neva.h"
#include "content/shell/common/shell_neva_switches.h"
#include "content/shell/common/shell_switches.h"

namespace content {

ShellJavaScriptDialogManager::ShellJavaScriptDialogManager()
    : should_proceed_on_beforeunload_(true), beforeunload_success_(true) {}

ShellJavaScriptDialogManager::~ShellJavaScriptDialogManager() {
}

void ShellJavaScriptDialogManager::RunJavaScriptDialog(
    WebContents* web_contents,
    RenderFrameHost* render_frame_host,
    JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    DialogClosedCallback callback,
    bool* did_suppress_message) {
  if (!dialog_request_callback_.is_null()) {
    dialog_request_callback_.Run();
    std::move(callback).Run(true, base::string16());
    dialog_request_callback_.Reset();
    return;
  }

#if defined(OS_MACOSX) || defined(OS_WIN)
  *did_suppress_message = false;

  if (dialog_) {
    // One dialog at a time, please.
    *did_suppress_message = true;
    return;
  }

  base::string16 new_message_text =
      url_formatter::FormatUrl(render_frame_host->GetLastCommittedURL()) +
      base::ASCIIToUTF16("\n\n") + message_text;
  gfx::NativeWindow parent_window = web_contents->GetTopLevelNativeWindow();

  dialog_.reset(new ShellJavaScriptDialog(this, parent_window, dialog_type,
                                          new_message_text, default_prompt_text,
                                          std::move(callback)));
#else
  // TODO: implement ShellJavaScriptDialog for other platforms, drop this #if
  *did_suppress_message = true;
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseExternalJsDialogs)) {
    if (dialog_) {
      // One dialog at a time, please.
      return;
    }

    *did_suppress_message = false;
    gfx::NativeWindow parent_window = web_contents->GetTopLevelNativeWindow();
    dialog_.reset(new neva::ShellJavaScriptDialog(
        this, parent_window, render_frame_host->GetLastCommittedURL(), dialog_type, message_text,
        default_prompt_text, std::move(callback)));
  }
  return;
#endif
}

void ShellJavaScriptDialogManager::RunBeforeUnloadDialog(
    WebContents* web_contents,
    RenderFrameHost* render_frame_host,
    bool is_reload,
    DialogClosedCallback callback) {
  // During tests, if the BeforeUnload should not proceed automatically, store
  // the callback and return.
  if (!dialog_request_callback_.is_null()) {
    dialog_request_callback_.Run();

    if (should_proceed_on_beforeunload_)
      std::move(callback).Run(beforeunload_success_, base::string16());
    else
      before_unload_callback_ = std::move(callback);
    dialog_request_callback_.Reset();
    return;
  }

#if defined(OS_MACOSX) || defined(OS_WIN)
  if (dialog_) {
    // Seriously!?
    std::move(callback).Run(true, base::string16());
    return;
  }

  base::string16 message_text =
      base::ASCIIToUTF16("Is it OK to leave/reload this page?");

  gfx::NativeWindow parent_window = web_contents->GetTopLevelNativeWindow();

  dialog_.reset(new ShellJavaScriptDialog(
      this, parent_window, JAVASCRIPT_DIALOG_TYPE_CONFIRM, message_text,
      base::string16(),  // default
      std::move(callback)));
#else
  // TODO: implement ShellJavaScriptDialog for other platforms, drop this #if
  std::move(callback).Run(true, base::string16());
  return;
#endif
}

void ShellJavaScriptDialogManager::CancelDialogs(WebContents* web_contents,
                                                 bool reset_state) {
#if defined(OS_MACOSX) || defined(OS_WIN)
  if (dialog_) {
    dialog_->Cancel();
    dialog_.reset();
  }
#else
  // TODO: implement ShellJavaScriptDialog for other platforms, drop this #if
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseExternalJsDialogs)) {

    if (dialog_) {
      dialog_->Cancel();
      dialog_.reset();
    }
  }
#endif

  if (before_unload_callback_.is_null())
    return;

  if (reset_state)
    std::move(before_unload_callback_).Run(false, base::string16());
}

void ShellJavaScriptDialogManager::DialogClosed(ShellJavaScriptDialog* dialog) {
#if defined(OS_MACOSX) || defined(OS_WIN)
  DCHECK_EQ(dialog, dialog_.get());
  dialog_.reset();
#else
  // TODO: implement ShellJavaScriptDialog for other platforms, drop this #if
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseExternalJsDialogs)) {

    dialog_.reset();
  }
#endif
}

}  // namespace content
