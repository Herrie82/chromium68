// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/desktop_browser_frame_aura.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/views/frame/browser_desktop_window_tree_host.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/web_applications/web_app.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_observer.h"
#include "ui/base/hit_test.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/font.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/visibility_controller.h"

#if defined(OS_WEBOS)
#include "base/command_line.h"
#include "extensions/common/switches.h"
#include "ui/aura/window_tree_host.h"
#endif

using aura::Window;

///////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAura, public:

DesktopBrowserFrameAura::DesktopBrowserFrameAura(
    BrowserFrame* browser_frame,
    BrowserView* browser_view)
    : views::DesktopNativeWidgetAura(browser_frame),
      browser_view_(browser_view),
      browser_frame_(browser_frame),
      browser_desktop_window_tree_host_(nullptr) {
  GetNativeWindow()->SetName("BrowserFrameAura");
}

///////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAura, protected:

DesktopBrowserFrameAura::~DesktopBrowserFrameAura() {
}

///////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAura, views::DesktopNativeWidgetAura overrides:

void DesktopBrowserFrameAura::OnHostClosed() {
  aura::client::SetVisibilityClient(GetNativeView()->GetRootWindow(), nullptr);
  DesktopNativeWidgetAura::OnHostClosed();
}

void DesktopBrowserFrameAura::InitNativeWidget(
    const views::Widget::InitParams& params) {
  browser_desktop_window_tree_host_ =
      BrowserDesktopWindowTreeHost::CreateBrowserDesktopWindowTreeHost(
          browser_frame_,
          this,
          browser_view_,
          browser_frame_);
  views::Widget::InitParams modified_params = params;
  modified_params.desktop_window_tree_host =
      browser_desktop_window_tree_host_->AsDesktopWindowTreeHost();
  DesktopNativeWidgetAura::InitNativeWidget(modified_params);

#if defined(OS_WEBOS)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(extensions::switches::kWebOSAppId)) {
    std::string app_id = command_line->GetSwitchValueASCII(extensions::switches::kWebOSAppId);
    if (host())
      host()->SetWindowProperty("appId", app_id);
  }
#endif

  visibility_controller_.reset(new wm::VisibilityController);
  aura::client::SetVisibilityClient(GetNativeView()->GetRootWindow(),
                                    visibility_controller_.get());
  wm::SetChildWindowVisibilityChangesAnimated(
      GetNativeView()->GetRootWindow());
}

////////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAura, NativeBrowserFrame implementation:

views::Widget::InitParams DesktopBrowserFrameAura::GetWidgetParams() {
  views::Widget::InitParams params;
  params.native_widget = this;
  return params;
}

bool DesktopBrowserFrameAura::UseCustomFrame() const {
  return true;
}

bool DesktopBrowserFrameAura::UsesNativeSystemMenu() const {
  return browser_desktop_window_tree_host_->UsesNativeSystemMenu();
}

int DesktopBrowserFrameAura::GetMinimizeButtonOffset() const {
  return browser_desktop_window_tree_host_->GetMinimizeButtonOffset();
}

bool DesktopBrowserFrameAura::ShouldSaveWindowPlacement() const {
  // The placement can always be stored.
  return true;
}

void DesktopBrowserFrameAura::GetWindowPlacement(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  *bounds = GetWidget()->GetRestoredBounds();
  if (IsMaximized())
    *show_state = ui::SHOW_STATE_MAXIMIZED;
  else if (IsMinimized())
    *show_state = ui::SHOW_STATE_MINIMIZED;
  else
    *show_state = ui::SHOW_STATE_NORMAL;
}

bool DesktopBrowserFrameAura::PreHandleKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  return false;
}

bool DesktopBrowserFrameAura::HandleKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  return false;
}
