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

#ifndef NEVA_INJECTION_COMMON_PUBLIC_RENDERER_INJECTION_LOADER_EXTENSION_H_
#define NEVA_INJECTION_COMMON_PUBLIC_RENDERER_INJECTION_LOADER_EXTENSION_H_

#include "content/common/content_export.h"
#include "injection/common/public/renderer/injection_base.h"

#include <string>

namespace extensions_v8 {

class InjectionWrapper;

class CONTENT_EXPORT InjectionLoaderExtension {
 public:
  static InjectionWrapper* Get(const std::string& name);
  static InjectionInstallFunction GetInjectionInstallFunction(
      const std::string& name);
};

}  // namespace extensions_v8

#endif  // NEVA_INJECTION_COMMON_PUBLIC_RENDERER_INJECTION_LOADER_EXTENSION_H_
