// SPDX-License-Identifier: BSD-3-Clause
/* sycophant.cc - Sycophant LD_PRELOAD shim */

#include <sys/mman.h>
#include <unistd.h>
#include <linux/limits.h>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <optional>
#include <functional>
#include <string_view>
#include <variant>
#include <map>

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#undef _GNU_SOURCE

#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <config.hh>
#include <types.hh>
#include <strutils.hh>
#include <sysutils.hh>

#include <fd.hh>
#include <mmap.hh>

namespace fs = std::filesystem;
namespace py = pybind11;

namespace sycophant {
	std::unique_ptr<libc_start_main_t> old_libc_start{nullptr};

	std::map<std::string_view, py::module> imports{};
	std::map<std::string_view, std::string_view> envmap{};
	std::vector<mapentry_t> procmaps{};

	mmap_t self;

	[[nodiscard]]
	std::size_t param_count(py::function& func) {
		const auto res = imports["inspect"].attr("signature")(func);
		return py::len(res.attr("parameters"));
	}


	[[nodiscard]]
	std::optional<std::reference_wrapper<std::string_view>> getenv(std::string_view name) {
		if (const auto e = envmap.find(name);e != envmap.end()) {
			return std::make_optional(std::ref(e->second));
		}

		return std::nullopt;
	}

}

PYBIND11_EMBEDDED_MODULE(sycophant, m) {
	m.attr("__version__") = sycophant::config::version;

	m.def("hook_addr", [](std::uintptr_t addr, py::function func) {

	});

	m.def("add_hook", [](std::string_view name, py::function func) {

		// hooks.push_back({
		// 	name, func,  nullptr
		// });
	});

	m.def("get_maps", [](){
		return sycophant::procmaps;
	});

	m.def("refresh_maps", [](){
		sycophant::build_maps(sycophant::procmaps);
	});


	py::class_<sycophant::mapentry_t>(m, "mapentry")
		.def_readonly("start",      &sycophant::mapentry_t::addr_s    )
		.def_readonly("end",        &sycophant::mapentry_t::addr_e    )
		.def_readonly("size",       &sycophant::mapentry_t::size      )
		.def_readonly("prot",       &sycophant::mapentry_t::prot      )
		.def_readonly("offset",     &sycophant::mapentry_t::offset    )
		.def_readonly("path",       &sycophant::mapentry_t::path      )
		.def_readonly("is_virtual", &sycophant::mapentry_t::is_virtual)
		.def_readonly("is_backed",  &sycophant::mapentry_t::is_backed )
		.def("__repr__", [](const sycophant::mapentry_t& entry) {
			const auto start{sycophant::fromint_t(entry.addr_s).to_hex()};
			const auto end{sycophant::fromint_t(entry.addr_s).to_hex()};
			const auto path{[&](){
				if (entry.is_backed) {
					return "\"" + entry.path + "\"";
				} else {
					return std::string{};
				}
			}()};

			return "<mapentry (" + start + "-" + end + ") " + path + ">";
		});

}

extern "C" {
	[[gnu::used, gnu::visibility("default")]]
	std::int32_t __libc_start_main(
		main_t main, std::int32_t argc, char** argv,
		void_t init, void_t fini, void_t rtld_fini, void_t stack_end
	) {
		std::int32_t ret{1};
		char** envp = &argv[argc + 1];

		/* Find the end of the env block so we can map it to a span */
		for (char** env = envp; *env != nullptr; ++env) {
			const std::string_view e{*env};
			const auto tok = e.find("=");

			sycophant::envmap.insert({e.substr(0, tok), e.substr(tok + 1, e.length())});

			if (std::strncmp("LD_PRELOAD", *env, 10) == 0) {
				std::memset(env, 0, std::strlen(*env));
			} else if (std::strncmp("SYCOPHANT", *env, 9) == 0) {
				std::memset(env, 0, std::strlen(*env));
			}
		}

		// Build out memory map
		sycophant::build_maps(sycophant::procmaps);

		// Map our current process into memory
		sycophant::self = sycophant::fd_t("/proc/self/exe", O_RDONLY).map(sycophant::prot_t::R);
		/* Now that pre-init is over we can spin up the interpreter */

		py::scoped_interpreter guard{
			true, argc, argv, true
		};

		// Load `inspect` so we can extract information from passed in functions
		sycophant::imports.insert({"inspect", py::module::import("inspect")});

		// Load up the hook module if specified, otherwise the default one
		if (auto mod = sycophant::getenv("SYCOPHANT_MODULE")) {
			sycophant::imports.insert({"sycophant", py::module::import((mod->get()).data())});
		} else {
			sycophant::imports.insert({"sycophant", py::module::import("sycophant_hooks")});
		}

		// If we have the original __libc_start_main then call it now that we're all setup
		if (*sycophant::old_libc_start != nullptr) {
			ret = (*sycophant::old_libc_start)(main, argc, argv, init, fini, rtld_fini, stack_end);
		}

		return ret;
	}

	[[gnu::constructor]]
	static void sycophant_ctor() {
		/* yoink __libc_start_main for ourselves */
		sycophant::old_libc_start = std::make_unique<libc_start_main_t>(
			reinterpret_cast<libc_start_main_t>(dlsym(RTLD_NEXT, "__libc_start_main"))
		);

		if (*sycophant::old_libc_start == nullptr) {
			fputs("[sycophant] unable to find __libc_start_main, bailing", stdout);
			std::exit(1);
		}
	}
}
