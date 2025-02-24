// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#ifndef CONTENT_PUBLIC_BROWSER_RUNTIME_DELEGATE_WEBOS_H_
#define CONTENT_PUBLIC_BROWSER_RUNTIME_DELEGATE_WEBOS_H_

#include "content/common/content_export.h"

namespace content {

class RuntimeDelegateWebOS;

// Setter and getter for the runtime delegate for WebOS. The delegate should be
// set before creating BrowserAccessibilityManager.
CONTENT_EXPORT void SetRuntimeDelegateWebOS(RuntimeDelegateWebOS* delegate);
RuntimeDelegateWebOS* GetRuntimeDelegateWebOS();

class CONTENT_EXPORT RuntimeDelegateWebOS {
 public:
  virtual ~RuntimeDelegateWebOS() {}

  virtual bool IsForegroundAppEnyo() = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_RUNTIME_DELEGATE_WEBOS_H_
