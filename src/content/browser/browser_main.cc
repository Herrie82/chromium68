// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/browser_main.h"

#include <memory>

#include "base/trace_event/trace_event.h"
#include "content/browser/browser_main_runner_impl.h"
#include "content/browser/browser_process_sub_thread.h"
#include "content/common/content_constants_internal.h"

#if defined(USE_MEMORY_TRACE)
#include "base/trace_event/neva/memory_trace/memory_trace_manager.h"
#endif

namespace content {

namespace {

// Generates a pair of BrowserMain async events. We don't use the TRACE_EVENT0
// macro because the tracing infrastructure doesn't expect synchronous events
// around the main loop of a thread.
class ScopedBrowserMainEvent {
 public:
  ScopedBrowserMainEvent() {
    TRACE_EVENT_ASYNC_BEGIN0("startup", "BrowserMain", 0);
  }
  ~ScopedBrowserMainEvent() {
    TRACE_EVENT_ASYNC_END0("startup", "BrowserMain", 0);
  }
};

}  // namespace

// Main routine for running as the Browser process.
int BrowserMain(
    const MainFunctionParams& parameters,
    std::unique_ptr<BrowserProcessSubThread> service_manager_thread) {
  ScopedBrowserMainEvent scoped_browser_main_event;

  base::trace_event::TraceLog::GetInstance()->set_process_name("Browser");
  base::trace_event::TraceLog::GetInstance()->SetProcessSortIndex(
      kTraceEventBrowserProcessSortIndex);

  std::unique_ptr<BrowserMainRunnerImpl> main_runner(
      BrowserMainRunnerImpl::Create());

  int exit_code =
      main_runner->Initialize(parameters, std::move(service_manager_thread));
  if (exit_code >= 0)
    return exit_code;

#if defined(USE_MEMORY_TRACE)
  // Initialize MemoryTraceManager if --trace-memory-browser is on.
  base::trace_event::neva::MemoryTraceManagerDelegate
      memory_trace_manager_delegate;
  memory_trace_manager_delegate.Initialize(true /* is_browser_process */);
#endif

  exit_code = main_runner->Run();

  main_runner->Shutdown();

  return exit_code;
}

}  // namespace content
