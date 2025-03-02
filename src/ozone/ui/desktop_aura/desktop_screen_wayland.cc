// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/ui/desktop_aura/desktop_screen_wayland.h"

#include "ozone/platform/desktop_platform_screen.h"
#include "ozone/platform/ozone_platform_wayland.h"
#include "ozone/ui/desktop_aura/desktop_window_tree_host_ozone_wayland.h"
#include "ui/aura/window.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"

namespace views {

DesktopScreenWayland::DesktopScreenWayland()
    : display::Screen(),
      rect_(0, 0, 0, 0),
      rotation_(0),
      displays_(),
      delegate_(nullptr) {
  platform_Screen_ = CreatePlatformScreen(this);
}

DesktopScreenWayland::~DesktopScreenWayland() {
}

void DesktopScreenWayland::SetGeometry(const gfx::Rect& geometry) {
  rect_ = geometry;
  int max_area = 0;
  const display::Display* matching = NULL;
  for (std::vector<display::Display>::const_iterator it = displays_.begin();
       it != displays_.end(); ++it) {
    gfx::Rect intersect = gfx::IntersectRects(it->bounds(), rect_);
    int area = intersect.width() * intersect.height();
    if (area > max_area) {
      max_area = area;
      matching = &*it;
    }
  }

  if (!matching) {
    std::vector<display::Display> old_displays = displays_;
    displays_.push_back(display::Display(displays_.size(), rect_));
    change_notifier_.NotifyDisplaysChanged(old_displays, displays_);
  }
}

gfx::Point DesktopScreenWayland::GetCursorScreenPoint() {
  if (delegate_)
    return delegate_->GetCursorScreenPoint();

  return platform_Screen_->GetCursorScreenPoint();
}

bool DesktopScreenWayland::IsWindowUnderCursor(gfx::NativeWindow window) {
  NOTIMPLEMENTED();
  return false;
}

gfx::NativeWindow DesktopScreenWayland::GetWindowAtScreenPoint(
    const gfx::Point& point) {
  const std::vector<aura::Window*>& windows =
      views::DesktopWindowTreeHostOzone::GetAllOpenWindows();
  for (std::vector<aura::Window*>::const_iterator it = windows.begin();
         it != windows.end(); ++it) {
    if ((*it)->GetBoundsInScreen().Contains(point))
      return *it;
  }

  return NULL;
}

int DesktopScreenWayland::GetNumDisplays() const {
  return displays_.size();
}

const std::vector<display::Display>& DesktopScreenWayland::GetAllDisplays() const {
  return displays_;
}

display::Display DesktopScreenWayland::GetDisplayNearestWindow(
    gfx::NativeView window) const {
  DCHECK(!rect_.IsEmpty());
  if (displays_.size() == 1)
    return displays_.front();
  // Getting screen bounds here safely is hard.
  //
  // You'd think we'd be able to just call window->GetBoundsInScreen(), but we
  // can't because |window| (and the associated RootWindow*) can be partially
  // initialized at this point; RootWindow initializations call through into
  // GetDisplayNearestWindow(). But the wayland resources are created before we
  // create the aura::RootWindow. So we ask what the DRWH believes the
  // window bounds are instead of going through the aura::Window's screen
  // bounds.
  aura::WindowTreeHost* host = window->GetHost();
  if (host) {
    DesktopWindowTreeHostOzone* rwh =
        DesktopWindowTreeHostOzone::GetHostForAcceleratedWidget(
            host->GetAcceleratedWidget());
    if (rwh)
      return GetDisplayMatching(rwh->GetBoundsInScreen());
  }

  return GetPrimaryDisplay();
}

display::Display DesktopScreenWayland::GetDisplayNearestPoint(
    const gfx::Point& point) const {
  if (displays_.size() == 1)
    return displays_.front();

  for (std::vector<display::Display>::const_iterator it = displays_.begin();
         it != displays_.end(); ++it) {
    if (it->bounds().Contains(point))
      return *it;
  }

  return GetPrimaryDisplay();
}

display::Display DesktopScreenWayland::GetDisplayMatching(
    const gfx::Rect& match_rect) const {
  if (displays_.size() == 1)
    return displays_.front();

  DCHECK(!rect_.IsEmpty());
  int max_area = 0;
  const display::Display* matching = NULL;
  for (std::vector<display::Display>::const_iterator it = displays_.begin();
       it != displays_.end(); ++it) {
    gfx::Rect intersect = gfx::IntersectRects(it->bounds(), match_rect);
    int area = intersect.width() * intersect.height();
    if (area > max_area) {
      max_area = area;
      matching = &*it;
    }
  }

  // Fallback to the primary display if there is no matching display.
  return matching ? *matching : GetPrimaryDisplay();
}

display::Display DesktopScreenWayland::GetPrimaryDisplay() const {
  DCHECK(!rect_.IsEmpty());
  return displays_.front();
}

void DesktopScreenWayland::AddObserver(display::DisplayObserver* observer) {
  change_notifier_.AddObserver(observer);
}

void DesktopScreenWayland::RemoveObserver(display::DisplayObserver* observer) {
  change_notifier_.RemoveObserver(observer);
}

void DesktopScreenWayland::SetDelegate(display::ScreenDelegate* delegate) {
  delegate_ = delegate;

  if (delegate_) {
    gfx::Size preferredSize = delegate_->GetPreferredScreenSize(rect_.size());
    int preferredRotation =
        delegate_->GetPreferredScreenRotationAsDegrees(rotation_);
    OnScreenChanged(preferredSize.width(), preferredSize.height(),
                    preferredRotation);
  }
}

void DesktopScreenWayland::OnScreenChanged(unsigned width,
                                           unsigned height,
                                           int rotation) {
  gfx::Size size = gfx::Size(width, height);
  if (delegate_) {
    size = delegate_->GetPreferredScreenSize(size);
    rotation = delegate_->GetPreferredScreenRotationAsDegrees(rotation);
  }

  if (rect_.width() != size.width() || rect_.height() != size.height() ||
      rotation_ != rotation) {
    rect_ = gfx::Rect(0, 0, size.width(), size.height());
    rotation_ = rotation;

    std::vector<display::Display> old_displays = displays_;
    std::vector<display::Display> new_displays;

    if (!displays_.size())
      displays_.push_back(display::Display(displays_.size(), rect_));

    for (std::vector<display::Display>::iterator it = displays_.begin();
         it != displays_.end(); ++it) {
      display::Display display = *it;
      display.set_bounds(gfx::Rect(0, 0, rect_.width(), rect_.height()));
      display.set_work_area(gfx::Rect(0, 0, rect_.width(), rect_.height()));
      display.SetRotationAsDegree(rotation);
      new_displays.push_back(display);
    }

    displays_ = new_displays;
    change_notifier_.NotifyDisplaysChanged(old_displays, displays_);
  }
}

}  // namespace views
