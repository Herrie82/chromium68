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

#ifndef NET_CODE_CACHE_CODE_CACHE_H_
#define NET_CODE_CACHE_CODE_CACHE_H_

#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "net/base/completion_callback.h"
#include "net/base/net_export.h"
#include "net/base/request_priority.h"

class GURL;

namespace base {
class time;
}
namespace net {

class IOBufferWithSize;

class NET_EXPORT CodeCache {
 public:
  typedef base::Callback<void(int, scoped_refptr<IOBufferWithSize>)>
      ReadCallback;
  virtual ~CodeCache() {}

  virtual void WriteMetadata(const GURL& url,
                             scoped_refptr<IOBufferWithSize> buf) = 0;

  virtual void ReadMetadata(const GURL& url,
                            const base::Time& last_modified,
                            ReadCallback& callback) = 0;
  virtual void ClearData(
      const CompletionCallback& callback = CompletionCallback()) = 0;
};

}  // namespace net

#endif  // NET_CODE_CACHE_CODE_CACHE_H_
