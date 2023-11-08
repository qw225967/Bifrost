#pragma once

#include <string>
#include <type_traits>
#include "unit_base.h"


// DataSize is a class represeting a count of bytes.
namespace rtc {
class DataSize final : public RelativeUnit<DataSize> {
public:
	template <typename T>
	static constexpr DataSize Bytes(T value) {
		static_assert(std::is_arithmetic<T>::value, "");
		return FromValue(value);
	}
	static constexpr DataSize Infinity() { return PlusInfinity(); }

	DataSize() = delete;

	template <typename T = int64_t>
	constexpr T bytes() const {
		return ToValue<T>();
	}

	constexpr int64_t bytes_or(int64_t fallback_value) const {
		return ToValueOr(fallback_value);
	}

private:
	friend class UnitBase<DataSize>;
	using RelativeUnit::RelativeUnit;
	static constexpr bool one_sided = true;
};
} //namespace rtc
