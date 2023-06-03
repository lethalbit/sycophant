// SPDX-License-Identifier: BSD-3-Clause
/* sysutils.cc - Helpers for collecting system data etc */

#include <sysutils.hh>

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>
#include <algorithm>

#include <fcntl.h>

#include <strutils.hh>
#include <fd.hh>

namespace sycophant {

	void build_maps(std::vector<mapentry_t>& map_entries) noexcept {
		map_entries.clear();

		auto maps = sycophant::fd_t("/proc/self/maps", O_RDONLY);
		std::string map_data{};
		// This is fucky-wucky because you can't stat `/proc/*` files to get their size
		char c{};
		while (!maps.isEOF()) {
			maps.read(&c, 1);
			map_data.append(1, c);
		}

		const auto lines{split(map_data)};

		for (auto map_line : lines) {
			if (map_line.length() == 0) {
				continue;
			}

			map_line.erase(std::unique(map_line.begin(), map_line.end(), [](const char& a, const char& b) {
           		return a == ' ' && b == ' ';
        	}), map_line.end());

			auto contents = split<' ', true>(map_line.data());

			mapentry_t entry{};

			// Ingest the addr range
			auto& addr_range = contents[0];
			const auto end = addr_range.find('-', 0);
			const auto addr_s = std::string_view{addr_range.substr(0, end)};
			const auto addr_e = std::string_view{addr_range.substr(end + 1, addr_range.length())};
			entry.addr_s = toint_t<std::uintptr_t>(addr_s).from_hex();
			entry.addr_e = toint_t<std::uintptr_t>(addr_e).from_hex();
			entry.size   = entry.addr_e - entry.addr_s;

			// Map protection
			auto& prot = contents[1];
			if (prot[0] == 'r') {
				entry.prot |= PROT_READ;
			}
			if (prot[1] == 'w')  {
				entry.prot |= PROT_WRITE;
			}
			if (prot[2] == 'x') {
				entry.prot |= PROT_EXEC;
			}

			if (prot[3] == 'p') {
				entry.prot |= MAP_PRIVATE;
			} else if (prot[3] == 's') {
				entry.prot |= MAP_SHARED;
			}

			// offset
			entry.offset = toint_t<std::uint64_t>(contents[2]).from_hex();

			// backing
			if (contents.size() == 6) {
				entry.path = std::string{contents[5]};
				entry.is_virtual = (entry.path[0] == '[');
				entry.is_backed = true;
			} else {
				entry.is_backed = false;
			}

			map_entries.emplace_back(entry);
		}
	}


	[[nodiscard]]
	bool addr_mapped(std::vector<mapentry_t>& map_entries, const std::uintptr_t addr) noexcept {
		for (auto& entry : map_entries) {
			if (addr >= entry.addr_s || addr <= entry.addr_e) {
				return true;
			}
		}
		return false;
	}

}
