/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef RTC_BASE_UNITS_UNIT_BASE_H_
#define RTC_BASE_UNITS_UNIT_BASE_H_

#include <stdint.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <assert.h>
#include "ns3/safe_conversions.h"



 // UnitBase is a base class for implementing custom value types with a specific
 // unit. It provides type safety and commonly useful operations. The underlying
 // storage is always an int64_t, it's up to the unit implementation to choose
 // what scale it represents.
 //
 // It's used like:
 // class MyUnit: public UnitBase<MyUnit> {...};
 //
 // Unit_T is the subclass representing the specific unit.
 namespace rtc {
template <class Unit_T>
class UnitBase {
public:
	UnitBase() = delete;
	static constexpr Unit_T Zero() { return Unit_T(0); }
	static constexpr Unit_T PlusInfinity() { return Unit_T(PlusInfinityVal()); }
	static constexpr Unit_T MinusInfinity() { return Unit_T(MinusInfinityVal()); }

	constexpr bool IsZero() const { return value_ == 0; }
	constexpr bool IsFinite() const { return !IsInfinite(); }
	constexpr bool IsInfinite() const {
		return value_ == PlusInfinityVal() || value_ == MinusInfinityVal();
	}
	constexpr bool IsPlusInfinity() const { return value_ == PlusInfinityVal(); }
	constexpr bool IsMinusInfinity() const {
		return value_ == MinusInfinityVal();
	}

	constexpr bool operator==(const Unit_T& other) const {
		return value_ == other.value_;
	}
	constexpr bool operator!=(const Unit_T& other) const {
		return value_ != other.value_;
	}
	constexpr bool operator<=(const Unit_T& other) const {
		return value_ <= other.value_;
	}
	constexpr bool operator>=(const Unit_T& other) const {
		return value_ >= other.value_;
	}
	constexpr bool operator>(const Unit_T& other) const {
		return value_ > other.value_;
	}
	constexpr bool operator<(const Unit_T& other) const {
		return value_ < other.value_;
	}
	constexpr Unit_T RoundTo(const Unit_T& resolution) const {
		assert(IsFinite());
		assert(resolution.IsFinite());
		assert(resolution.value_ > 0);
		return Unit_T((value_ + resolution.value_ / 2) / resolution.value_) *
			resolution.value_;
	}
	constexpr Unit_T RoundUpTo(const Unit_T& resolution) const {
		assert(IsFinite());
		assert(resolution.IsFinite());
		assert(resolution.value_ > 0);
		return Unit_T((value_ + resolution.value_ - 1) / resolution.value_) *
			resolution.value_;
	}
	constexpr Unit_T RoundDownTo(const Unit_T& resolution) const {
		assert(IsFinite());
		assert(resolution.IsFinite());
		assert(resolution.value_ > 0);
		return Unit_T(value_ / resolution.value_) * resolution.value_;
	}

protected:
	template <
		typename T,
		typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
	static constexpr Unit_T FromValue(T value) {
		if (Unit_T::one_sided)
			assert(value >= 0);
		assert(value > MinusInfinityVal());
		assert(value < PlusInfinityVal());
		return Unit_T(dchecked_cast<int64_t>(value));
	}
	template <typename T,
		typename std::enable_if<std::is_floating_point<T>::value>::type* =
		nullptr>
	static constexpr Unit_T FromValue(T value) {
		if (value == std::numeric_limits<T>::infinity()) {
			return PlusInfinity();
		}
		else if (value == -std::numeric_limits<T>::infinity()) {
			return MinusInfinity();
		}
		else {
			assert(!std::isnan(value));
			return FromValue(dchecked_cast<int64_t>(value));
		}
	}

	template <
		typename T,
		typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
	static constexpr Unit_T FromFraction(int64_t denominator, T value) {
		if (Unit_T::one_sided)
			assert(value >= 0);
		assert(value > MinusInfinityVal() / denominator);
		assert(value < PlusInfinityVal() / denominator);
		return Unit_T(dchecked_cast<int64_t>(value * denominator));
	}
	template <typename T,
		typename std::enable_if<std::is_floating_point<T>::value>::type* =
		nullptr>
	static constexpr Unit_T FromFraction(int64_t denominator, T value) {
		return FromValue(value * denominator);
	}

	template <typename T = int64_t>
	constexpr typename std::enable_if<std::is_integral<T>::value, T>::type
		ToValue() const {
		assert(IsFinite());
		return dchecked_cast<T>(value_);
	}
	template <typename T>
	constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type
		ToValue() const {
		return IsPlusInfinity()
			? std::numeric_limits<T>::infinity()
			: IsMinusInfinity() ? -std::numeric_limits<T>::infinity()
			: value_;
	}
	template <typename T>
	constexpr T ToValueOr(T fallback_value) const {
		return IsFinite() ? value_ : fallback_value;
	}

	template <int64_t Denominator, typename T = int64_t>
	constexpr typename std::enable_if<std::is_integral<T>::value, T>::type
		ToFraction() const {
		assert(IsFinite());
		if (Unit_T::one_sided) {
			return dchecked_cast<T>(
				DivRoundPositiveToNearest(value_, Denominator));
		}
		else {
			return dchecked_cast<T>(DivRoundToNearest(value_, Denominator));
		}
	}
	template <int64_t Denominator, typename T>
	constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type
		ToFraction() const {
		return ToValue<T>() * (1 / static_cast<T>(Denominator));
	}

	template <int64_t Denominator>
	constexpr int64_t ToFractionOr(int64_t fallback_value) const {
		return IsFinite() ? Unit_T::one_sided
			? DivRoundPositiveToNearest(value_, Denominator)
			: DivRoundToNearest(value_, Denominator)
			: fallback_value;
	}

	template <int64_t Factor, typename T = int64_t>
	constexpr typename std::enable_if<std::is_integral<T>::value, T>::type
		ToMultiple() const {
		assert(ToValue() >= std::numeric_limits<T>::min() / Factor);
		assert(ToValue() <= std::numeric_limits<T>::max() / Factor);
		return dchecked_cast<T>(ToValue() * Factor);
	}
	template <int64_t Factor, typename T>
	constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type
		ToMultiple() const {
		return ToValue<T>() * Factor;
	}

	explicit constexpr UnitBase(int64_t value) : value_(value) {}

private:
	template <class RelativeUnit_T>
	friend class RelativeUnit;

	static inline constexpr int64_t PlusInfinityVal() {
		return std::numeric_limits<int64_t>::max();
	}
	static inline constexpr int64_t MinusInfinityVal() {
		return std::numeric_limits<int64_t>::min();
	}

	constexpr Unit_T& AsSubClassRef() { return static_cast<Unit_T&>(*this); }
	constexpr const Unit_T& AsSubClassRef() const {
		return static_cast<const Unit_T&>(*this);
	}
	// Assumes that n >= 0 and d > 0.
	static constexpr int64_t DivRoundPositiveToNearest(int64_t n, int64_t d) {
		return (n + d / 2) / d;
	}
	// Assumes that d > 0.
	static constexpr int64_t DivRoundToNearest(int64_t n, int64_t d) {
		return (n + (n >= 0 ? d / 2 : -d / 2)) / d;
	}

	int64_t value_;
};

// Extends UnitBase to provide operations for relative units, that is, units
// that have a meaningful relation between values such that a += b is a
// sensible thing to do. For a,b <- same unit.
template <class Unit_T>
class RelativeUnit : public UnitBase<Unit_T> {
public:
	constexpr Unit_T Clamped(Unit_T min_value, Unit_T max_value) const {
		return std::max(min_value,
			std::min(UnitBase<Unit_T>::AsSubClassRef(), max_value));
	}
	constexpr void Clamp(Unit_T min_value, Unit_T max_value) {
		*this = Clamped(min_value, max_value);
	}
	constexpr Unit_T operator+(const Unit_T other) const {
		if (this->IsPlusInfinity() || other.IsPlusInfinity()) {
			assert(!this->IsMinusInfinity());
			assert(!other.IsMinusInfinity());
			return this->PlusInfinity();
		}
		else if (this->IsMinusInfinity() || other.IsMinusInfinity()) {
			assert(!this->IsPlusInfinity());
			assert(!other.IsPlusInfinity());
			return this->MinusInfinity();
		}
		return UnitBase<Unit_T>::FromValue(this->ToValue() + other.ToValue());
	}
	constexpr Unit_T operator-(const Unit_T other) const {
		if (this->IsPlusInfinity() || other.IsMinusInfinity()) {
			assert(!this->IsMinusInfinity());
			assert(!other.IsPlusInfinity());
			return this->PlusInfinity();
		}
		else if (this->IsMinusInfinity() || other.IsPlusInfinity()) {
			assert(!this->IsPlusInfinity());
			assert(!other.IsMinusInfinity());
			return this->MinusInfinity();
		}
		return UnitBase<Unit_T>::FromValue(this->ToValue() - other.ToValue());
	}
	constexpr Unit_T& operator+=(const Unit_T other) {
		*this = *this + other;
		return this->AsSubClassRef();
	}
	constexpr Unit_T& operator-=(const Unit_T other) {
		*this = *this - other;
		return this->AsSubClassRef();
	}
	constexpr double operator/(const Unit_T other) const {
		return UnitBase<Unit_T>::template ToValue<double>() /
			other.template ToValue<double>();
	}
	template <typename T>
	constexpr typename std::enable_if<std::is_arithmetic<T>::value, Unit_T>::type
		operator/(const T& scalar) const {
		return UnitBase<Unit_T>::FromValue(
			std::round(UnitBase<Unit_T>::template ToValue<int64_t>() / scalar));
	}
	constexpr Unit_T operator*(double scalar) const {
		return UnitBase<Unit_T>::FromValue(std::round(this->ToValue() * scalar));
	}
	constexpr Unit_T operator*(int64_t scalar) const {
		return UnitBase<Unit_T>::FromValue(this->ToValue() * scalar);
	}
	constexpr Unit_T operator*(int32_t scalar) const {
		return UnitBase<Unit_T>::FromValue(this->ToValue() * scalar);
	}

protected:
	using UnitBase<Unit_T>::UnitBase;
};

template <class Unit_T>
inline constexpr Unit_T operator*(double scalar, RelativeUnit<Unit_T> other) {
	return other * scalar;
}
template <class Unit_T>
inline constexpr Unit_T operator*(int64_t scalar, RelativeUnit<Unit_T> other) {
	return other * scalar;
}
template <class Unit_T>
inline constexpr Unit_T operator*(int32_t scalar, RelativeUnit<Unit_T> other) {
	return other * scalar;
}
template <class Unit_T>
inline constexpr Unit_T operator-(RelativeUnit<Unit_T> other) {
	if (other.IsPlusInfinity())
		return UnitBase<Unit_T>::MinusInfinity();
	if (other.IsMinusInfinity())
		return UnitBase<Unit_T>::PlusInfinity();
	return -1 * other;
}
} //namespace rtc

#endif  // RTC_BASE_UNITS_UNIT_BASE_H_
