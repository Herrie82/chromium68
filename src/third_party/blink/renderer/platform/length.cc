/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller ( mueller@kde.org )
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2006 Andrew Wellington (proton@wiretapped.net)
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

#include "third_party/blink/renderer/platform/length.h"

#include "base/macros.h"
#include "third_party/blink/renderer/platform/animation/animation_utilities.h"
#include "third_party/blink/renderer/platform/calculation_value.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"

namespace blink {

class CalculationValueHandleMap {
  USING_FAST_MALLOC(CalculationValueHandleMap);

 public:
  CalculationValueHandleMap() : index_(1) {}

  int insert(scoped_refptr<CalculationValue> calc_value) {
    DCHECK(index_);
    // FIXME calc(): https://bugs.webkit.org/show_bug.cgi?id=80489
    // This monotonically increasing handle generation scheme is potentially
    // wasteful of the handle space. Consider reusing empty handles.

    // Mantissa has only 23bits. So many numbers above 0x1000000(16777216)
    // cannot be represented properly with accuracy of 32-bit floating point
    // numbers.
    while (map_.Contains(index_))
      index_ = (index_ + 1) % 0x1000000;

    map_.Set(index_, std::move(calc_value));

    return index_;
  }

  void Remove(int index) {
    DCHECK(map_.Contains(index));
    map_.erase(index);
  }

  CalculationValue& Get(int index) {
    DCHECK(map_.Contains(index));
    return *map_.at(index);
  }

  void DecrementRef(int index) {
    DCHECK(map_.Contains(index));
    CalculationValue* value = map_.at(index);
    if (value->HasOneRef()) {
      // Force the CalculationValue destructor early to avoid a potential
      // recursive call inside HashMap remove().
      map_.Set(index, nullptr);
      map_.erase(index);
    } else {
      value->Release();
    }
  }

 private:
  int index_;
  HashMap<int, scoped_refptr<CalculationValue>> map_;

  DISALLOW_COPY_AND_ASSIGN(CalculationValueHandleMap);
};

static CalculationValueHandleMap& CalcHandles() {
  DEFINE_STATIC_LOCAL(CalculationValueHandleMap, handle_map, ());
  return handle_map;
}

Length::Length(scoped_refptr<CalculationValue> calc)
    : quirk_(false), type_(kCalculated), is_float_(false) {
  int_value_ = CalcHandles().insert(std::move(calc));
}

Length Length::BlendMixedTypes(const Length& from,
                               double progress,
                               ValueRange range) const {
  DCHECK(from.IsSpecified());
  DCHECK(IsSpecified());
  PixelsAndPercent from_pixels_and_percent = from.GetPixelsAndPercent();
  PixelsAndPercent to_pixels_and_percent = GetPixelsAndPercent();
  const float pixels = blink::Blend(from_pixels_and_percent.pixels,
                                    to_pixels_and_percent.pixels, progress);
  const float percent = blink::Blend(from_pixels_and_percent.percent,
                                     to_pixels_and_percent.percent, progress);
  return Length(
      CalculationValue::Create(PixelsAndPercent(pixels, percent), range));
}

Length Length::BlendSameTypes(const Length& from,
                              double progress,
                              ValueRange range) const {
  LengthType result_type = GetType();
  if (IsZero())
    result_type = from.GetType();

  float blended_value = blink::Blend(from.Value(), Value(), progress);
  if (range == kValueRangeNonNegative)
    blended_value = clampTo<float>(blended_value, 0);
  return Length(blended_value, result_type);
}

PixelsAndPercent Length::GetPixelsAndPercent() const {
  switch (GetType()) {
    case kFixed:
      return PixelsAndPercent(Value(), 0);
    case kPercent:
      return PixelsAndPercent(0, Value());
    case kCalculated:
      return GetCalculationValue().GetPixelsAndPercent();
    default:
      NOTREACHED();
      return PixelsAndPercent(0, 0);
  }
}

Length Length::SubtractFromOneHundredPercent() const {
  PixelsAndPercent result = GetPixelsAndPercent();
  result.pixels = -result.pixels;
  result.percent = 100 - result.percent;
  if (result.pixels && result.percent)
    return Length(CalculationValue::Create(result, kValueRangeAll));
  if (result.percent)
    return Length(result.percent, kPercent);
  return Length(result.pixels, kFixed);
}

Length Length::Zoom(double factor) const {
  switch (GetType()) {
    case kFixed:
      return Length(GetFloatValue() * factor, kFixed);
    case kCalculated: {
      PixelsAndPercent result = GetPixelsAndPercent();
      result.pixels *= factor;
      return Length(CalculationValue::Create(
          result, GetCalculationValue().GetValueRange()));
    }
    default:
      return *this;
  }
}

CalculationValue& Length::GetCalculationValue() const {
  DCHECK(IsCalculated());
  return CalcHandles().Get(CalculationHandle());
}

void Length::IncrementCalculatedRef() const {
  DCHECK(IsCalculated());
  GetCalculationValue().AddRef();
}

void Length::DecrementCalculatedRef() const {
  DCHECK(IsCalculated());
  CalcHandles().DecrementRef(CalculationHandle());
}

float Length::NonNanCalculatedValue(LayoutUnit max_value) const {
  DCHECK(IsCalculated());
  float result = GetCalculationValue().Evaluate(max_value.ToFloat());
  if (std::isnan(result))
    return 0;
  return result;
}

bool Length::IsCalculatedEqual(const Length& o) const {
  return IsCalculated() &&
         (&GetCalculationValue() == &o.GetCalculationValue() ||
          GetCalculationValue() == o.GetCalculationValue());
}

struct SameSizeAsLength {
  int32_t value;
  int32_t meta_data;
};
static_assert(sizeof(Length) == sizeof(SameSizeAsLength),
              "length should stay small");

}  // namespace blink
