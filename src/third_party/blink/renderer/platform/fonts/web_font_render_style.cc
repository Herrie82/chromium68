// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_font_render_style.h"

#include "build/build_config.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/fonts/font_description.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_font.h"
#include "third_party/blink/renderer/platform/layout_test_support.h"

namespace blink {

namespace {

SkPaint::Hinting g_skia_hinting = SkPaint::kNormal_Hinting;
bool g_use_skia_auto_hint = true;
bool g_use_skia_bitmaps = true;
bool g_use_skia_anti_alias = true;
bool g_use_skia_subpixel_rendering = false;
#if defined(USE_NEVA_APPRUNTIME)
bool g_use_fake_bold_text = true;
#endif

}  // namespace

// static
void WebFontRenderStyle::SetSkiaFontManager(sk_sp<SkFontMgr> font_mgr) {
  FontCache::SetFontManager(std::move(font_mgr));
}

// static
void WebFontRenderStyle::SetHinting(SkPaint::Hinting hinting) {
  g_skia_hinting = hinting;
}

// static
void WebFontRenderStyle::SetAutoHint(bool use_auto_hint) {
  g_use_skia_auto_hint = use_auto_hint;
}

// static
void WebFontRenderStyle::SetUseBitmaps(bool use_bitmaps) {
  g_use_skia_bitmaps = use_bitmaps;
}

// static
void WebFontRenderStyle::SetAntiAlias(bool use_anti_alias) {
  g_use_skia_anti_alias = use_anti_alias;
}

// static
void WebFontRenderStyle::SetSubpixelRendering(bool use_subpixel_rendering) {
  g_use_skia_subpixel_rendering = use_subpixel_rendering;
}

// static
void WebFontRenderStyle::SetSubpixelPositioning(bool use_subpixel_positioning) {
  FontDescription::SetSubpixelPositioning(use_subpixel_positioning);
}

// static
void WebFontRenderStyle::SetSystemFontFamily(const WebString& name) {
  FontCache::SetSystemFontFamily(name);
}

#if defined(USE_NEVA_APPRUNTIME)
// static
void WebFontRenderStyle::SetAllowFakeBoldText(bool use_fake_bold_text) {
  g_use_fake_bold_text = use_fake_bold_text;
}

// static
bool WebFontRenderStyle::GetAllowFakeBoldText() {
  return g_use_fake_bold_text;
}
#endif

// static
WebFontRenderStyle WebFontRenderStyle::GetDefault() {
  WebFontRenderStyle result;
  result.use_anti_alias = g_use_skia_anti_alias;
  result.hint_style = SkPaint::kNo_Hinting;
  result.hint_style = g_skia_hinting;
  result.use_bitmaps = g_use_skia_bitmaps;
  result.use_auto_hint = g_use_skia_auto_hint;
  result.use_anti_alias = g_use_skia_anti_alias;
  result.use_subpixel_rendering = g_use_skia_subpixel_rendering;
  result.use_subpixel_positioning = FontDescription::SubpixelPositioning();
#if defined(USE_NEVA_APPRUNTIME)
  result.use_fake_bold_text = g_use_fake_bold_text;
#endif
  return result;
}

void WebFontRenderStyle::OverrideWith(const WebFontRenderStyle& other) {
  if (other.use_anti_alias != WebFontRenderStyle::kNoPreference)
    use_anti_alias = other.use_anti_alias;

  if (other.use_hinting != WebFontRenderStyle::kNoPreference) {
    use_hinting = other.use_hinting;
    hint_style = other.hint_style;
  }

  if (other.use_bitmaps != WebFontRenderStyle::kNoPreference)
    use_bitmaps = other.use_bitmaps;
  if (other.use_auto_hint != WebFontRenderStyle::kNoPreference)
    use_auto_hint = other.use_auto_hint;
  if (other.use_anti_alias != WebFontRenderStyle::kNoPreference)
    use_anti_alias = other.use_anti_alias;
  if (other.use_subpixel_rendering != WebFontRenderStyle::kNoPreference)
    use_subpixel_rendering = other.use_subpixel_rendering;
  if (other.use_subpixel_positioning != WebFontRenderStyle::kNoPreference)
    use_subpixel_positioning = other.use_subpixel_positioning;
}

void WebFontRenderStyle::ApplyToPaintFont(PaintFont& font,
                                          float device_scale_factor) const {
  auto sk_hint_style = static_cast<SkPaint::Hinting>(hint_style);
  font.SetAntiAlias(use_anti_alias);
  font.SetHinting(sk_hint_style);
  font.SetEmbeddedBitmapText(use_bitmaps);
  font.SetAutohinted(use_auto_hint);
  if (use_anti_alias)
    font.SetLcdRenderText(use_subpixel_rendering);

  // Force-enable subpixel positioning, except when full hinting is requested on
  // low-dpi screen or when running layout tests.
  bool force_subpixel_positioning =
      !LayoutTestSupport::IsRunningLayoutTest() &&
      (sk_hint_style != SkPaint::kFull_Hinting || device_scale_factor > 1.0f);

  font.SetSubpixelText(force_subpixel_positioning || use_subpixel_positioning);
}

}  // namespace blink
