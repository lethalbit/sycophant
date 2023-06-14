// SPDX-License-Identifier: BSD-3-Clause
/* sysutils.hh - Helpers for collecting system data etc */
#pragma once
#if !defined(SYCOPHANT_SYSUTILS_HH)
#define SYCOPHANT_SYSUTILS_HH

#include <vector>
#include <optional>

#include <types.hh>

namespace sycophant {

	void build_maps(std::vector<mapentry_t>& map_entries) noexcept;

	[[nodiscard]]
	std::optional<std::reference_wrapper<const mapentry_t>> get_map_entry(const std::vector<mapentry_t>& map_entries, std::uintptr_t addr) noexcept;
}

#endif /* SYCOPHANT_SYSUTILS_HH */
