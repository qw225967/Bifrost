/*
 *  Copyright 2014 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

 // Borrowed from Chromium's src/base/numerics/safe_conversions.h.

#ifndef RTC_BASE_NUMERICS_SAFE_CONVERSIONS_H_
#define RTC_BASE_NUMERICS_SAFE_CONVERSIONS_H_

#include <limits>

#include "safe_conversions_impl.h"

	// Convenience function that returns true if the supplied value is in range
	// for the destination type.
template <typename Dst, typename Src>
inline constexpr bool IsValueInRangeForNumericType(Src value) {
	return RangeCheck<Dst>(value) == TYPE_VALID;
}

// checked_cast<> and dchecked_cast<> are analogous to static_cast<> for
// numeric types, except that they [D]CHECK that the specified numeric
// conversion will not overflow or underflow. NaN source will always trigger
// the [D]CHECK.
template <typename Dst, typename Src>
inline constexpr Dst checked_cast(Src value) {
	assert(IsValueInRangeForNumericType<Dst>(value));
	return static_cast<Dst>(value);
}
template <typename Dst, typename Src>
inline constexpr Dst dchecked_cast(Src value) {
	assert(IsValueInRangeForNumericType<Dst>(value));
	return static_cast<Dst>(value);
}

// saturated_cast<> is analogous to static_cast<> for numeric types, except
// that the specified numeric conversion will saturate rather than overflow or
// underflow. NaN assignment to an integral will trigger a RTC_CHECK condition.
template <typename Dst, typename Src>
inline constexpr Dst saturated_cast(Src value) {
	// Optimization for floating point values, which already saturate.
	if (std::numeric_limits<Dst>::is_iec559)
		return static_cast<Dst>(value);

	switch (RangeCheck<Dst>(value)) {
	case TYPE_VALID:
		return static_cast<Dst>(value);

	case TYPE_UNDERFLOW:
		return std::numeric_limits<Dst>::min();

	case TYPE_OVERFLOW:
		return std::numeric_limits<Dst>::max();		
	}
	//这个地方理论上应该不能到达，但为了编译能通过（需要返回一个值）因此只能出此下策
	return std::numeric_limits<Dst>::infinity();
}


#endif  // RTC_BASE_NUMERICS_SAFE_CONVERSIONS_H_
