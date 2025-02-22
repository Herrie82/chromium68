// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/animation/compositor_scroll_offset_animation_curve.h"
#include "third_party/blink/renderer/platform/animation/timing_function.h"

#include "cc/animation/scroll_offset_animation_curve.h"
#include "cc/animation/timing_function.h"

using blink::CompositorScrollOffsetAnimationCurve;
using DurationBehavior = cc::ScrollOffsetAnimationCurve::DurationBehavior;
using EaseType = cc::CubicBezierTimingFunction::EaseType;

namespace blink {

static DurationBehavior GetDurationBehavior(
    CompositorScrollOffsetAnimationCurve::ScrollDurationBehavior
        web_duration_behavior) {
  switch (web_duration_behavior) {
    case CompositorScrollOffsetAnimationCurve::kScrollDurationDeltaBased:
      return DurationBehavior::DELTA_BASED;

    case CompositorScrollOffsetAnimationCurve::kScrollDurationConstant:
      return DurationBehavior::CONSTANT;

    case CompositorScrollOffsetAnimationCurve::kScrollDurationInverseDelta:
      return DurationBehavior::INVERSE_DELTA;
  }
  NOTREACHED();
  return DurationBehavior::DELTA_BASED;
}

static EaseType GetEaseType(
    CompositorScrollOffsetAnimationCurve::ScrollEaseType web_ease_type) {
  switch (web_ease_type) {
    case CompositorScrollOffsetAnimationCurve::EASE_IN_OUT:
      return EaseType::EASE_IN_OUT;
    case CompositorScrollOffsetAnimationCurve::EASE_OUT:
      return EaseType::EASE_OUT;
  }
  NOTREACHED();
  return EaseType::EASE_IN_OUT;
}

CompositorScrollOffsetAnimationCurve::CompositorScrollOffsetAnimationCurve(
    FloatPoint target_value,
    ScrollDurationBehavior duration_behavior,
    ScrollEaseType ease_type)
    : curve_(cc::ScrollOffsetAnimationCurve::Create(
          gfx::ScrollOffset(target_value.X(), target_value.Y()),
          cc::CubicBezierTimingFunction::CreatePreset(GetEaseType(ease_type)),
          GetDurationBehavior(duration_behavior))) {}

CompositorScrollOffsetAnimationCurve::CompositorScrollOffsetAnimationCurve(
    FloatPoint target_value,
    ScrollDurationBehavior duration_behavior)
    : curve_(cc::ScrollOffsetAnimationCurve::Create(
          gfx::ScrollOffset(target_value.X(), target_value.Y()),
          cc::CubicBezierTimingFunction::CreatePreset(
              CubicBezierTimingFunction::EaseType::EASE_IN_OUT),
          GetDurationBehavior(duration_behavior))) {}

CompositorScrollOffsetAnimationCurve::CompositorScrollOffsetAnimationCurve(
    cc::ScrollOffsetAnimationCurve* curve)
    : curve_(curve->CloneToScrollOffsetAnimationCurve()) {}

CompositorScrollOffsetAnimationCurve::~CompositorScrollOffsetAnimationCurve() =
    default;

void CompositorScrollOffsetAnimationCurve::SetInitialValue(
    FloatPoint initial_value) {
  curve_->SetInitialValue(
      gfx::ScrollOffset(initial_value.X(), initial_value.Y()));
}

FloatPoint CompositorScrollOffsetAnimationCurve::GetValue(double time) const {
  gfx::ScrollOffset value =
      curve_->GetValue(base::TimeDelta::FromSecondsD(time));
  return FloatPoint(value.x(), value.y());
}

void CompositorScrollOffsetAnimationCurve::ApplyAdjustment(IntSize adjustment) {
  curve_->ApplyAdjustment(
      gfx::Vector2dF(adjustment.Width(), adjustment.Height()));
}

double CompositorScrollOffsetAnimationCurve::Duration() const {
  return curve_->Duration().InSecondsF();
}

FloatPoint CompositorScrollOffsetAnimationCurve::TargetValue() const {
  gfx::ScrollOffset target = curve_->target_value();
  return FloatPoint(target.x(), target.y());
}

void CompositorScrollOffsetAnimationCurve::UpdateTarget(double time,
                                                        FloatPoint new_target) {
  curve_->UpdateTarget(time, gfx::ScrollOffset(new_target.X(), new_target.Y()));
}

std::unique_ptr<cc::AnimationCurve>
CompositorScrollOffsetAnimationCurve::CloneToAnimationCurve() const {
  return curve_->Clone();
}

}  // namespace blink
