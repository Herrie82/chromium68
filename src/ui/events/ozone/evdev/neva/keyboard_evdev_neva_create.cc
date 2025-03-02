// Copyright (c) 2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "ui/events/ozone/evdev/neva/keyboard_evdev_neva.h"

namespace ui {

KeyboardEvdevNeva::KeyboardEvdevNeva(
    EventModifiers* modifiers,
    KeyboardLayoutEngine* keyboard_layout_engine,
    const EventDispatchCallback& callback)
    : KeyboardEvdev(modifiers, keyboard_layout_engine, callback) {
#if defined(OS_WEBOS)
  SetAutoRepeatEnabled(false);
#endif
}

std::unique_ptr<KeyboardEvdevNeva>
KeyboardEvdevNeva::Create(EventModifiers* modifiers,
                          KeyboardLayoutEngine* keyboard_layout_engine,
                          const EventDispatchCallback& callback) {

  return std::make_unique<KeyboardEvdevNeva>(modifiers,
                                             keyboard_layout_engine,
                                             callback);
}

} // namespace ui
