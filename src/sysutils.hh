// SPDX-License-Identifier: BSD-3-Clause
/* sysutils.hh - Helpers for collecting system data etc */
#pragma once
#if !defined(SYCOPHANT_SYSUTILS_HH)
#define SYCOPHANT_SYSUTILS_HH

#include <vector>

#include <types.hh>

namespace sycophant {

	void build_maps(std::vector<mapentry_t>& map_entries) noexcept;
}

#endif /* SYCOPHANT_SYSUTILS_HH */
