// SPDX-License-Identifier: BSD-3-Clause
/* strutils.hh - Helpers for mashing with strings */
#pragma once
#if !defined(SYCOPHANT_STRUTILS_HH)
#define SYCOPHANT_STRUTILS_HH

#include <type_traits>
#include <string>
#include <string_view>
#include <cstdint>
#include <memory>

#include <types.hh>

namespace sycophant {

	template<typename int_t>
	struct toint_t final {
	private:
		using uint_t = std::make_unsigned_t<int_t>;
		const char *const _val{nullptr};
		const size_t _len{};
		constexpr static const bool _signed = std::is_signed_v<int_t>;

		[[nodiscard]]
		constexpr static bool _is_dec(const char x) noexcept {
			return x >= '0' && x <= '9';
		}
		[[nodiscard]]
        constexpr static bool _is_hex(const char x) noexcept {
			return _is_dec(x) || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F');
		}
		[[nodiscard]]
		constexpr static bool _is_oct(const char x) noexcept {
			return x >= '0' && x <= '7';
		}


		template<bool chkfunc(const char)>
		[[nodiscard]]
		bool check() const noexcept {
			for (std::size_t i{}; i < _len; ++i) {
				if (!chkfunc(_val[i]))
                    return false;
			}
			return true;
		}
	public:

		toint_t(const char* const val) noexcept :
			_val{val}, _len{std::char_traits<char>::length(val)} { }

		toint_t(const std::string& val) noexcept :
			_val{val.data()}, _len{val.length()} { }

		toint_t(const std::string_view& val) noexcept :
			_val{val.data()}, _len{val.length()} { }

		constexpr toint_t(const char* const val, const std::size_t len) noexcept :
			_val{val}, _len{len} { }

		[[nodiscard]]
		constexpr std::size_t length() const noexcept {
			return _len;
		}

		[[nodiscard]]
		bool is_dec() const noexcept {
			if (_signed && _len > 0 && _val[0] == '-' && length() == 1) {
				return false;
			}

			for (std::size_t i{}; i < _len; ++i) {
				if (_signed && i == 0 && _val[i] == '-') {
					continue;
				} else if (!_is_dec(_val[i])) {
					return false;
				}
			}
			return true;
		}

		[[nodiscard]]
		bool is_hex() const noexcept {
			return check<_is_hex>();
		}

		[[nodiscard]]
		bool is_oct() const noexcept {
			return check<_is_oct>();
		}

		[[nodiscard]]
		int_t from_dec() const noexcept {
			return *this;
		}

		operator int_t() const noexcept {
			if (!is_dec()) {
				return {};
			}

			int_t res{};

			for (std::size_t i{}; i < _len; ++i) {
				if (_signed && i == 0 && _val[i] == '-') {
					continue;
				}
				res *= 10;
				res += _val[i] - '0';
			}

			if ((_signed) && _len > 0 && _val[0] == '-') {
				return -res;
			}
			return res;
		}

		int_t from_hex() const noexcept {
			if (!is_hex()) {
				return {};
			}

			int_t res{};

			for (std::size_t i{}; i < _len; ++i) {
				std::uint8_t hex{_val[i]};
				if (hex >= 'a' && hex <= 'f') {
					hex -= std::uint8_t(0x20U);
				}

				res <<= 4;
				hex -= std::uint8_t(0x30U);
				if (hex > 0x09U) {
					hex -= std::uint8_t(0x07U);
				}

				res += hex;
			}


			return res;
		}

		int_t from_oct() const noexcept {
			if (!is_oct()) {
				return {};
			}

			int_t res{};

			for (std::size_t i{}; i < _len; ++i) {
				res <<= 3;
				res += _val[i] - 0x30;
			}
			return res;
		}
	};


	template<
		typename int_t, typename val_t, std::size_t _len = static_cast<std::size_t>(-1),
		char padding = '0'
	>
	struct fromint_t final {
	public:
		constexpr static const auto npos{static_cast<std::size_t>(-1)};
	private:
		using uint_t   = std::make_unsigned_t<int_t>;
		using arith_t  = promoted_type_t<int_t>;
		using uarith_t = std::make_unsigned_t<arith_t>;
		const val_t& _val;

		[[nodiscard]]
		constexpr std::size_t calc_digits(const uarith_t num) const noexcept {
			return 1U + (num < 10 ? 0U : calc_digits(num / 10U));
		}

		[[nodiscard]]
		constexpr std::size_t digits(const arith_t num) const noexcept {
			return std::is_signed_v<int_t> && num < 0 ?
				1U + calc_digits(~uarith_t(num) + 1U) :
				calc_digits(uarith_t(num));
		}

		[[nodiscard]]
		constexpr std::size_t pow10(const std::size_t pow) const noexcept {
			return pow ? pow10(pow - 1U) * 10U : 1U;
		}

		[[nodiscard]]
		constexpr std::size_t zeros(const arith_t num) const noexcept {
			const int_t n{num / 10};
			return n * 10U == num ? 1U + zeros(n) : 0U;
		}

		[[nodiscard]]
		constexpr std::size_t hex_digits(const uarith_t num) const noexcept {
			return 1U + (num < 16U ? 0U : hex_digits(num >> 4U));
		}

		[[nodiscard]]
		constexpr std::size_t oct_digits(const uarith_t num) const noexcept {
			return 1U + (num < 8U ? 0U : oct_digits(num >> 3U));
		}

		[[nodiscard]]
		constexpr arith_t value_as_int(const val_t& num) const noexcept {
			return num;
		}


		constexpr uarith_t process(const uarith_t num, char* const buff, const std::size_t digits, const std::size_t idx) const noexcept {
			if (num < 10) {
				buff[digits - idx] = static_cast<char>(num + '0');
				buff[digits + 1] = 0;
			} else {
				const auto n{char(num - (process(num / 10U, buff, digits, idx + 1U) * 10U))};
				buff[digits - idx] = static_cast<char>(n + '0');
			}
			return num;
		}


		constexpr void process(const uarith_t num ,char* const buff) const noexcept {
			proccess(num, buff, digits() - 1U, 0);
		}

		template<typename T = int_t>
		constexpr std::enable_if_t<
			std::is_same_v<T, int_t> && std::is_integral_v<T> &&
			!is_boolean_v<T> && std::is_unsigned_v<T> && _len == npos
		>
		format(char* const buff) const noexcept {
			process(value_as_int(_val), buff);
		}

		template<typename T = int_t>
		constexpr std::enable_if_t<
			std::is_same_v<T, int_t> && std::is_integral_v<T> &&
			!is_boolean_v<T> && std::is_signed_v<T> && _len == npos
		>
		format(char* const buff) const noexcept {
			const auto value{value_as_int(_val)};
			if (value < 0) {
				buff[0] = '-';
				process(~uarith_t(value) + 1U, buff);
			}
			else
				process(uarith_t(value), buff);
		}

		constexpr void format_fixed(char* const buff, const uarith_t val) const noexcept {
			const auto len{calc_digits(val)};
			if (len <=  _len) {
				const auto offset{_len - len};
				const auto _buff{std::fill_n(buff, offset, padding)};
				process(val, _buff, uint8_t(len - 1U), 0U);
			} else {
				std::fill_n(buff, _len, padding);
			}
		}


		template<typename T = int_t>
		constexpr std::enable_if_t<
			std::is_same_v<T, int_t> && std::is_integral_v<T> &&
			!is_boolean_v<T> && std::is_unsigned_v<T> && _len != npos
		>
		format(char* const buff) const noexcept {
			format_fixed(buff, value_as_int(_val));
		}

		template<typename T = int_t>
		constexpr std::enable_if_t<
			std::is_same_v<T, int_t> && std::is_integral_v<T> &&
			!is_boolean_v<T> && std::is_signed_v<T> && _len != npos
		>
		format(char* const buff) const noexcept {
			const auto val{value_as_int(_val)};
			if (val < 0) {
				const auto len{digits(val)};
				if (len <= _len) {
					const auto offset{_len - len};
					const auto _buff{std::fill_n(buff + 1, offset, padding)};
					process(~uarith_t(val) + 1U, _buff, len - 2U, 0U);
				} else {
					std::fill_n(buff, _len, padding);
				}
				buff[0] = '-';
			} else {
				format_fixed(buff, uint_t(val));
			}
		}

		void format_hex(std::string &res, const bool capitals, const std::ptrdiff_t offset) const noexcept {
			auto val{value_as_int(_val)};
			auto digit{res.rbegin() + offset};
			while (digit != res.rend()) {
				auto hex{uarith_t(val & 0x0FU)};
				val >>= 4U;
				if (hex > 9U) {
					hex += 0x07U;
					if (!capitals) {
						hex += 0x20U;
					}
				}
				*digit++ = char(hex + 0x30U);
				if (!val) {
					break;
				}
			}
			std::fill_n(res.begin(), res.rend() - digit, padding);
		}

		void format_oct(std::string &res, const std::ptrdiff_t offset) const noexcept {
			auto val{value_as_int(_val)};
			auto digit{res.rbegin() + offset};
			while (digit != res.rend()) {
				auto oct{uarith_t(val & 0x07U)};
				val >>= 3U;
				*digit++ = char(oct + 0x30U);
				if (!val) {
					break;
				}
			}
			std::fill_n(res.begin(), res.rend() - digit, padding);
		}

		constexpr void format_frac(const std::uint8_t maxdig, char* const buff) const noexcept {
			const arith_t val{value_as_int(_val)};
			const auto dig{digits(val)};
			const auto frac{static_cast<uarith_t>(val)};

			if (!frac) {
				buff[0] = '0';
			} else if (dig >= maxdig) {
				const auto trunc{frac - ((frac / pow10(maxdig)) * pow10(maxdig))};
				if (!trunc) {
					buff[0] = '0';
				} else {
					const auto trailing{trailing_zeros()};
					process(static_cast<uarith_t>(trunc / pow10(trailing)), buff, maxdig - trailing - 1U, 0U);
				}
			} else {
				const auto trailing{trailing_zeros()};
				const auto leading{maxdig - dig};
				for (std::size_t i{}; i < leading; ++i) {
					buff[i] = '0';
				}
				process(static_cast<uarith_t>(frac / pow10(trailing)), buff + leading, dig - trailing - 1U, 0U);
			}
		}

	public:
		constexpr fromint_t(const val_t& val) noexcept : _val{val} { }

		[[nodiscard]]
		operator std::unique_ptr<char[]>() const {
			auto num{std::make_unique<char[]>(length())};
			if (!num) {
				return nullptr;
			}
			format(num.get());
			return num;
		}

		[[nodiscard]]
		explicit operator const char*() const {
			std::unique_ptr<char[]> num{*this};
			return num.release();
		}

		[[nodiscard]]
		operator std::string() const {
			std::string num(length(), '\0');
			format(num.data());
			return num;
		}

		template<typename T = int_t>
		[[nodiscard]]
		std::enable_if_t<std::is_same_v<T, int_t> && std::is_unsigned_v<T>, std::string>
		to_hex(const bool upper = true) const noexcept {
			std::string num(hex_length(), '\0');
			format_hex(num, upper, 1);
			return num;
		}

		template<typename T = int_t>
		[[nodiscard]]
		std::enable_if_t<std::is_same_v<T, int_t> && std::is_unsigned_v<T>, std::string>
		to_oct() const noexcept {
			std::string num(oct_length(), '\0');
			format_oct(num, 1);
			return num;
		}

		[[nodiscard]]
		std::string to_dec() const noexcept {
			return *this;
		}

		[[nodiscard]]
		constexpr std::size_t digits() const noexcept {
			return digits(value_as_int(_val));
		}

		[[nodiscard]]
		constexpr std::size_t length() const noexcept {
			return _len == npos ? digits() + 1U : _len +1U;
		}

		[[nodiscard]]
		constexpr std::size_t hex_length() const noexcept {
			return _len == npos ? hex_digits(value_as_int(_val)) + 1U : _len + 1U;
		}

		[[nodiscard]]
		constexpr std::size_t oct_length() const noexcept {
			return _len == npos ? oct_digits(value_as_int(_val)) + 1U : _len + 1U;
		}

		constexpr void format_to(char* const buff) const noexcept {
			format(buff);
		}

		[[nodiscard]]
		std::unique_ptr<char []> format_frac(const std::uint8_t maxdigs) const {
			auto num{std::make_unique<char[]>(fraction_length(maxdigs))};
			if (!num) {
				return nullptr;
			}
			format_frac(maxdigs, num.get());
			return num;
		}

		[[nodiscard]]
		constexpr std::size_t fraction_digits(const std::uint8_t maxdigs) const {
			const auto dig{digits()};
			if (dig > maxdigs) {
				return maxdigs;
			}
			return (maxdigs - dig) + (dig + trailing_zeros());
		}

		[[nodiscard]]
		constexpr std::size_t trailing_zeros() const noexcept {
			return value_as_int(_val) ? zeros(_val) : 0U;
		}

		[[nodiscard]]
		constexpr std::uint8_t fraction_length(const std::uint8_t maxdigs) const noexcept {
			return std::uint8_t(fraction_digits(maxdigs) + 1);
		}

		void format_frac_to(const std::uint8_t maxdigs, char* const buff) const noexcept {
			return format_frac(maxdigs, buff);
		}
	};

	template<typename val_t>
	fromint_t(val_t) -> fromint_t<val_t, val_t>;

	template<typename val_t>
	constexpr inline fromint_t<val_t, val_t> fromint(const val_t& val) {
		return {val};
	}

	template<typename int_t, typename val_t>
	constexpr inline fromint_t<int_t, val_t> fromint(const val_t& val) {
		return {val};
	}

	template<std::size_t len, typename val_t>
	constexpr inline fromint_t<val_t, val_t, len> fromint(const val_t& val) noexcept {
		return {val};
	}

	template<std::size_t len, char pad, typename val_t>
	constexpr inline fromint_t<val_t, val_t, len, pad> fromint(const val_t& val) noexcept {
		return {val};
	}

	template<std::uint8_t ch = '\n', bool collapse = false>
	auto split(const std::string& data) noexcept {
		constexpr const auto npos{std::string::npos};
		std::size_t begin{0};
		std::vector<std::string> lines{};

		while (begin != npos) {
			const auto end{data.find(ch, begin)};
			const auto len{(end == npos ? data.length() : end) - begin};
			lines.emplace_back(data.substr(begin, len));
			begin = end == npos ? npos : end + 1;
		}

		return lines;
	}


}

#endif /* SYCOPHANT_STRUTILS_HH */
