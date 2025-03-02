// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/shaping/shaping_line_breaker.h"

#include <unicode/uscript.h>
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/fonts/font.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/fonts/font_test_utilities.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_inline_headers.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_test_info.h"
#include "third_party/blink/renderer/platform/text/text_break_iterator.h"
#include "third_party/blink/renderer/platform/text/text_run.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

namespace {

scoped_refptr<ShapeResult> ShapeLine(ShapingLineBreaker* breaker,
                              unsigned start_offset,
                              LayoutUnit available_space,
                              unsigned* break_offset) {
  ShapingLineBreaker::Result result;
  scoped_refptr<ShapeResult> shape_result =
      breaker->ShapeLine(start_offset, available_space, &result);
  *break_offset = result.break_offset;
  return shape_result;
}

}  // namespace

class ShapingLineBreakerTest : public testing::Test {
 protected:
  void SetUp() override {
    font_description.SetComputedSize(12.0);
    font = Font(font_description);
    font.Update(nullptr);
  }

  void TearDown() override {}

  // Compute all break positions by |NextBreakOpportunity|.
  Vector<unsigned> BreakPositionsByNext(const ShapingLineBreaker& breaker,
                                        const String& string) {
    Vector<unsigned> break_positions;
    for (unsigned i = 0; i <= string.length(); i++) {
      unsigned next = breaker.NextBreakOpportunity(i, 0).offset;
      if (break_positions.IsEmpty() || break_positions.back() != next)
        break_positions.push_back(next);
    }
    return break_positions;
  }

  // Compute all break positions by |PreviousBreakOpportunity|.
  Vector<unsigned> BreakPositionsByPrevious(const ShapingLineBreaker& breaker,
                                            const String& string) {
    Vector<unsigned> break_positions;
    for (unsigned i = string.length(); i; i--) {
      unsigned previous = breaker.PreviousBreakOpportunity(i, 0).offset;
      if (previous &&
          (break_positions.IsEmpty() || break_positions.back() != previous))
        break_positions.push_back(previous);
    }
    break_positions.Reverse();
    return break_positions;
  }

  FontCachePurgePreventer font_cache_purge_preventer;
  FontDescription font_description;
  Font font;
  unsigned start_index = 0;
  unsigned num_glyphs = 0;
  hb_script_t script = HB_SCRIPT_INVALID;
};

TEST_F(ShapingLineBreakerTest, ShapeLineLatin) {
  String string = To16Bit(
      "Test run with multiple words and breaking "
      "opportunities.",
      56);
  LazyLineBreakIterator break_iterator(string, "en-US", LineBreakType::kNormal);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 56);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  // "Test run with multiple"
  scoped_refptr<ShapeResult> first4 = shaper.Shape(&font, direction, 0, 22);
  ASSERT_LT(first4->SnappedWidth(), result->SnappedWidth());

  // "Test run with"
  scoped_refptr<ShapeResult> first3 = shaper.Shape(&font, direction, 0, 13);
  ASSERT_LT(first3->SnappedWidth(), first4->SnappedWidth());

  // "Test run"
  scoped_refptr<ShapeResult> first2 = shaper.Shape(&font, direction, 0, 8);
  ASSERT_LT(first2->SnappedWidth(), first3->SnappedWidth());

  // "Test"
  scoped_refptr<ShapeResult> first1 = shaper.Shape(&font, direction, 0, 4);
  ASSERT_LT(first1->SnappedWidth(), first2->SnappedWidth());

  ShapingLineBreaker breaker(&shaper, &font, result.get(), &break_iterator);
  scoped_refptr<ShapeResult> line;
  unsigned break_offset = 0;

  // Test the case where the entire string fits.
  line = ShapeLine(&breaker, 0, result->SnappedWidth(), &break_offset);
  EXPECT_EQ(56u, break_offset);  // After the end of the string.
  EXPECT_EQ(result->SnappedWidth(), line->SnappedWidth());

  // Test cases where we break between words.
  line = ShapeLine(&breaker, 0, first4->SnappedWidth(), &break_offset);
  EXPECT_EQ(22u, break_offset);  // Between "multiple" and " words"
  EXPECT_EQ(first4->SnappedWidth(), line->SnappedWidth());

  line = ShapeLine(&breaker, 0, first4->SnappedWidth() + 10, &break_offset);
  EXPECT_EQ(22u, break_offset);  // Between "multiple" and " words"
  EXPECT_EQ(first4->SnappedWidth(), line->SnappedWidth());

  line = ShapeLine(&breaker, 0, first4->SnappedWidth() - 1, &break_offset);
  EXPECT_EQ(13u, break_offset);  // Between "width" and "multiple"
  EXPECT_EQ(first3->SnappedWidth(), line->SnappedWidth());

  line = ShapeLine(&breaker, 0, first3->SnappedWidth(), &break_offset);
  EXPECT_EQ(13u, break_offset);  // Between "width" and "multiple"
  EXPECT_EQ(first3->SnappedWidth(), line->SnappedWidth());

  line = ShapeLine(&breaker, 0, first3->SnappedWidth() - 1, &break_offset);
  EXPECT_EQ(8u, break_offset);  // Between "run" and "width"
  EXPECT_EQ(first2->SnappedWidth(), line->SnappedWidth());

  line = ShapeLine(&breaker, 0, first2->SnappedWidth(), &break_offset);
  EXPECT_EQ(8u, break_offset);  // Between "run" and "width"
  EXPECT_EQ(first2->SnappedWidth(), line->SnappedWidth());

  line = ShapeLine(&breaker, 0, first2->SnappedWidth() - 1, &break_offset);
  EXPECT_EQ(4u, break_offset);  // Between "Test" and "run"
  EXPECT_EQ(first1->SnappedWidth(), line->SnappedWidth());

  line = ShapeLine(&breaker, 0, first1->SnappedWidth(), &break_offset);
  EXPECT_EQ(4u, break_offset);  // Between "Test" and "run"
  EXPECT_EQ(first1->SnappedWidth(), line->SnappedWidth());

  // Test the case where we cannot break earlier.
  line = ShapeLine(&breaker, 0, first1->SnappedWidth() - 1, &break_offset);
  EXPECT_EQ(4u, break_offset);  // Between "Test" and "run"
  EXPECT_EQ(first1->SnappedWidth(), line->SnappedWidth());
}

TEST_F(ShapingLineBreakerTest, ShapeLineLatinMultiLine) {
  String string = To16Bit("Line breaking test case.", 24);
  LazyLineBreakIterator break_iterator(string, "en-US", LineBreakType::kNormal);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 24);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);
  scoped_refptr<ShapeResult> first = shaper.Shape(&font, direction, 0, 4);
  scoped_refptr<ShapeResult> mid_third = shaper.Shape(&font, direction, 0, 16);

  ShapingLineBreaker breaker(&shaper, &font, result.get(), &break_iterator);
  unsigned break_offset = 0;

  ShapeLine(&breaker, 0, result->SnappedWidth() - 1, &break_offset);
  EXPECT_EQ(18u, break_offset);

  ShapeLine(&breaker, 0, first->SnappedWidth(), &break_offset);
  EXPECT_EQ(4u, break_offset);

  ShapeLine(&breaker, 0, mid_third->SnappedWidth(), &break_offset);
  EXPECT_EQ(13u, break_offset);

  ShapeLine(&breaker, 13u, mid_third->SnappedWidth(), &break_offset);
  EXPECT_EQ(24u, break_offset);
}

TEST_F(ShapingLineBreakerTest, ShapeLineLatinBreakAll) {
  String string = To16Bit("Testing break type-break all.", 29);
  LazyLineBreakIterator break_iterator(string, "en-US",
                                       LineBreakType::kBreakAll);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), 29);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);
  scoped_refptr<ShapeResult> midpoint = shaper.Shape(&font, direction, 0, 16);

  ShapingLineBreaker breaker(&shaper, &font, result.get(), &break_iterator);
  scoped_refptr<ShapeResult> line;
  unsigned break_offset = 0;

  line = ShapeLine(&breaker, 0, midpoint->SnappedWidth(), &break_offset);
  EXPECT_EQ(16u, break_offset);
  EXPECT_EQ(midpoint->SnappedWidth(), line->SnappedWidth());

  line = ShapeLine(&breaker, 16u, result->SnappedWidth(), &break_offset);
  EXPECT_EQ(29u, break_offset);
  EXPECT_GE(midpoint->SnappedWidth(), line->SnappedWidth());
}

TEST_F(ShapingLineBreakerTest, ShapeLineZeroAvailableWidth) {
  String string(u"Testing overflow line break.");
  LazyLineBreakIterator break_iterator(string, "en-US", LineBreakType::kNormal);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);

  ShapingLineBreaker breaker(&shaper, &font, result.get(), &break_iterator);
  scoped_refptr<ShapeResult> line;
  unsigned break_offset = 0;
  LayoutUnit zero(0);

  line = ShapeLine(&breaker, 0, zero, &break_offset);
  EXPECT_EQ(7u, break_offset);

  line = ShapeLine(&breaker, 7, zero, &break_offset);
  EXPECT_EQ(16u, break_offset);

  line = ShapeLine(&breaker, 16, zero, &break_offset);
  EXPECT_EQ(21u, break_offset);

  line = ShapeLine(&breaker, 21, zero, &break_offset);
  EXPECT_EQ(28u, break_offset);
}

// Hits DCHECK at end of ShapingLineBreaker::ShapeLine, not clear if the test
// is correct. Disabling for now.
TEST_F(ShapingLineBreakerTest, DISABLED_ShapeLineArabicThaiHanLatin) {
  UChar mixed_string[] = {0x628, 0x20,   0x64A, 0x629, 0x20,
                          0xE20, 0x65E5, 0x62,  0};
  LazyLineBreakIterator break_iterator(mixed_string, "ar_AE",
                                       LineBreakType::kNormal);
  TextDirection direction = TextDirection::kRtl;

  HarfBuzzShaper shaper(mixed_string, 8);
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction);
  scoped_refptr<ShapeResult> words[] = {shaper.Shape(&font, direction, 0, 1),
                                 shaper.Shape(&font, direction, 2, 4),
                                 shaper.Shape(&font, direction, 5, 6),
                                 shaper.Shape(&font, direction, 6, 7),
                                 shaper.Shape(&font, direction, 7, 8)};
  auto* const longest_word =
      std::max_element(std::begin(words), std::end(words),
                       [](const scoped_refptr<ShapeResult>& a,
                          const scoped_refptr<ShapeResult>& b) {
                         return a->SnappedWidth() < b->SnappedWidth();
                       });
  LayoutUnit longest_word_width = (*longest_word)->SnappedWidth();

  ShapingLineBreaker breaker(&shaper, &font, result.get(), &break_iterator);
  scoped_refptr<ShapeResult> line;
  unsigned break_offset = 0;

  ShapeLine(&breaker, 0, longest_word_width, &break_offset);
  EXPECT_EQ(1u, break_offset);

  ShapeLine(&breaker, 1, longest_word_width, &break_offset);
  EXPECT_EQ(4u, break_offset);

  ShapeLine(&breaker, 4, longest_word_width, &break_offset);
  EXPECT_EQ(6u, break_offset);

  ShapeLine(&breaker, 6, longest_word_width, &break_offset);
  EXPECT_EQ(7u, break_offset);

  ShapeLine(&breaker, 7, longest_word_width, &break_offset);
  EXPECT_EQ(8u, break_offset);
}

TEST_F(ShapingLineBreakerTest, ShapeLineRangeEndMidWord) {
  String string(u"Mid word");
  LazyLineBreakIterator break_iterator(string, "en-US", LineBreakType::kNormal);
  TextDirection direction = TextDirection::kLtr;

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, direction, 0, 2);

  ShapingLineBreaker breaker(&shaper, &font, result.get(), &break_iterator);
  scoped_refptr<ShapeResult> line;
  unsigned break_offset = 0;

  line = ShapeLine(&breaker, 0, LayoutUnit::Max(), &break_offset);
  EXPECT_EQ(2u, break_offset);
  EXPECT_EQ(result->Width(), line->Width());
}

struct BreakOpportunityTestData {
  const char16_t* string;
  Vector<unsigned> break_positions;
  Vector<unsigned> break_positions_with_soft_hyphen_disabled;
};

class BreakOpportunityTest
    : public ShapingLineBreakerTest,
      public testing::WithParamInterface<BreakOpportunityTestData> {};

INSTANTIATE_TEST_CASE_P(
    ShapingLineBreakerTest,
    BreakOpportunityTest,
    testing::Values(BreakOpportunityTestData{u"x y z", {1, 3, 5}},
                    BreakOpportunityTestData{u"y\xADz", {2, 3}, {3}},
                    BreakOpportunityTestData{u"\xADz", {1, 2}, {2}},
                    BreakOpportunityTestData{u"y\xAD", {2}, {2}},
                    BreakOpportunityTestData{u"\xAD\xADz", {2, 3}, {3}}));

TEST_P(BreakOpportunityTest, Next) {
  const BreakOpportunityTestData& data = GetParam();
  String string(data.string);
  LazyLineBreakIterator break_iterator(string);

  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  ShapingLineBreaker breaker(&shaper, &font, result.get(), &break_iterator);
  EXPECT_THAT(BreakPositionsByNext(breaker, string),
              testing::ElementsAreArray(data.break_positions));

  if (!data.break_positions_with_soft_hyphen_disabled.IsEmpty()) {
    breaker.DisableSoftHyphen();
    EXPECT_THAT(BreakPositionsByNext(breaker, string),
                testing::ElementsAreArray(
                    data.break_positions_with_soft_hyphen_disabled));
  }
}

TEST_P(BreakOpportunityTest, Previous) {
  const BreakOpportunityTestData& data = GetParam();
  String string(data.string);
  LazyLineBreakIterator break_iterator(string);
  HarfBuzzShaper shaper(string.Characters16(), string.length());
  scoped_refptr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);

  ShapingLineBreaker breaker(&shaper, &font, result.get(), &break_iterator);
  EXPECT_THAT(BreakPositionsByPrevious(breaker, string),
              testing::ElementsAreArray(data.break_positions));

  if (!data.break_positions_with_soft_hyphen_disabled.IsEmpty()) {
    breaker.DisableSoftHyphen();
    EXPECT_THAT(BreakPositionsByPrevious(breaker, string),
                testing::ElementsAreArray(
                    data.break_positions_with_soft_hyphen_disabled));
  }
}

}  // namespace blink
