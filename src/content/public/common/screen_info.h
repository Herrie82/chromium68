// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SCREEN_INFO_H_
#define CONTENT_PUBLIC_COMMON_SCREEN_INFO_H_

#include "build/build_config.h"
#include "content/common/content_export.h"
#include "content/public/common/screen_orientation_values.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/icc_profile.h"
#if defined(USE_NEVA_MEDIA)
#include "ui/gfx/geometry/point_f.h"
#endif
#if defined(USE_NEVA_APPRUNTIME)
#include "ui/gfx/geometry/size.h"
#endif

namespace content {

// Information about the screen on which a RenderWidget is being displayed. This
// is the content counterpart to WebScreenInfo in blink.
struct CONTENT_EXPORT ScreenInfo {
    ScreenInfo();
    ScreenInfo(const ScreenInfo& other);
    ~ScreenInfo();

    // Device scale factor. Specifies the ratio between physical and logical
    // pixels.
    float device_scale_factor = 1.f;

    // The color space of the output display.
    gfx::ColorSpace color_space = gfx::ColorSpace::CreateSRGB();

#if defined(OS_MACOSX)
    // The ICC profile from which |color_space| was derived, if any. This is
    // used only on macOS, to ensure that the color profile set on an IOSurface
    // exactly match that of the display, when possible (because that has
    // significant power implications).
    // https://crbug.com/766736#c1
    gfx::ICCProfile icc_profile;
#endif

    // The screen depth in bits per pixel
    uint32_t depth = 0;

    // The bits per colour component. This assumes that the colours are balanced
    // equally.
    uint32_t depth_per_component = 0;

    // This can be true for black and white printers
    bool is_monochrome = false;

    // The display monitor rectangle in virtual-screen coordinates. Note that
    // this may be negative.
    gfx::Rect rect;

    // The portion of the monitor's rectangle that can be used by applications.
    gfx::Rect available_rect;

    // The monitor's orientation.
    ScreenOrientationValues orientation_type =
        SCREEN_ORIENTATION_VALUES_DEFAULT;

    // This is the orientation angle of the displayed content in degrees.
    // It is the opposite of the physical rotation.
    uint16_t orientation_angle = 0;
#if defined(USE_NEVA_MEDIA)
    gfx::PointF additional_contents_scale = gfx::PointF(1.f,1.f);
#endif

#if defined(USE_NEVA_APPRUNTIME)
    // hardware resolution
    gfx::Size hardware_resolution;
#endif

    bool operator==(const ScreenInfo& other) const;
    bool operator!=(const ScreenInfo& other) const;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_SCREEN_INFO_H_
