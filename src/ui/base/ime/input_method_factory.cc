// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/input_method_factory.h"

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "ui/base/ime/mock_input_method.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_neva_switches.h"
#include "ui/gfx/switches.h"

#if defined(OS_CHROMEOS)
#include "ui/base/ime/input_method_chromeos.h"
#elif defined(OS_WIN)
#include "ui/base/ime/input_method_win.h"
#include "ui/base/ime/input_method_win_tsf.h"
#elif defined(OS_MACOSX)
#include "ui/base/ime/input_method_mac.h"
// Removed defined(USE_X11) for ozone-wayland port
#elif defined(USE_AURA)
#include "ui/base/ime/neva/input_method_auralinux_neva.h"
#include "ui/base/ime/input_method_auralinux.h"
#else
#include "ui/base/ime/input_method_minimal.h"
#endif

namespace {

ui::InputMethod* g_input_method_for_testing = nullptr;

bool g_input_method_set_for_testing = false;

bool g_create_input_method_called = false;

}  // namespace

namespace ui {

std::unique_ptr<InputMethod> CreateInputMethod(
    internal::InputMethodDelegate* delegate,
    gfx::AcceleratedWidget widget) {
  if (!g_create_input_method_called)
    g_create_input_method_called = true;

  if (g_input_method_for_testing) {
    ui::InputMethod* ret = g_input_method_for_testing;
    g_input_method_for_testing = nullptr;
    return base::WrapUnique(ret);
  }

  if (g_input_method_set_for_testing)
    return std::make_unique<MockInputMethod>(delegate);

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kHeadless))
    return base::WrapUnique(new MockInputMethod(delegate));

#if defined(OS_CHROMEOS)
  return std::make_unique<InputMethodChromeOS>(delegate);
#elif defined(OS_WIN)
  if (base::FeatureList::IsEnabled(features::kTSFImeSupport))
    return std::make_unique<InputMethodWinTSF>(delegate, widget);
  return std::make_unique<InputMethodWin>(delegate, widget);
#elif defined(OS_MACOSX)
  return std::make_unique<InputMethodMac>(delegate);
// Removed defined(USE_X11) for ozone-wayland port
#elif defined(USE_AURA)
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableNevaIme))
    return std::make_unique<InputMethodAuraLinuxNeva>(delegate, widget);
  else
    return std::make_unique<InputMethodAuraLinux>(delegate);
#else
  return std::make_unique<InputMethodMinimal>(delegate);
#endif
}

void SetUpInputMethodFactoryForTesting() {
  if (g_input_method_set_for_testing)
    return;

  CHECK(!g_create_input_method_called)
      << "ui::SetUpInputMethodFactoryForTesting was called after use of "
      << "ui::CreateInputMethod.  You must call "
      << "ui::SetUpInputMethodFactoryForTesting earlier.";

  g_input_method_set_for_testing = true;
}

void SetUpInputMethodForTesting(InputMethod* input_method) {
  g_input_method_for_testing = input_method;
}

}  // namespace ui
