/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All
 * rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_ENGINE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_STYLE_ENGINE_H_

#include <memory>
#include <utility>
#include "base/auto_reset.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/css/active_style_sheets.h"
#include "third_party/blink/renderer/core/css/css_global_rule_set.h"
#include "third_party/blink/renderer/core/css/document_style_sheet_collection.h"
#include "third_party/blink/renderer/core/css/invalidation/pending_invalidations.h"
#include "third_party/blink/renderer/core/css/invalidation/style_invalidator.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_stats.h"
#include "third_party/blink/renderer/core/css/style_engine_context.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/tree_ordered_list.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/fonts/font_selector_client.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class CSSFontSelector;
class CSSStyleSheet;
class FontSelector;
class MediaQueryEvaluator;
class Node;
class RuleFeatureSet;
class ShadowTreeStyleSheetCollection;
class StyleRuleFontFace;
class StyleRuleUsageTracker;
class StyleSheetContents;
class ViewportStyleResolver;

enum InvalidationScope { kInvalidateCurrentScope, kInvalidateAllScopes };

using StyleSheetKey = AtomicString;

// The StyleEngine class manages style-related state for the document. There is
// a 1-1 relationship of Document to StyleEngine. The document calls the
// StyleEngine when the the document is updated in a way that impacts styles.
class CORE_EXPORT StyleEngine final
    : public GarbageCollectedFinalized<StyleEngine>,
      public FontSelectorClient,
      public TraceWrapperBase {
  USING_GARBAGE_COLLECTED_MIXIN(StyleEngine);

 public:
  class IgnoringPendingStylesheet {
    DISALLOW_NEW();

   public:
    IgnoringPendingStylesheet(StyleEngine& engine)
        : scope_(&engine.ignore_pending_stylesheets_, true) {}

   private:
    base::AutoReset<bool> scope_;
  };

  friend class IgnoringPendingStylesheet;

  static StyleEngine* Create(Document& document) {
    return new StyleEngine(document);
  }

  ~StyleEngine() override;

  const HeapVector<TraceWrapperMember<StyleSheet>>&
  StyleSheetsForStyleSheetList(TreeScope&);

#if defined(USE_NEVA_APPRUNTIME)
  const HeapVector<
      std::pair<StyleSheetKey, TraceWrapperMember<CSSStyleSheet>>>&
  InjectedAuthorStyleSheets();
#else
  const HeapVector<
      std::pair<StyleSheetKey, TraceWrapperMember<CSSStyleSheet>>>&
  InjectedAuthorStyleSheets() const {
    return injected_author_style_sheets_;
  }
#endif

  CSSStyleSheet* InspectorStyleSheet() const { return inspector_style_sheet_; }

  const ActiveStyleSheetVector ActiveStyleSheetsForInspector();

  bool NeedsActiveStyleUpdate() const;
  void SetNeedsActiveStyleUpdate(TreeScope&);
  void AddStyleSheetCandidateNode(Node&);
  void RemoveStyleSheetCandidateNode(Node&, ContainerNode& insertion_point);
  void ModifiedStyleSheetCandidateNode(Node&);
  void MoreStyleSheetsWillChange(TreeScope&,
                                 StyleSheetList* old_sheets,
                                 StyleSheetList* new_sheets);
  void MediaQueriesChangedInScope(TreeScope&);
  void WatchedSelectorsChanged();
  void InitialStyleChanged();
  void InitialViewportChanged();
  void ViewportRulesChanged();
  void HtmlImportAddedOrRemoved();
  void V0ShadowAddedOnV1Document();

  void InjectSheet(const StyleSheetKey&, StyleSheetContents*,
                   WebDocument::CSSOrigin =
                       WebDocument::kAuthorOrigin);
  void RemoveInjectedSheet(const StyleSheetKey&,
                           WebDocument::CSSOrigin =
                               WebDocument::kAuthorOrigin);

#if defined(USE_NEVA_APPRUNTIME)
  void InvalidateInjectedStyleSheetCache();
  void UpdateInjectedStyleSheetCache();
  void CompatibilityModeChanged();
#endif

  CSSStyleSheet& EnsureInspectorStyleSheet();
  RuleSet* WatchedSelectorsRuleSet() {
    DCHECK(IsMaster());
    DCHECK(global_rule_set_);
    return global_rule_set_->WatchedSelectorsRuleSet();
  }
  bool HasStyleSheets() const {
    return GetDocumentStyleSheetCollection().HasStyleSheets();
  }

  RuleSet* RuleSetForSheet(CSSStyleSheet&);
  void MediaQueryAffectingValueChanged();
  void UpdateActiveStyleSheetsInImport(
      StyleEngine& master_engine,
      DocumentStyleSheetCollector& parent_collector);
  void UpdateActiveStyle();
  void MarkAllTreeScopesDirty() { all_tree_scopes_dirty_ = true; }

  String PreferredStylesheetSetName() const {
    return preferred_stylesheet_set_name_;
  }
  void SetPreferredStylesheetSetNameIfNotSet(const String&);
  void SetHttpDefaultStyle(const String&);

  void AddPendingSheet(StyleEngineContext&);
  void RemovePendingSheet(Node& style_sheet_candidate_node,
                          const StyleEngineContext&);

  bool HasPendingScriptBlockingSheets() const {
    return pending_script_blocking_stylesheets_ > 0;
  }
  bool HasPendingRenderBlockingSheets() const {
    return pending_render_blocking_stylesheets_ > 0;
  }
  bool HaveScriptBlockingStylesheetsLoaded() const {
    return !HasPendingScriptBlockingSheets();
  }
  bool HaveRenderBlockingStylesheetsLoaded() const {
    return !HasPendingRenderBlockingSheets();
  }
  bool IgnoringPendingStylesheets() const {
    return ignore_pending_stylesheets_;
  }

  unsigned MaxDirectAdjacentSelectors() const {
    return GetRuleFeatureSet().MaxDirectAdjacentSelectors();
  }
  bool UsesFirstLineRules() const {
    return GetRuleFeatureSet().UsesFirstLineRules();
  }
  bool UsesWindowInactiveSelector() const {
    return GetRuleFeatureSet().UsesWindowInactiveSelector();
  }

  bool UsesRemUnits() const { return uses_rem_units_; }
  void SetUsesRemUnit(bool uses_rem_units) { uses_rem_units_ = uses_rem_units; }
  bool UpdateRemUnits(const ComputedStyle* old_root_style,
                      const ComputedStyle* new_root_style);

  void ResetCSSFeatureFlags(const RuleFeatureSet&);

  void ShadowRootRemovedFromDocument(ShadowRoot*);
  void AddTreeBoundaryCrossingScope(const TreeScope&);
  const TreeOrderedList& TreeBoundaryCrossingScopes() const {
    return tree_boundary_crossing_scopes_;
  }
  void ResetAuthorStyle(TreeScope&);

  StyleResolver* Resolver() const { return resolver_; }

  void SetRuleUsageTracker(StyleRuleUsageTracker*);

  StyleResolver& EnsureResolver() {
    UpdateActiveStyle();
    if (!resolver_)
      CreateResolver();
    return *resolver_;
  }

  bool HasResolver() const { return resolver_; }

  PendingInvalidations& GetPendingNodeInvalidations() {
    return pending_invalidations_;
  }
  // Push all pending invalidations on the document.
  void InvalidateStyle();
  bool MediaQueryAffectedByViewportChange();
  bool MediaQueryAffectedByDeviceChange();
  bool HasViewportDependentMediaQueries() {
    DCHECK(IsMaster());
    DCHECK(global_rule_set_);
    UpdateActiveStyle();
    return !global_rule_set_->GetRuleFeatureSet()
                .ViewportDependentMediaQueryResults()
                .IsEmpty();
  }

  CSSFontSelector* GetFontSelector() { return font_selector_; }
  void SetFontSelector(CSSFontSelector*);

  void RemoveFontFaceRules(const HeapVector<Member<const StyleRuleFontFace>>&);
  // updateGenericFontFamilySettings is used from WebSettingsImpl.
  void UpdateGenericFontFamilySettings();

  void DidDetach();

  CSSStyleSheet* CreateSheet(Element&,
                             const String& text,
                             TextPosition start_position,
                             StyleEngineContext&);

  void CollectFeaturesTo(RuleFeatureSet& features) const {
    CollectUserStyleFeaturesTo(features);
    CollectScopedStyleFeaturesTo(features);
  }

  void EnsureUAStyleForFullscreen();
  void EnsureUAStyleForElement(const Element&);

  void PlatformColorsChanged();

  bool HasRulesForId(const AtomicString& id) const;
  void ClassChangedForElement(const SpaceSplitString& changed_classes,
                              Element&);
  void ClassChangedForElement(const SpaceSplitString& old_classes,
                              const SpaceSplitString& new_classes,
                              Element&);
  void AttributeChangedForElement(const QualifiedName& attribute_name,
                                  Element&);
  void IdChangedForElement(const AtomicString& old_id,
                           const AtomicString& new_id,
                           Element&);
  void PseudoStateChangedForElement(CSSSelector::PseudoType, Element&);
  void ScheduleSiblingInvalidationsForElement(Element&,
                                              ContainerNode& scheduling_parent,
                                              unsigned min_direct_adjacent);
  void ScheduleInvalidationsForInsertedSibling(Element* before_element,
                                               Element& inserted_element);
  void ScheduleInvalidationsForRemovedSibling(Element* before_element,
                                              Element& removed_element,
                                              Element& after_element);
  void ScheduleNthPseudoInvalidations(ContainerNode&);
  void ScheduleInvalidationsForRuleSets(TreeScope&,
                                        const HeapHashSet<Member<RuleSet>>&,
                                        InvalidationScope =
                                            kInvalidateCurrentScope);

  void NodeWillBeRemoved(Node&);

  unsigned StyleForElementCount() const { return style_for_element_count_; }
  void IncStyleForElementCount() { style_for_element_count_++; }

  StyleResolverStats* Stats() { return style_resolver_stats_.get(); }
  void SetStatsEnabled(bool);

  void ApplyRuleSetChanges(TreeScope&,
                           const ActiveStyleSheetVector& old_style_sheets,
                           const ActiveStyleSheetVector& new_style_sheets,
                           InvalidationScope = kInvalidateCurrentScope);

  void CollectMatchingUserRules(ElementRuleCollector&) const;

  void CustomPropertyRegistered();

  bool NeedsWhitespaceReattachment() const {
    return !whitespace_reattach_set_.IsEmpty();
  }
  bool NeedsWhitespaceReattachment(Element* element) const {
    return whitespace_reattach_set_.Contains(element);
  }
  void ClearWhitespaceReattachSet() { whitespace_reattach_set_.clear(); }
  void MarkForWhitespaceReattachment();

  StyleRuleKeyframes* KeyframeStylesForAnimation(
      const AtomicString& animation_name);

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override { return "StyleEngine"; }

 private:
  // FontSelectorClient implementation.
  void FontsNeedUpdate(FontSelector*) override;

 private:
  StyleEngine(Document&);
  bool NeedsActiveStyleSheetUpdate() const {
    return all_tree_scopes_dirty_ || tree_scopes_removed_ ||
           document_scope_dirty_ || dirty_tree_scopes_.size() ||
           user_style_dirty_;
  }

  TreeScopeStyleSheetCollection& EnsureStyleSheetCollectionFor(TreeScope&);
  TreeScopeStyleSheetCollection* StyleSheetCollectionFor(TreeScope&);
  bool ShouldUpdateDocumentStyleSheetCollection() const;
  bool ShouldUpdateShadowTreeStyleSheetCollection() const;

  void MarkDocumentDirty();
  void MarkTreeScopeDirty(TreeScope&);
  void MarkUserStyleDirty();

  bool IsMaster() const { return is_master_; }
  Document* Master();
  Document& GetDocument() const { return *document_; }

  typedef HeapHashSet<Member<TreeScope>> UnorderedTreeScopeSet;

  void MediaQueryAffectingValueChanged(UnorderedTreeScopeSet&);
  const RuleFeatureSet& GetRuleFeatureSet() const {
    DCHECK(IsMaster());
    DCHECK(global_rule_set_);
    return global_rule_set_->GetRuleFeatureSet();
  }

  void CreateResolver();
  void ClearResolvers();

  void CollectUserStyleFeaturesTo(RuleFeatureSet&) const;
  void CollectScopedStyleFeaturesTo(RuleFeatureSet&) const;

  CSSStyleSheet* ParseSheet(Element&,
                            const String& text,
                            TextPosition start_position);

  const DocumentStyleSheetCollection& GetDocumentStyleSheetCollection() const {
    DCHECK(document_style_sheet_collection_);
    return *document_style_sheet_collection_;
  }

  DocumentStyleSheetCollection& GetDocumentStyleSheetCollection() {
    DCHECK(document_style_sheet_collection_);
    return *document_style_sheet_collection_;
  }

  void UpdateActiveStyleSheetsInShadow(
      TreeScope*,
      UnorderedTreeScopeSet& tree_scopes_removed);

  bool ShouldSkipInvalidationFor(const Element&) const;
  void ScheduleRuleSetInvalidationsForElement(
      Element&,
      const HeapHashSet<Member<RuleSet>>&);
  void ScheduleTypeRuleSetInvalidations(ContainerNode&,
                                        const HeapHashSet<Member<RuleSet>>&);
  void InvalidateSlottedElements(HTMLSlotElement&);

  void UpdateViewport();
  void UpdateActiveUserStyleSheets();
  void UpdateActiveStyleSheets();
  void UpdateGlobalRuleSet() {
    DCHECK(!NeedsActiveStyleSheetUpdate());
    if (global_rule_set_)
      global_rule_set_->Update(GetDocument());
  }
  const MediaQueryEvaluator& EnsureMediaQueryEvaluator();
  void UpdateStyleSheetList(TreeScope&);

  void ClearFontCache();
  void RefreshFontCache();
  void MarkFontCacheDirty() { font_cache_dirty_ = true; }
  bool IsFontCacheDirty() const { return font_cache_dirty_; }

  void ClearKeyframeRules() { keyframes_rule_map_.clear(); }

  void AddFontFaceRules(const RuleSet&);
  void AddKeyframeRules(const RuleSet&);
  void AddKeyframeStyle(StyleRuleKeyframes*);

  Member<Document> document_;
  bool is_master_;

  // Track the number of currently loading top-level stylesheets needed for
  // layout.  Sheets loaded using the @import directive are not included in this
  // count.  We use this count of pending sheets to detect when we can begin
  // attaching elements and when it is safe to execute scripts.
  int pending_script_blocking_stylesheets_ = 0;
  int pending_render_blocking_stylesheets_ = 0;
  int pending_body_stylesheets_ = 0;

  Member<CSSStyleSheet> inspector_style_sheet_;

  TraceWrapperMember<DocumentStyleSheetCollection>
      document_style_sheet_collection_;

  Member<StyleRuleUsageTracker> tracker_;

  using StyleSheetCollectionMap =
      HeapHashMap<WeakMember<TreeScope>,
                  Member<ShadowTreeStyleSheetCollection>>;
  StyleSheetCollectionMap style_sheet_collection_map_;

  bool document_scope_dirty_ = true;
  bool all_tree_scopes_dirty_ = false;
  bool tree_scopes_removed_ = false;
  bool user_style_dirty_ = false;
  UnorderedTreeScopeSet dirty_tree_scopes_;
  UnorderedTreeScopeSet active_tree_scopes_;
  TreeOrderedList tree_boundary_crossing_scopes_;

  String preferred_stylesheet_set_name_;

  bool uses_rem_units_ = false;
  bool ignore_pending_stylesheets_ = false;

  Member<StyleResolver> resolver_;
  Member<ViewportStyleResolver> viewport_resolver_;
  Member<MediaQueryEvaluator> media_query_evaluator_;
  Member<CSSGlobalRuleSet> global_rule_set_;
  PendingInvalidations pending_invalidations_;

  // This is a set of rendered elements which had one or more of its rendered
  // children removed since the last lifecycle update. For such elements we need
  // to re-attach whitespace children. Also see reattach_all_whitespace_nodes_
  // in the WhitespaceAttacher class.
  HeapHashSet<Member<Element>> whitespace_reattach_set_;

  Member<CSSFontSelector> font_selector_;
  bool font_cache_dirty_ = false;

  HeapHashMap<AtomicString, WeakMember<StyleSheetContents>>
      text_to_sheet_cache_;
  HeapHashMap<WeakMember<StyleSheetContents>, AtomicString>
      sheet_to_text_cache_;

  std::unique_ptr<StyleResolverStats> style_resolver_stats_;
  unsigned style_for_element_count_ = 0;

  HeapVector<std::pair<StyleSheetKey, TraceWrapperMember<CSSStyleSheet>>>
      injected_user_style_sheets_;
  HeapVector<std::pair<StyleSheetKey, TraceWrapperMember<CSSStyleSheet>>>
      injected_author_style_sheets_;

  ActiveStyleSheetVector active_user_style_sheets_;

  using KeyframesRuleMap =
      HeapHashMap<AtomicString, Member<StyleRuleKeyframes>>;
  KeyframesRuleMap keyframes_rule_map_;

#if defined(USE_NEVA_APPRUNTIME)
  bool injected_style_sheet_cache_valid_;
#endif

  friend class StyleEngineTest;
};

}  // namespace blink

#endif
