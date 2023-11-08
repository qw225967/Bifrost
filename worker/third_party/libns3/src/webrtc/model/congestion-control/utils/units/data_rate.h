#pragma once

/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <limits>
#include <string>
#include <type_traits>
#include <iostream>

#include "data_size.h"
#include "time_delta.h"
#include "unit_base.h"
#include "frequency.h"


 // RtcDataRate is a class that represents a given data rate. This can be used to
 // represent bandwidth, encoding bitrate, etc. The internal storage is bits per
 // second (bps).
using namespace rtc;
class RtcDataRate final : public RelativeUnit<RtcDataRate> {
public:
	template <typename T>
	static constexpr RtcDataRate BitsPerSec(T value) {
		static_assert(std::is_arithmetic<T>::value, "");
		return FromValue(value);
	}
	template <typename T>
	static constexpr RtcDataRate BytesPerSec(T value) {
		static_assert(std::is_arithmetic<T>::value, "");
		return FromFraction(8, value);
	}
	template <typename T>
	static constexpr RtcDataRate KilobitsPerSec(T value) {
		static_assert(std::is_arithmetic<T>::value, "");
		return FromFraction(1000, value);
	}
	static constexpr RtcDataRate Infinity() { return PlusInfinity(); }

	RtcDataRate() = delete;

	template <typename T = int64_t>
	constexpr T bps() const {
		return ToValue<T>();
	}
	template <typename T = int64_t>
	constexpr T bytes_per_sec() const {
		return ToFraction<8, T>();
	}
	template <typename T = int64_t>
	constexpr T kbps() const {
		return ToFraction<1000, T>();
	}
	constexpr int64_t bps_or(int64_t fallback_value) const {
		return ToValueOr(fallback_value);
	}
	constexpr int64_t kbps_or(int64_t fallback_value) const {
		return ToFractionOr<1000>(fallback_value);
	}

private:
	// Bits per second used internally to simplify debugging by making the value
	// more recognizable.
	friend class UnitBase<RtcDataRate>;
	using RelativeUnit::RelativeUnit;
	static constexpr bool one_sided = true;
};

namespace data_rate_impl {
	inline constexpr int64_t Microbits(const DataSize& size) {
		constexpr int64_t kMaxBeforeConversion =
			std::numeric_limits<int64_t>::max() / 8000000;
		assert(size.bytes() <= kMaxBeforeConversion);
		return size.bytes() * 8000000;
	}

	inline constexpr int64_t MillibytePerSec(const RtcDataRate& size) {
		constexpr int64_t kMaxBeforeConversion =
			std::numeric_limits<int64_t>::max() / (1000 / 8);
		assert(size.bps() <= kMaxBeforeConversion);			
		return size.bps() * (1000 / 8);
	}
}  // namespace data_rate_impl

inline constexpr RtcDataRate operator/(const DataSize size,
	const TimeDelta duration) {
	return RtcDataRate::BitsPerSec(data_rate_impl::Microbits(size) / duration.us());
}
inline constexpr TimeDelta operator/(const DataSize size, const RtcDataRate rate) {
	return TimeDelta::Micros(data_rate_impl::Microbits(size) / rate.bps());
}
inline constexpr DataSize operator*(const RtcDataRate rate,
	const TimeDelta duration) {
	int64_t microbits = rate.bps() * duration.us();
	return DataSize::Bytes((microbits + 4000000) / 8000000);
}
inline constexpr DataSize operator*(const TimeDelta duration,
	const RtcDataRate rate) {
	return rate * duration;
}

//inline constexpr DataSize operator/(const RtcDataRate rate,
//    const Frequency frequency) {
//    int64_t millihertz = frequency.millihertz<int64_t>();
//    // Note that the value is truncated here reather than rounded, potentially
//    // introducing an error of .5 bytes if rounding were expected.
//    return DataSize::Bytes(data_rate_impl::MillibytePerSec(rate) / millihertz);
//}
inline constexpr Frequency operator/(const RtcDataRate rate, const DataSize size) {
	return Frequency::MilliHertz(data_rate_impl::MillibytePerSec(rate) /
		size.bytes());
}
inline constexpr RtcDataRate operator*(const DataSize size,
	const Frequency frequency) {
	assert(frequency.IsZero() ||
		size.bytes() <= std::numeric_limits<int64_t>::max() / 8 /
		frequency.millihertz<int64_t>());
	int64_t millibits_per_second =
		size.bytes() * 8 * frequency.millihertz<int64_t>();
	return RtcDataRate::BitsPerSec((millibits_per_second + 500) / 1000);
}
inline constexpr RtcDataRate operator*(const Frequency frequency,
	const DataSize size) {
	return size * frequency;
}


