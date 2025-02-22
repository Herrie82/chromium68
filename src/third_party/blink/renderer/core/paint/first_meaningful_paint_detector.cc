// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/first_meaningful_paint_detector.h"

#include "base/logging_pmlog.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_layer_tree_view.h"
#include "third_party/blink/renderer/core/css/font_face_set_document.h"
#include "third_party/blink/renderer/core/paint/paint_timing.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"

#if defined(USE_NEVA_APPRUNTIME)
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#endif

namespace blink {

namespace {

// Web fonts that laid out more than this number of characters block First
// Meaningful Paint.
const int kBlankCharactersThreshold = 200;

}  // namespace

FirstMeaningfulPaintDetector& FirstMeaningfulPaintDetector::From(
    Document& document) {
  return PaintTiming::From(document).GetFirstMeaningfulPaintDetector();
}

FirstMeaningfulPaintDetector::FirstMeaningfulPaintDetector(
    PaintTiming* paint_timing,
    Document& document)
    : paint_timing_(paint_timing),
      network0_quiet_timer_(
          document.GetTaskRunner(TaskType::kInternalDefault),
          this,
          &FirstMeaningfulPaintDetector::Network0QuietTimerFired),
      network2_quiet_timer_(
          document.GetTaskRunner(TaskType::kInternalDefault),
          this,
          &FirstMeaningfulPaintDetector::Network2QuietTimerFired) {}

Document* FirstMeaningfulPaintDetector::GetDocument() {
  return paint_timing_->GetSupplementable();
}

// Computes "layout significance" (http://goo.gl/rytlPL) of a layout operation.
// Significance of a layout is the number of layout objects newly added to the
// layout tree, weighted by page height (before and after the layout).
// A paint after the most significance layout during page load is reported as
// First Meaningful Paint.
void FirstMeaningfulPaintDetector::MarkNextPaintAsMeaningfulIfNeeded(
    const LayoutObjectCounter& counter,
    int contents_height_before_layout,
    int contents_height_after_layout,
    int visible_height) {
  if (network0_quiet_reached_ && network2_quiet_reached_)
    return;

  unsigned delta = counter.Count() - prev_layout_object_count_;
  prev_layout_object_count_ = counter.Count();

  if (visible_height == 0)
    return;

  double ratio_before = std::max(
      1.0, static_cast<double>(contents_height_before_layout) / visible_height);
  double ratio_after = std::max(
      1.0, static_cast<double>(contents_height_after_layout) / visible_height);
  double significance = delta / ((ratio_before + ratio_after) / 2);

  // If the page has many blank characters, the significance value is
  // accumulated until the text become visible.
  int approximate_blank_character_count =
      FontFaceSetDocument::ApproximateBlankCharacterCount(*GetDocument());
  if (approximate_blank_character_count > kBlankCharactersThreshold) {
    accumulated_significance_while_having_blank_text_ += significance;
  } else {
    significance += accumulated_significance_while_having_blank_text_;
    accumulated_significance_while_having_blank_text_ = 0;
    if (significance > max_significance_so_far_) {
#if defined(USE_NEVA_APPRUNTIME)
      PMLOG_DEBUG(
          FMP, "%s %s (%p) detects NextPaintIsMeaningful as significance(%f)",
          __FILE__, __func__, this, significance);
#endif
      next_paint_is_meaningful_ = true;
      max_significance_so_far_ = significance;
    }
  }
}

void FirstMeaningfulPaintDetector::NotifyPaint() {
#if defined(USE_NEVA_APPRUNTIME)
  if (GetDocument() && !GetDocument()->HasDeferredBackgroundImages() &&
      defer_first_meaningful_paint_ == kDeferBackgroundImagesWait) {
    defer_first_meaningful_paint_ = kDoNotDefer;
    next_paint_is_meaningful_ = true;
    PMLOG_DEBUG(FMP, "%s %s (%p) NoDeferredBackgroundImages anymore", __FILE__,
                __func__, this);
  }
#endif
  if (!next_paint_is_meaningful_)
    return;

  // Skip document background-only paints.
  if (paint_timing_->FirstPaintRendered().is_null())
    return;
  provisional_first_meaningful_paint_ = CurrentTimeTicks();
  next_paint_is_meaningful_ = false;

  if (network2_quiet_reached_)
    return;

  had_user_input_before_provisional_first_meaningful_paint_ = had_user_input_;
  provisional_first_meaningful_paint_swap_ = TimeTicks();
#if defined(USE_NEVA_APPRUNTIME)
  if (GetDocument() && GetDocument()->GetSettings() &&
      GetDocument()->GetSettings()->NotifyFMPDirectly()) {
    had_user_input_before_provisional_first_meaningful_paint_ = had_user_input_;
    provisional_first_meaningful_paint_swap_ =
        provisional_first_meaningful_paint_;
    first_meaningful_paint2_quiet_ = provisional_first_meaningful_paint_swap_;
    network2_quiet_reached_ = true;
    SetFirstMeaningfulPaint(first_meaningful_paint2_quiet_,
                            provisional_first_meaningful_paint_swap_);
    return;
  }

  PMLOG_DEBUG(FMP, "%s %s (%p) registers notify swap time", __FILE__, __func__,
              this);
#endif
  RegisterNotifySwapTime(PaintEvent::kProvisionalFirstMeaningfulPaint);
}

// This is called only on FirstMeaningfulPaintDetector for main frame.
void FirstMeaningfulPaintDetector::NotifyInputEvent() {
  // Ignore user inputs before first paint.
  if (paint_timing_->FirstPaintRendered().is_null())
    return;
  had_user_input_ = kHadUserInput;
}

int FirstMeaningfulPaintDetector::ActiveConnections() {
  DCHECK(GetDocument());
  ResourceFetcher* fetcher = GetDocument()->Fetcher();
  return fetcher->BlockingRequestCount() + fetcher->NonblockingRequestCount();
}

// This function is called when the number of active connections is decreased
// and when the document is parsed.
void FirstMeaningfulPaintDetector::CheckNetworkStable() {
  DCHECK(GetDocument());
  if (!GetDocument()->HasFinishedParsing())
    return;

  SetNetworkQuietTimers(ActiveConnections());
}

void FirstMeaningfulPaintDetector::SetNetworkQuietTimers(
    int active_connections) {
  if (!network2_quiet_reached_ && active_connections <= 2) {
    // If activeConnections < 2 and the timer is already running, current
    // 2-quiet window continues; the timer shouldn't be restarted.
    if (active_connections == 2 || !network2_quiet_timer_.IsActive()) {
      double timeout = kNetwork2QuietWindowSeconds;
      Settings* settings = GetDocument()->GetSettings();
      if (settings && settings->NetworkStableTimeout() > 0)
        timeout = settings->NetworkStableTimeout();
      network2_quiet_timer_.StartOneShot(timeout, FROM_HERE);
    }
  }
  if (!network0_quiet_reached_ && active_connections == 0) {
    // This restarts 0-quiet timer if it's already running.
    network0_quiet_timer_.StartOneShot(kNetwork0QuietWindowSeconds, FROM_HERE);
  }
}

void FirstMeaningfulPaintDetector::Network0QuietTimerFired(TimerBase*) {
  if (!GetDocument() || network0_quiet_reached_ || ActiveConnections() > 0 ||
      paint_timing_->FirstContentfulPaintRendered().is_null())
    return;
  network0_quiet_reached_ = true;

  if (!provisional_first_meaningful_paint_.is_null()) {
    // Enforce FirstContentfulPaint <= FirstMeaningfulPaint.
    first_meaningful_paint0_quiet_ =
        std::max(provisional_first_meaningful_paint_,
                 paint_timing_->FirstContentfulPaintRendered());
  }
  ReportHistograms();
}

void FirstMeaningfulPaintDetector::Network2QuietTimerFired(TimerBase*) {
#if defined(USE_NEVA_APPRUNTIME)
  if (!GetDocument() || network2_quiet_reached_ || ActiveConnections() > 2)
    return;
  if (paint_timing_->FirstContentfulPaintRendered().is_null()) {
    // notify paint when First Meaningful Paint was not detected until loading resources
    NotifyNonFirstMeaningfulPaint();
    return;
  }
#else
  if (!GetDocument() || network2_quiet_reached_ || ActiveConnections() > 2 ||
      paint_timing_->FirstContentfulPaintRendered().is_null())
    return;
#endif
  network2_quiet_reached_ = true;

  if (!provisional_first_meaningful_paint_.is_null()) {
    TimeTicks first_meaningful_paint2_quiet_swap;
    // Enforce FirstContentfulPaint <= FirstMeaningfulPaint.
    if (provisional_first_meaningful_paint_ <
        paint_timing_->FirstContentfulPaintRendered()) {
      first_meaningful_paint2_quiet_ =
          paint_timing_->FirstContentfulPaintRendered();
      first_meaningful_paint2_quiet_swap =
          paint_timing_->FirstContentfulPaint();
      // It's possible that this timer fires between when the first contentful
      // paint is set and its SwapPromise is fulfilled. If this happens, defer
      // until NotifyFirstContentfulPaint() is called.
      if (first_meaningful_paint2_quiet_swap.is_null())
        defer_first_meaningful_paint_ = kDeferFirstContentfulPaintNotSet;
    } else {
      first_meaningful_paint2_quiet_ = provisional_first_meaningful_paint_;
      first_meaningful_paint2_quiet_swap =
          provisional_first_meaningful_paint_swap_;
      // We might still be waiting for one or more swap promises, in which case
      // we want to defer reporting first meaningful paint until they complete.
      // Otherwise, we would either report the wrong swap timestamp or none at
      // all.
      if (outstanding_swap_promise_count_ > 0)
        defer_first_meaningful_paint_ = kDeferOutstandingSwapPromises;
    }
    if (defer_first_meaningful_paint_ == kDoNotDefer) {
      // Report FirstMeaningfulPaint when the page reached network 2-quiet if
      // we aren't waiting for a swap timestamp.
      SetFirstMeaningfulPaint(first_meaningful_paint2_quiet_,
                              first_meaningful_paint2_quiet_swap);
    }
  }
#if defined(USE_NEVA_APPRUNTIME)
  else {
    // notify paint when First Meaningful Paint was not detected until loading resources
    NotifyNonFirstMeaningfulPaint();
  }
#endif
  ReportHistograms();
}

void FirstMeaningfulPaintDetector::ReportHistograms() {
  // This enum backs an UMA histogram, and should be treated as append-only.
  enum HadNetworkQuiet {
    kHadNetwork0Quiet,
    kHadNetwork2Quiet,
    kHadNetworkQuietEnumMax
  };
  DEFINE_STATIC_LOCAL(EnumerationHistogram, had_network_quiet_histogram,
                      ("PageLoad.Internal.Renderer."
                       "FirstMeaningfulPaintDetector.HadNetworkQuiet",
                       kHadNetworkQuietEnumMax));

  // This enum backs an UMA histogram, and should be treated as append-only.
  enum FMPOrderingEnum {
    kFMP0QuietFirst,
    kFMP2QuietFirst,
    kFMP0QuietEqualFMP2Quiet,
    kFMPOrderingEnumMax
  };
  DEFINE_STATIC_LOCAL(
      EnumerationHistogram, first_meaningful_paint_ordering_histogram,
      ("PageLoad.Internal.Renderer.FirstMeaningfulPaintDetector."
       "FirstMeaningfulPaintOrdering",
       kFMPOrderingEnumMax));

  if (!first_meaningful_paint0_quiet_.is_null() &&
      !first_meaningful_paint2_quiet_.is_null()) {
    int sample;
    if (first_meaningful_paint2_quiet_ < first_meaningful_paint0_quiet_) {
      sample = kFMP0QuietFirst;
    } else if (first_meaningful_paint2_quiet_ >
               first_meaningful_paint0_quiet_) {
      sample = kFMP2QuietFirst;
    } else {
      sample = kFMP0QuietEqualFMP2Quiet;
    }
    first_meaningful_paint_ordering_histogram.Count(sample);
  } else if (!first_meaningful_paint0_quiet_.is_null()) {
    had_network_quiet_histogram.Count(kHadNetwork0Quiet);
  } else if (!first_meaningful_paint2_quiet_.is_null()) {
    had_network_quiet_histogram.Count(kHadNetwork2Quiet);
  }
}

void FirstMeaningfulPaintDetector::RegisterNotifySwapTime(PaintEvent event) {
  ++outstanding_swap_promise_count_;
  paint_timing_->RegisterNotifySwapTime(
      event, CrossThreadBind(&FirstMeaningfulPaintDetector::ReportSwapTime,
                             WrapCrossThreadWeakPersistent(this), event));
}

void FirstMeaningfulPaintDetector::ReportSwapTime(
    PaintEvent event,
    WebLayerTreeView::SwapResult result,
    base::TimeTicks timestamp) {
  DCHECK(event == PaintEvent::kProvisionalFirstMeaningfulPaint);
  DCHECK_GT(outstanding_swap_promise_count_, 0U);
  --outstanding_swap_promise_count_;

  // If the swap fails for any reason, we use the timestamp when the SwapPromise
  // was broken. |result| == WebLayerTreeView::SwapResult::kDidNotSwapSwapFails
  // usually means the compositor decided not swap because there was no actual
  // damage, which can happen when what's being painted isn't visible. In this
  // case, the timestamp will be consistent with the case where the swap
  // succeeds, as they both capture the time up to swap. In other failure cases
  // (aborts during commit), this timestamp is an improvement over the blink
  // paint time, but does not capture some time we're interested in, e.g.  image
  // decoding.
  //
  // TODO(crbug.com/738235): Consider not reporting any timestamp when failing
  // for reasons other than kDidNotSwapSwapFails.
  paint_timing_->ReportSwapResultHistogram(result);
  provisional_first_meaningful_paint_swap_ = timestamp;

#if defined(USE_NEVA_APPRUNTIME)
  PMLOG_DEBUG(
      FMP, "%s %s (%p) reports provisional_first_meaningful_paint_swap_ [%f]",
      __FILE__, __func__, this,
      TimeTicksInSeconds(provisional_first_meaningful_paint_swap_));
#endif
  probe::paintTiming(GetDocument(), "firstMeaningfulPaintCandidate",
                     TimeTicksInSeconds(timestamp));

  // Ignore the first meaningful paint candidate as this generally is the first
  // contentful paint itself.
  if (!seen_first_meaningful_paint_candidate_) {
    seen_first_meaningful_paint_candidate_ = true;
  } else {
    paint_timing_->SetFirstMeaningfulPaintCandidate(
        provisional_first_meaningful_paint_swap_);
  }

#if defined(USE_NEVA_APPRUNTIME)
  PMLOG_DEBUG(FMP, "%s %s (%p) seen_first_meaningful_paint_candidate_[%s] ",
              __FILE__, __func__, this,
              seen_first_meaningful_paint_candidate_ ? "true" : "false");
  if (seen_first_meaningful_paint_candidate_) {
    if (GetDocument() && GetDocument()->HasDeferredBackgroundImages()) {
      defer_first_meaningful_paint_ = kDeferBackgroundImagesWait;
      PMLOG_DEBUG(FMP, "%s %s (%p) document has deferredbackgroundimages",
                  __FILE__, __func__, this);
      return;
    }
    first_meaningful_paint2_quiet_ = CurrentTimeTicks();
    network2_quiet_reached_ = true;
    SetFirstMeaningfulPaint(first_meaningful_paint2_quiet_,
                            provisional_first_meaningful_paint_swap_);
    return;
  }
#endif

  if (defer_first_meaningful_paint_ == kDeferOutstandingSwapPromises &&
      outstanding_swap_promise_count_ == 0) {
    DCHECK(!first_meaningful_paint2_quiet_.is_null());
    SetFirstMeaningfulPaint(first_meaningful_paint2_quiet_,
                            provisional_first_meaningful_paint_swap_);
  }
}

void FirstMeaningfulPaintDetector::NotifyFirstContentfulPaint(
    TimeTicks swap_stamp) {
  if (defer_first_meaningful_paint_ != kDeferFirstContentfulPaintNotSet)
    return;
  SetFirstMeaningfulPaint(first_meaningful_paint2_quiet_, swap_stamp);
}

void FirstMeaningfulPaintDetector::SetFirstMeaningfulPaint(
    TimeTicks stamp,
    TimeTicks swap_stamp) {
  DCHECK(paint_timing_->FirstMeaningfulPaint().is_null());
  DCHECK_GE(swap_stamp, stamp);
  DCHECK(!swap_stamp.is_null());
  DCHECK(network2_quiet_reached_);

  double swap_time_seconds = TimeTicksInSeconds(swap_stamp);
  probe::paintTiming(GetDocument(), "firstMeaningfulPaint", swap_time_seconds);

  // If there's only been one contentful paint, then there won't have been
  // a meaningful paint signalled to the Scheduler, so mark one now.
  // This is a no-op if a FMPC has already been marked.
  paint_timing_->SetFirstMeaningfulPaintCandidate(swap_stamp);

#if defined(USE_NEVA_APPRUNTIME)
  PMLOG_DEBUG(FMP,
              "%s %s (%p) sets firstMeaningfulPaint swap_time_seconds [%f]",
              __FILE__, __func__, this, swap_time_seconds);
#endif
  paint_timing_->SetFirstMeaningfulPaint(
      stamp, swap_stamp,
      had_user_input_before_provisional_first_meaningful_paint_);
}

#if defined(USE_NEVA_APPRUNTIME)
void FirstMeaningfulPaintDetector::StopNetwork2QuietWindowTimer() {
  if (network2_quiet_timer_.IsActive())
    network2_quiet_timer_.Stop();
}

void FirstMeaningfulPaintDetector::ResetStateToMarkNextPaintForContainer() {
  PMLOG_DEBUG(FMP, "%s %s (%p) resets states for nextFMP", __FILE__, __func__,
              this);
  next_paint_is_meaningful_ = false;
  had_user_input_ = kNoUserInput;
  had_user_input_before_provisional_first_meaningful_paint_ = kNoUserInput;
  max_significance_so_far_ = 0.0;
  provisional_first_meaningful_paint_ = TimeTicks::Now();
  provisional_first_meaningful_paint_swap_ = TimeTicks::Now();
  accumulated_significance_while_having_blank_text_ = 0.0;
  prev_layout_object_count_ = 0;
  seen_first_meaningful_paint_candidate_ = false;
  outstanding_swap_promise_count_ = 0;
  defer_first_meaningful_paint_ = kDoNotDefer;
}

void FirstMeaningfulPaintDetector::NotifyNonFirstMeaningfulPaint() {
  DCHECK(GetDocument());

  if (GetDocument() && GetDocument()->Loader()) {
    PMLOG_DEBUG(FMP, "%s %s (%p) commits non firstmeaningfulpaint after load",
                __FILE__, __func__, this);
    GetDocument()->Loader()->CommitNonFirstMeaningfulPaintAfterLoad();
  }
}
#endif

void FirstMeaningfulPaintDetector::Trace(blink::Visitor* visitor) {
  visitor->Trace(paint_timing_);
}

}  // namespace blink
