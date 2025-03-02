// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/animation_host.h"

#include <algorithm>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "cc/animation/animation.h"
#include "cc/animation/animation_delegate.h"
#include "cc/animation/animation_events.h"
#include "cc/animation/animation_id_provider.h"
#include "cc/animation/animation_timeline.h"
#include "cc/animation/element_animations.h"
#include "cc/animation/keyframe_effect.h"
#include "cc/animation/scroll_offset_animation_curve.h"
#include "cc/animation/scroll_offset_animations.h"
#include "cc/animation/scroll_offset_animations_impl.h"
#include "cc/animation/scroll_timeline.h"
#include "cc/animation/timing_function.h"
#include "cc/animation/worklet_animation.h"
#include "ui/gfx/geometry/box_f.h"
#include "ui/gfx/geometry/scroll_offset.h"

namespace cc {

namespace {
WorkletAnimation* ToWorkletAnimation(Animation* animation) {
  DCHECK(animation->IsWorkletAnimation());
  return static_cast<WorkletAnimation*>(animation);
}
}  // namespace

std::unique_ptr<AnimationHost> AnimationHost::CreateMainInstance() {
  return base::WrapUnique(new AnimationHost(ThreadInstance::MAIN));
}

std::unique_ptr<AnimationHost> AnimationHost::CreateForTesting(
    ThreadInstance thread_instance) {
  auto animation_host = base::WrapUnique(new AnimationHost(thread_instance));

  if (thread_instance == ThreadInstance::IMPL)
    animation_host->SetSupportsScrollAnimations(true);

  return animation_host;
}

AnimationHost::AnimationHost(ThreadInstance thread_instance)
    : mutator_host_client_(nullptr),
      thread_instance_(thread_instance),
      supports_scroll_animations_(false),
      needs_push_properties_(false),
      mutator_(nullptr) {
  if (thread_instance_ == ThreadInstance::IMPL) {
    scroll_offset_animations_impl_ =
        std::make_unique<ScrollOffsetAnimationsImpl>(this);
  } else {
    scroll_offset_animations_ = std::make_unique<ScrollOffsetAnimations>(this);
  }
}

AnimationHost::~AnimationHost() {
  scroll_offset_animations_impl_ = nullptr;

  ClearMutators();
  DCHECK(!mutator_host_client());
  DCHECK(element_to_animations_map_.empty());
}

std::unique_ptr<MutatorHost> AnimationHost::CreateImplInstance(
    bool supports_impl_scrolling) const {
  DCHECK_EQ(thread_instance_, ThreadInstance::MAIN);

  auto mutator_host_impl =
      base::WrapUnique<MutatorHost>(new AnimationHost(ThreadInstance::IMPL));
  mutator_host_impl->SetSupportsScrollAnimations(supports_impl_scrolling);
  return mutator_host_impl;
}

AnimationTimeline* AnimationHost::GetTimelineById(int timeline_id) const {
  auto f = id_to_timeline_map_.find(timeline_id);
  return f == id_to_timeline_map_.end() ? nullptr : f->second.get();
}

void AnimationHost::ClearMutators() {
  for (auto& kv : id_to_timeline_map_)
    EraseTimeline(kv.second);
  id_to_timeline_map_.clear();
}

void AnimationHost::EraseTimeline(scoped_refptr<AnimationTimeline> timeline) {
  timeline->ClearAnimations();
  timeline->SetAnimationHost(nullptr);
}

void AnimationHost::AddAnimationTimeline(
    scoped_refptr<AnimationTimeline> timeline) {
  DCHECK(timeline->id());
  timeline->SetAnimationHost(this);
  id_to_timeline_map_.insert(
      std::make_pair(timeline->id(), std::move(timeline)));
  SetNeedsPushProperties();
}

void AnimationHost::RemoveAnimationTimeline(
    scoped_refptr<AnimationTimeline> timeline) {
  DCHECK(timeline->id());
  EraseTimeline(timeline);
  id_to_timeline_map_.erase(timeline->id());
  SetNeedsPushProperties();
}

void AnimationHost::RegisterElement(ElementId element_id,
                                    ElementListType list_type) {
  scoped_refptr<ElementAnimations> element_animations =
      GetElementAnimationsForElementId(element_id);
  if (element_animations)
    element_animations->ElementRegistered(element_id, list_type);
}

void AnimationHost::UnregisterElement(ElementId element_id,
                                      ElementListType list_type) {
  scoped_refptr<ElementAnimations> element_animations =
      GetElementAnimationsForElementId(element_id);
  if (element_animations)
    element_animations->ElementUnregistered(element_id, list_type);
}

void AnimationHost::RegisterKeyframeEffectForElement(
    ElementId element_id,
    KeyframeEffect* keyframe_effect) {
  DCHECK(element_id);
  DCHECK(keyframe_effect);

  scoped_refptr<ElementAnimations> element_animations =
      GetElementAnimationsForElementId(element_id);
  if (!element_animations) {
    element_animations = ElementAnimations::Create();
    element_animations->SetElementId(element_id);
    element_to_animations_map_[element_animations->element_id()] =
        element_animations;
  }

  if (element_animations->animation_host() != this) {
    element_animations->SetAnimationHost(this);
    element_animations->InitAffectedElementTypes();
  }

  element_animations->AddKeyframeEffect(keyframe_effect);
}

void AnimationHost::UnregisterKeyframeEffectForElement(
    ElementId element_id,
    KeyframeEffect* keyframe_effect) {
  DCHECK(element_id);
  DCHECK(keyframe_effect);

  scoped_refptr<ElementAnimations> element_animations =
      GetElementAnimationsForElementId(element_id);
  DCHECK(element_animations);
  element_animations->RemoveKeyframeEffect(keyframe_effect);

  if (element_animations->IsEmpty()) {
    element_animations->ClearAffectedElementTypes();
    element_to_animations_map_.erase(element_animations->element_id());
    element_animations->SetAnimationHost(nullptr);
  }
}

void AnimationHost::SetMutatorHostClient(MutatorHostClient* client) {
  if (mutator_host_client_ == client)
    return;

  mutator_host_client_ = client;
}

void AnimationHost::SetNeedsCommit() {
  DCHECK(mutator_host_client_);
  mutator_host_client_->SetMutatorsNeedCommit();
  // TODO(loyso): Invalidate property trees only if really needed.
  mutator_host_client_->SetMutatorsNeedRebuildPropertyTrees();
}

void AnimationHost::SetNeedsPushProperties() {
  needs_push_properties_ = true;
}

void AnimationHost::PushPropertiesTo(MutatorHost* mutator_host_impl) {
  auto* host_impl = static_cast<AnimationHost*>(mutator_host_impl);

  if (needs_push_properties_) {
    needs_push_properties_ = false;
    PushTimelinesToImplThread(host_impl);
    RemoveTimelinesFromImplThread(host_impl);
    PushPropertiesToImplThread(host_impl);
    // This is redundant but used in tests.
    host_impl->needs_push_properties_ = false;
  }
}

void AnimationHost::PushTimelinesToImplThread(AnimationHost* host_impl) const {
  for (auto& kv : id_to_timeline_map_) {
    auto& timeline = kv.second;
    AnimationTimeline* timeline_impl =
        host_impl->GetTimelineById(timeline->id());
    if (timeline_impl)
      continue;

    scoped_refptr<AnimationTimeline> to_add = timeline->CreateImplInstance();
    host_impl->AddAnimationTimeline(to_add.get());
  }
}

void AnimationHost::RemoveTimelinesFromImplThread(
    AnimationHost* host_impl) const {
  IdToTimelineMap& timelines_impl = host_impl->id_to_timeline_map_;

  // Erase all the impl timelines which |this| doesn't have.
  for (auto it = timelines_impl.begin(); it != timelines_impl.end();) {
    auto& timeline_impl = it->second;
    if (timeline_impl->is_impl_only() || GetTimelineById(timeline_impl->id())) {
      ++it;
    } else {
      host_impl->EraseTimeline(it->second);
      it = timelines_impl.erase(it);
    }
  }
}

void AnimationHost::PushPropertiesToImplThread(AnimationHost* host_impl) {
  // Sync all animations with impl thread to create ElementAnimations. This
  // needs to happen before the element animations are synced below.
  for (auto& kv : id_to_timeline_map_) {
    AnimationTimeline* timeline = kv.second.get();
    if (AnimationTimeline* timeline_impl =
            host_impl->GetTimelineById(timeline->id())) {
      timeline->PushPropertiesTo(timeline_impl);
    }
  }

  // Sync properties for created ElementAnimations.
  for (auto& kv : element_to_animations_map_) {
    const auto& element_animations = kv.second;
    if (auto element_animations_impl =
            host_impl->GetElementAnimationsForElementId(kv.first)) {
      element_animations->PushPropertiesTo(std::move(element_animations_impl));
    }
  }

  // Update the impl-only scroll offset animations.
  scroll_offset_animations_->PushPropertiesTo(
      host_impl->scroll_offset_animations_impl_.get());
  host_impl->main_thread_animations_count_ = main_thread_animations_count_;
  host_impl->current_frame_had_raf_ = current_frame_had_raf_;
  host_impl->next_frame_has_pending_raf_ = next_frame_has_pending_raf_;
}

scoped_refptr<ElementAnimations>
AnimationHost::GetElementAnimationsForElementId(ElementId element_id) const {
  if (!element_id)
    return nullptr;
  auto iter = element_to_animations_map_.find(element_id);
  return iter == element_to_animations_map_.end() ? nullptr : iter->second;
}

void AnimationHost::SetSupportsScrollAnimations(
    bool supports_scroll_animations) {
  supports_scroll_animations_ = supports_scroll_animations;
}

bool AnimationHost::SupportsScrollAnimations() const {
  return supports_scroll_animations_;
}

bool AnimationHost::NeedsTickAnimations() const {
  return !ticking_animations_.empty();
}

bool AnimationHost::NeedsTickMutator(base::TimeTicks monotonic_time,
                                     const ScrollTree& scroll_tree) const {
  if (!mutator_ || !mutator_->HasAnimators())
    return false;

  for (auto& animation : ticking_animations_) {
    if (!animation->IsWorkletAnimation())
      continue;

    if (ToWorkletAnimation(animation.get())
            ->NeedsUpdate(monotonic_time, scroll_tree))
      return true;
  }

  return false;
}

bool AnimationHost::ActivateAnimations() {
  if (!NeedsTickAnimations())
    return false;

  TRACE_EVENT0("cc", "AnimationHost::ActivateAnimations");
  AnimationsList ticking_animations_copy = ticking_animations_;
  for (auto& it : ticking_animations_copy)
    it->ActivateKeyframeEffects();

  return true;
}

bool AnimationHost::TickAnimations(base::TimeTicks monotonic_time,
                                   const ScrollTree& scroll_tree) {
  TRACE_EVENT0("cc", "AnimationHost::TickAnimations");
  bool did_animate = false;

  if (NeedsTickAnimations()) {
    TRACE_EVENT_INSTANT0("cc", "NeedsTickAnimations", TRACE_EVENT_SCOPE_THREAD);
    AnimationsList ticking_animations_copy = ticking_animations_;
    for (auto& it : ticking_animations_copy)
      it->Tick(monotonic_time);

    did_animate = true;
  }
  if (NeedsTickMutator(monotonic_time, scroll_tree)) {
    // TODO(majidvp): At the moment we call this for both active and pending
    // trees similar to other animations. However our final goal is to only call
    // it once, ideally after activation, and only when the input
    // to an active timeline has changed. http://crbug.com/767210
    mutator_->Mutate(CollectAnimatorsState(monotonic_time, scroll_tree));
    did_animate = true;
  }

  return did_animate;
}

void AnimationHost::TickScrollAnimations(base::TimeTicks monotonic_time,
                                         const ScrollTree& scroll_tree) {
  // TODO(majidvp) For now the logic simply generates an update when at least
  // one animation needs updating but this is inefficient. We need a more
  // fine-grained approach based on invalidating individual ScrollTimelines and
  // then ticking the animations attached to those timelines. To make
  // this happen we probably need to move "ticking" animations to timeline.
  if (!NeedsTickMutator(monotonic_time, scroll_tree))
    return;
  DCHECK(mutator_);

  // TODO(majidvp): We need to return a boolean here so that LTHI knows
  // whether it needs to schedule another frame.
  mutator_->Mutate(CollectAnimatorsState(monotonic_time, scroll_tree));
}

std::unique_ptr<MutatorInputState> AnimationHost::CollectAnimatorsState(
    base::TimeTicks monotonic_time,
    const ScrollTree& scroll_tree) {
  TRACE_EVENT0("cc", "AnimationHost::CollectAnimatorsState");
  std::unique_ptr<MutatorInputState> result =
      std::make_unique<MutatorInputState>();

  for (auto& animation : ticking_animations_) {
    if (!animation->IsWorkletAnimation())
      continue;

    WorkletAnimation* worklet_animation = ToWorkletAnimation(animation.get());
    MutatorInputState::AnimationState state{
        worklet_animation->id(), worklet_animation->name(),
        worklet_animation->CurrentTime(monotonic_time, scroll_tree)};

    result->animations.push_back(std::move(state));
  }

  return result;
}

bool AnimationHost::UpdateAnimationState(bool start_ready_animations,
                                         MutatorEvents* mutator_events) {
  if (!NeedsTickAnimations())
    return false;

  auto* animation_events = static_cast<AnimationEvents*>(mutator_events);

  TRACE_EVENT0("cc", "AnimationHost::UpdateAnimationState");
  AnimationsList ticking_animations_copy = ticking_animations_;
  for (auto& it : ticking_animations_copy)
    it->UpdateState(start_ready_animations, animation_events);

  return true;
}

std::unique_ptr<MutatorEvents> AnimationHost::CreateEvents() {
  return std::make_unique<AnimationEvents>();
}

void AnimationHost::SetAnimationEvents(
    std::unique_ptr<MutatorEvents> mutator_events) {
  auto events =
      base::WrapUnique(static_cast<AnimationEvents*>(mutator_events.release()));

  for (size_t event_index = 0; event_index < events->events_.size();
       ++event_index) {
    ElementId element_id = events->events_[event_index].element_id;

    // Use the map of all ElementAnimations, not just ticking animations, since
    // non-ticking animations may still receive events for impl-only animations.
    const ElementToAnimationsMap& all_element_animations =
        element_to_animations_map_;
    auto iter = all_element_animations.find(element_id);
    if (iter != all_element_animations.end()) {
      switch (events->events_[event_index].type) {
        case AnimationEvent::STARTED:
          (*iter).second->NotifyAnimationStarted(events->events_[event_index]);
          break;

        case AnimationEvent::FINISHED:
          (*iter).second->NotifyAnimationFinished(events->events_[event_index]);
          break;

        case AnimationEvent::ABORTED:
          (*iter).second->NotifyAnimationAborted(events->events_[event_index]);
          break;

        case AnimationEvent::TAKEOVER:
          (*iter).second->NotifyAnimationTakeover(events->events_[event_index]);
          break;
      }
    }
  }
}

bool AnimationHost::ScrollOffsetAnimationWasInterrupted(
    ElementId element_id) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->ScrollOffsetAnimationWasInterrupted()
             : false;
}

bool AnimationHost::IsAnimatingFilterProperty(ElementId element_id,
                                              ElementListType list_type) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->IsCurrentlyAnimatingProperty(
                   TargetProperty::FILTER, list_type)
             : false;
}

bool AnimationHost::IsAnimatingOpacityProperty(
    ElementId element_id,
    ElementListType list_type) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->IsCurrentlyAnimatingProperty(
                   TargetProperty::OPACITY, list_type)
             : false;
}

bool AnimationHost::IsAnimatingTransformProperty(
    ElementId element_id,
    ElementListType list_type) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->IsCurrentlyAnimatingProperty(
                   TargetProperty::TRANSFORM, list_type)
             : false;
}

bool AnimationHost::HasPotentiallyRunningFilterAnimation(
    ElementId element_id,
    ElementListType list_type) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->IsPotentiallyAnimatingProperty(
                   TargetProperty::FILTER, list_type)
             : false;
}

bool AnimationHost::HasPotentiallyRunningOpacityAnimation(
    ElementId element_id,
    ElementListType list_type) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->IsPotentiallyAnimatingProperty(
                   TargetProperty::OPACITY, list_type)
             : false;
}

bool AnimationHost::HasPotentiallyRunningTransformAnimation(
    ElementId element_id,
    ElementListType list_type) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->IsPotentiallyAnimatingProperty(
                   TargetProperty::TRANSFORM, list_type)
             : false;
}

#if defined(USE_NEVA_APPRUNTIME)
bool AnimationHost::HasPotentiallyRunningScrollAnimation(
    ElementId element_id,
    ElementListType list_type) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->IsPotentiallyAnimatingProperty(
                   TargetProperty::SCROLL_OFFSET, list_type)
             : false;
}
#endif

bool AnimationHost::HasAnyAnimationTargetingProperty(
    ElementId element_id,
    TargetProperty::Type property) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  if (!element_animations)
    return false;

  return element_animations->HasAnyAnimationTargetingProperty(property);
}

bool AnimationHost::HasOnlyTranslationTransforms(
    ElementId element_id,
    ElementListType list_type) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->HasOnlyTranslationTransforms(list_type)
             : true;
}

bool AnimationHost::AnimationsPreserveAxisAlignment(
    ElementId element_id) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->AnimationsPreserveAxisAlignment()
             : true;
}

bool AnimationHost::MaximumTargetScale(ElementId element_id,
                                       ElementListType list_type,
                                       float* max_scale) const {
  *max_scale = 0.f;
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->MaximumTargetScale(list_type, max_scale)
             : true;
}

bool AnimationHost::AnimationStartScale(ElementId element_id,
                                        ElementListType list_type,
                                        float* start_scale) const {
  *start_scale = 0.f;
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations
             ? element_animations->AnimationStartScale(list_type, start_scale)
             : true;
}

bool AnimationHost::IsElementAnimating(ElementId element_id) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations ? element_animations->HasAnyKeyframeModel() : false;
}

bool AnimationHost::HasTickingKeyframeModelForTesting(
    ElementId element_id) const {
  auto element_animations = GetElementAnimationsForElementId(element_id);
  return element_animations ? element_animations->HasTickingKeyframeEffect()
                            : false;
}

void AnimationHost::ImplOnlyScrollAnimationCreate(
    ElementId element_id,
    const gfx::ScrollOffset& target_offset,
    const gfx::ScrollOffset& current_offset,
    base::TimeDelta delayed_by,
    base::TimeDelta animation_start_offset) {
  DCHECK(scroll_offset_animations_impl_);
  scroll_offset_animations_impl_->ScrollAnimationCreate(
      element_id, target_offset, current_offset, delayed_by,
      animation_start_offset);
}

bool AnimationHost::ImplOnlyScrollAnimationUpdateTarget(
    ElementId element_id,
    const gfx::Vector2dF& scroll_delta,
    const gfx::ScrollOffset& max_scroll_offset,
    base::TimeTicks frame_monotonic_time,
    base::TimeDelta delayed_by) {
  DCHECK(scroll_offset_animations_impl_);
  return scroll_offset_animations_impl_->ScrollAnimationUpdateTarget(
      element_id, scroll_delta, max_scroll_offset, frame_monotonic_time,
      delayed_by);
}

ScrollOffsetAnimations& AnimationHost::scroll_offset_animations() const {
  DCHECK(scroll_offset_animations_);
  return *scroll_offset_animations_.get();
}

void AnimationHost::ScrollAnimationAbort() {
  DCHECK(scroll_offset_animations_impl_);
  scroll_offset_animations_impl_->ScrollAnimationAbort(
      false /* needs_completion */);
}

void AnimationHost::AddToTicking(scoped_refptr<Animation> animation) {
  DCHECK(std::find(ticking_animations_.begin(), ticking_animations_.end(),
                   animation) == ticking_animations_.end());
  ticking_animations_.push_back(animation);
}

void AnimationHost::RemoveFromTicking(scoped_refptr<Animation> animation) {
  auto to_erase = std::find(ticking_animations_.begin(),
                            ticking_animations_.end(), animation);
  if (to_erase != ticking_animations_.end())
    ticking_animations_.erase(to_erase);
}

const AnimationHost::AnimationsList&
AnimationHost::ticking_animations_for_testing() const {
  return ticking_animations_;
}

const AnimationHost::ElementToAnimationsMap&
AnimationHost::element_animations_for_testing() const {
  return element_to_animations_map_;
}

void AnimationHost::SetLayerTreeMutator(
    std::unique_ptr<LayerTreeMutator> mutator) {
  if (mutator == mutator_)
    return;
  mutator_ = std::move(mutator);
  mutator_->SetClient(this);
}

void AnimationHost::SetMutationUpdate(
    std::unique_ptr<MutatorOutputState> output_state) {
  if (!output_state)
    return;

  TRACE_EVENT0("cc", "AnimationHost::SetMutationUpdate");
  for (auto& animation_state : output_state->animations) {
    int id = animation_state.animation_id;

    // TODO(majidvp): Use a map to make lookup O(1)
    auto to_update =
        std::find_if(ticking_animations_.begin(), ticking_animations_.end(),
                     [id](auto& it) { return it->id() == id; });

    if (to_update == ticking_animations_.end())
      continue;

    DCHECK(to_update->get()->IsWorkletAnimation());
    ToWorkletAnimation(to_update->get())
        ->SetLocalTime(animation_state.local_time);
  }
}

size_t AnimationHost::CompositedAnimationsCount() const {
  size_t composited_animations_count = 0;
  for (const auto& it : ticking_animations_)
    composited_animations_count += it->TickingKeyframeModelsCount();
  return composited_animations_count;
}

void AnimationHost::SetAnimationCounts(
    size_t total_animations_count,
    bool current_frame_had_raf,
    bool next_frame_has_pending_raf) {
  // If an animation is being run on the compositor, it will have a ticking
  // Animation (which will have a corresponding impl-thread version). Therefore
  // to find the count of main-only animations, we can simply subtract the
  // number of ticking animations from the total count.
  size_t ticking_animations_count = ticking_animations_.size();
  if (main_thread_animations_count_ !=
      total_animations_count - ticking_animations_count) {
    main_thread_animations_count_ =
        total_animations_count - ticking_animations_count;
    DCHECK_GE(main_thread_animations_count_, 0u);
    SetNeedsPushProperties();
  }
  if (current_frame_had_raf != current_frame_had_raf_) {
    current_frame_had_raf_ = current_frame_had_raf;
    SetNeedsPushProperties();
  }
  if (next_frame_has_pending_raf != next_frame_has_pending_raf_) {
    next_frame_has_pending_raf_ = next_frame_has_pending_raf;
    SetNeedsPushProperties();
  }
}

size_t AnimationHost::MainThreadAnimationsCount() const {
  return main_thread_animations_count_;
}

bool AnimationHost::CurrentFrameHadRAF() const {
  return current_frame_had_raf_;
}

bool AnimationHost::NextFrameHasPendingRAF() const {
  return next_frame_has_pending_raf_;
}

}  // namespace cc
