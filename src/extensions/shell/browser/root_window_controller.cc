// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/root_window_controller.h"

#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "extensions/shell/browser/shell_app_delegate.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tracker.h"
#include "ui/aura/window_tree_host.h"
#include "ui/display/display.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/wm/core/default_screen_position_client.h"

// neva include
#include "components/guest_view/browser/guest_view_base.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_plugin_guest_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/guest_view/web_view/web_view_guest.h"

#if defined(USE_NEVA_MEDIA)
#include "content/public/browser/neva/media_state_manager.h"
#endif

namespace extensions {

namespace {

// A simple layout manager that makes each new window fill its parent.
class FillLayout : public aura::LayoutManager {
 public:
  FillLayout(aura::Window* owner) : owner_(owner) { DCHECK(owner_); }
  ~FillLayout() override = default;

 private:
  // aura::LayoutManager:
  void OnWindowResized() override {
    // Size the owner's immediate child windows.
    aura::WindowTracker children_tracker(owner_->children());
    while (!children_tracker.windows().empty()) {
      aura::Window* child = children_tracker.Pop();
      child->SetBounds(gfx::Rect(owner_->bounds().size()));
    }
  }

  void OnWindowAddedToLayout(aura::Window* child) override {
    DCHECK_EQ(owner_, child->parent());

    // Create a rect at 0,0 with the size of the parent.
    gfx::Size parent_size = child->parent()->bounds().size();
    child->SetBounds(gfx::Rect(parent_size));
  }

  void OnWillRemoveWindowFromLayout(aura::Window* child) override {}

  void OnWindowRemovedFromLayout(aura::Window* child) override {}

  void OnChildWindowVisibilityChanged(aura::Window* child,
                                      bool visible) override {}

  void SetChildBounds(aura::Window* child,
                      const gfx::Rect& requested_bounds) override {
    SetChildBoundsDirect(child, requested_bounds);
  }

  aura::Window* owner_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(FillLayout);
};

// A simple screen positioning client that translates bounds to screen
// coordinates using the offset of the root window in screen coordinates.
class ScreenPositionClient : public wm::DefaultScreenPositionClient {
 public:
  ScreenPositionClient() = default;
  ~ScreenPositionClient() override = default;

  // wm::DefaultScreenPositionClient:
  void SetBounds(aura::Window* window,
                 const gfx::Rect& bounds,
                 const display::Display& display) override {
    aura::Window* root_window = window->GetRootWindow();
    DCHECK(window);

    // Convert the window's origin to its root window's coordinates.
    gfx::Point origin = bounds.origin();
    aura::Window::ConvertPointToTarget(window->parent(), root_window, &origin);

    // Translate the origin by the root window's offset in screen coordinates.
    gfx::Point host_origin = GetOriginInScreen(root_window);
    origin.Offset(-host_origin.x(), -host_origin.y());
    window->SetBounds(gfx::Rect(origin, bounds.size()));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ScreenPositionClient);
};

}  // namespace

RootWindowController::RootWindowController(
    DesktopDelegate* desktop_delegate,
    const gfx::Rect& bounds,
    content::BrowserContext* browser_context)
    : desktop_delegate_(desktop_delegate),
      browser_context_(browser_context),
      screen_position_client_(std::make_unique<ScreenPositionClient>()) {
  DCHECK(desktop_delegate_);
  DCHECK(browser_context_);
  host_.reset(aura::WindowTreeHost::Create(bounds));

  host_->InitHost();
  host_->window()->Show();

  aura::client::SetWindowParentingClient(host_->window(), this);
  aura::client::SetScreenPositionClient(host_->window(),
                                        screen_position_client_.get());

  // Ensure the window fills the display.
  host_->window()->SetLayoutManager(new FillLayout(host_->window()));

  host_->AddObserver(this);
  host_->Show();
}

RootWindowController::~RootWindowController() {
  CloseAppWindows();
  DestroyWindowTreeHost();
}

void RootWindowController::AddAppWindow(AppWindow* app_window,
                                        gfx::NativeWindow window) {
  if (app_windows_.empty()) {
    // Start observing for OnAppWindowRemoved.
    AppWindowRegistry* registry = AppWindowRegistry::Get(browser_context_);
    registry->AddObserver(this);
  }

  app_windows_.push_back(app_window);

  aura::Window* root_window = host_->window();
  root_window->AddChild(window);
}

void RootWindowController::RemoveAppWindow(AppWindow* app_window) {
  host_->window()->RemoveChild(app_window->GetNativeWindow());
  app_windows_.remove(app_window);
  if (app_windows_.empty())
    AppWindowRegistry::Get(browser_context_)->RemoveObserver(this);
}

void RootWindowController::CloseAppWindows() {
  if (app_windows_.empty())
    return;

  // Remove the observer before closing windows to avoid triggering
  // OnAppWindowRemoved, which would mutate |app_windows_|.
  AppWindowRegistry::Get(browser_context_)->RemoveObserver(this);
  for (AppWindow* app_window : app_windows_)
    app_window->GetBaseWindow()->Close();  // Close() deletes |app_window|.
  app_windows_.clear();
}

void RootWindowController::UpdateSize(const gfx::Size& size) {
  host_->SetBoundsInPixels(gfx::Rect(size));
}

aura::Window* RootWindowController::GetDefaultParent(aura::Window* window,
                                                     const gfx::Rect& bounds) {
  return host_->window();
}

void RootWindowController::OnHostCloseRequested(aura::WindowTreeHost* host) {
  DCHECK_EQ(host_.get(), host);
  CloseAppWindows();

  // The ShellDesktopControllerAura will delete us.
  desktop_delegate_->CloseRootWindowController(this);
}

#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
void RootWindowController::OnWindowHostStateChanged(aura::WindowTreeHost* host,
                                                    ui::WidgetState new_state) {
  if (app_windows_.empty())
    return;

  if (new_state == ui::WidgetState::MINIMIZED ||
      new_state == ui::WidgetState::MAXIMIZED) {
    for (AppWindow* app_window : app_windows_) {
      content::WebContents* web_contents = app_window->web_contents();
      if (web_contents == nullptr)
        continue;
      content::BrowserContext* browser_context =
          web_contents->GetBrowserContext();
      if (browser_context == nullptr ||
          browser_context->GetGuestManager() == nullptr)
        continue;
      browser_context->GetGuestManager()->ForEachGuest(
          web_contents,
          base::Bind([](ui::WidgetState new_state, content::WebContents* guest_contents) {
            WebViewGuest* guest_view =
                guest_view::GuestViewBase::FromWebContents(guest_contents)
                    ->As<WebViewGuest>();
            // Suspend or resume only for non-suspended WebViewGuest
            // Embeder will take care of suspended WebViewGuest
#if defined(USE_NEVA_MEDIA)
            if (guest_view != nullptr && !guest_view->is_suspended()) {
              if (new_state == ui::WidgetState::MINIMIZED) {
                content::MediaStateManager::GetInstance()->SuspendAllMedia(
                    guest_contents);
              } else if (new_state == ui::WidgetState::MAXIMIZED) {
                content::MediaStateManager::GetInstance()->ResumeAllMedia(
                    guest_contents);
              }
            }
#endif
            return false;
          }, new_state));
    }
  }
}

void RootWindowController::OnWindowHostClose(aura::WindowTreeHost* host) {
  OnHostCloseRequested(host);
}
#endif

void RootWindowController::OnAppWindowRemoved(AppWindow* window) {
  if (app_windows_.empty())
    return;

  // If we created this AppWindow, remove it from our list so we don't try to
  // close it again later.
  app_windows_.remove(window);

  // Close when all AppWindows are closed.
  if (app_windows_.empty()) {
    AppWindowRegistry::Get(browser_context_)->RemoveObserver(this);
    desktop_delegate_->CloseRootWindowController(this);
  }
}

void RootWindowController::DestroyWindowTreeHost() {
  host_->RemoveObserver(this);
  host_.reset();
}

}  // namespace extensions
