// SPDX-License-Identifier: BSD-3-Clause
/* bitutils.hh - Helpers for mashing with bits */
#pragma once
#if !defined(SYCOPHANT_BITUTILS_HH)
#define SYCOPHANT_BITUTILS_HH

#include <cstdint>

namespace sycophant {

	[[nodiscard]]
	inline constexpr std::uint8_t swap(const std::uint8_t v) noexcept {
		return std::uint8_t(
			((v & 0x0FU) << 4U) |
			((v & 0xF0U) >> 4U)
		);
	}

	[[nodiscard]]
	inline constexpr std::uint16_t swap(const std::uint16_t v) noexcept {
		return std::uint16_t(
			((v & 0x00FFU) << 8U) |
			((v & 0xFF00U) >> 8U)
		);
	}

	[[nodiscard]]
	inline constexpr std::uint32_t swap(const std::uint32_t v) noexcept {
		return std::uint32_t(
			((v & 0x000000FFU) << 24U) |
			((v & 0x0000FF00U) << 8U ) |
			((v & 0x00FF0000U) >> 8U ) |
			((v & 0xFF000000U) >> 24U)
		);
	}

	[[nodiscard]]
	inline constexpr std::uint64_t swap(const std::uint64_t v) noexcept {
		return std::uint64_t(
			((v & 0x00000000000000FFU) << 56U) |
			((v & 0x000000000000FF00U) << 40U) |
			((v & 0x0000000000FF0000U) << 24U) |
			((v & 0x00000000FF000000U) << 8U ) |
			((v & 0x000000FF00000000U) >> 8U ) |
			((v & 0x0000FF0000000000U) >> 24U) |
			((v & 0x00FF000000000000U) >> 40U) |
			((v & 0xFF00000000000000U) >> 56U)
		);
	}

}

#endif /* SYCOPHANT_BITUTILS_HH */
