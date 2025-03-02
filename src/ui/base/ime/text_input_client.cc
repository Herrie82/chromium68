// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/text_input_client.h"

namespace ui {

TextInputClient::~TextInputClient() {
}

///@name USE_NEVA_APPRUNTIME
///@{
bool TextInputClient::SystemKeyboardDisabled() const {
  return false;
}
///@}

}  // namespace ui
