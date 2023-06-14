// SPDX-License-Identifier: BSD-3-Clause
/* sysutils.cc - Helpers for collecting system data etc */

#include <sysutils.hh>

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <optional>

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
			const auto addr_s = std::string{addr_range.substr(0, end).data(), end};
			const auto addr_e = std::string{addr_range.substr(end + 1, addr_range.length()).data()};
			entry.addr_s = toint_t<std::uintptr_t>(addr_s).from_hex();
			entry.addr_e = toint_t<std::uintptr_t>(addr_e).from_hex();
			entry.size   = entry.addr_e - entry.addr_s;

			// Map protection
			auto& prot = contents[1];
			if (prot[0] == 'r') {
				entry.flags |= mapentry_flags_t::READ;
			}
			if (prot[1] == 'w')  {
				entry.flags |= mapentry_flags_t::WRITE;
			}
			if (prot[2] == 'x') {
				entry.flags |= mapentry_flags_t::EXEC;
			}

			if (prot[3] == 'p') {
				entry.flags |= mapentry_flags_t::PRIV;
			} else if (prot[3] == 's') {
				entry.flags |= mapentry_flags_t::SHARED;
			}

			// offset
			entry.offset = toint_t<std::uint64_t>(contents[2]).from_hex();

			entry.path = std::string{contents[5]};
			if (!entry.path.empty()) {
				if (entry.path[0] == '[') {
					entry.flags |= mapentry_flags_t::VIRT;
				}
				entry.flags |= mapentry_flags_t::BACKED;
			}

			map_entries.emplace_back(entry);
		}
	}

	[[nodiscard]]
	std::optional<std::reference_wrapper<const mapentry_t>> get_map_entry(const std::vector<mapentry_t>& map_entries, std::uintptr_t addr) noexcept {
		auto res = std::find_if(std::begin(map_entries), std::end(map_entries), [&](const mapentry_t& entry){
			return (addr >= entry.addr_s && addr <= entry.addr_e);
		});

		if (res == std::end(map_entries)) {
			return std::nullopt;
		}

		return std::make_optional(std::ref(*res));
	}

}
