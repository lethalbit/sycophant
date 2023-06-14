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
#include <vector>

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

#include <rwlock.hh>
#include <fd.hh>
#include <mmap.hh>
#include <elf.hh>

namespace fs = std::filesystem;
namespace py = pybind11;

namespace sycophant {
	struct sycophant_t final {
		std::unique_ptr<libc_start_main_t> old_libc_start{nullptr};
		std::unique_ptr<pthread_create_t> old_pthread_create{nullptr};
		std::unique_ptr<pthread_join_t> old_pthread_join{nullptr};

		std::map<std::string_view, py::module> imports{};
		std::map<std::string_view, std::string_view> envmap{};
		rwlock_t<std::vector<mapentry_t>> procmaps{};
		rwlock_t<std::vector<std::uint64_t>> threads{};

		mmap_t self;
		mmap_t trampoline{-1, 8192, prot_t::RWX, MAP_PRIVATE | MAP_ANONYMOUS};
	} state{};

	[[nodiscard]]
	std::size_t param_count(py::function& func) {
		const auto res = state.imports["inspect"].attr("signature")(func);
		return py::len(res.attr("parameters"));
	}


	[[nodiscard]]
	std::optional<std::reference_wrapper<std::string_view>> getenv(std::string_view name) {
		if (const auto e = state.envmap.find(name);e != state.envmap.end()) {
			return std::make_optional(std::ref(e->second));
		}

		return std::nullopt;
	}


	[[nodiscard]]
	fs::path expanduser(std::string_view path) {
		if (!path.empty() && path[0] == '~' && path[1] == '/') {
			fs::path tmp{};
			if (auto homedir = getenv("HOME")) {
				tmp.concat(homedir->get());

			} else if(auto username = getenv("USER")) {
				tmp.concat("/home/");
				tmp.concat(username->get());
			} else {
				tmp.concat("~");
			}
			tmp.concat(path.substr(1, path.length()));

			return tmp;
		}

		return fs::path(path);
	}


}

PYBIND11_EMBEDDED_MODULE(sycophant, m) {
	m.attr("__version__") = sycophant::config::version;
	m.attr("__doc__") = "Sycophant Python API";

	auto proc = m.def_submodule("proc", "interact with the running process");


	auto proc_mem = proc.def_submodule("mem", "interact with process memory");

	proc_mem.def("read", [](std::uintptr_t addr, std::size_t len) {

	});

	proc_mem.def("write", [](std::uintptr_t addr, std::vector<std::uint8_t> buff) {

	});

	auto proc_threads = proc.def_submodule("threads", "process thread information");

	proc_threads.def("known", []() {
		return *(sycophant::state.threads.read());
	});

	auto proc_maps = proc.def_submodule("maps", "process map information");

	proc_maps.def("all", []() {
		auto maps = sycophant::state.procmaps.read();
		return *maps;
	});

	proc_maps.def("get", [](std::size_t idx) -> std::optional<sycophant::mapentry_t> {
		auto maps = sycophant::state.procmaps.read();
		if (idx > maps->size()) {
			return std::nullopt;
		}
		return std::make_optional((*maps)[idx]);
	});

	proc_maps.def("refresh", []() {
		auto maps = sycophant::state.procmaps.write();

		sycophant::build_maps(*maps);
	});

	proc_maps.def("has_addr", [](std::uintptr_t addr) {
		auto maps = sycophant::state.procmaps.read();

		return sycophant::addr_mapped(*maps, addr);
	});

	py::class_<sycophant::mapentry_t>(proc_maps, "mapentry")
		.def_readonly("start",      &sycophant::mapentry_t::addr_s    )
		.def_readonly("end",        &sycophant::mapentry_t::addr_e    )
		.def_readonly("size",       &sycophant::mapentry_t::size      )
		.def_readonly("flags",      &sycophant::mapentry_t::flags     )
		.def_readonly("offset",     &sycophant::mapentry_t::offset    )
		.def_readonly("path",       &sycophant::mapentry_t::path      )
		.def("can_read", [](const sycophant::mapentry_t& entry) {
			return (entry.flags & sycophant::mapentry_flags_t::READ) == sycophant::mapentry_flags_t::READ;
		})
		.def("can_write", [](const sycophant::mapentry_t& entry) {
			return (entry.flags & sycophant::mapentry_flags_t::WRITE) == sycophant::mapentry_flags_t::WRITE;
		})
		.def("can_execute", [](const sycophant::mapentry_t& entry) {
			return (entry.flags & sycophant::mapentry_flags_t::EXEC) == sycophant::mapentry_flags_t::EXEC;
		})
		.def("is_backed", [](const sycophant::mapentry_t& entry) {
			return (entry.flags & sycophant::mapentry_flags_t::BACKED) == sycophant::mapentry_flags_t::BACKED;
		})
		.def("is_virtual", [](const sycophant::mapentry_t& entry) {
			return (entry.flags & sycophant::mapentry_flags_t::VIRT) == sycophant::mapentry_flags_t::VIRT;
		})
		.def("__repr__", [](const sycophant::mapentry_t& entry) {
			const auto start{sycophant::fromint_t(entry.addr_s).to_hex()};
			const auto end{sycophant::fromint_t(entry.addr_e).to_hex()};
			const auto size{sycophant::fromint_t(entry.size).to_dec()};
			const auto path{[&](){
				if ((entry.flags & sycophant::mapentry_flags_t::BACKED) == sycophant::mapentry_flags_t::BACKED) {
					return "\"" + entry.path + "\"";
				} else {
					return std::string{"ANONYMOUS"};
				}
			}()};

			const auto prot{[&](){
				std::string _prot{};
				if ((entry.flags & sycophant::mapentry_flags_t::READ) == sycophant::mapentry_flags_t::READ) {
					_prot.append("r");
				} else {
					_prot.append("-");
				}
				if ((entry.flags & sycophant::mapentry_flags_t::WRITE) == sycophant::mapentry_flags_t::WRITE) {
					_prot.append("w");
				} else {
					_prot.append("-");
				}
				if ((entry.flags & sycophant::mapentry_flags_t::EXEC) == sycophant::mapentry_flags_t::EXEC) {
					_prot.append("x");
				} else {
					_prot.append("-");
				}
				if ((entry.flags & sycophant::mapentry_flags_t::SHARED) == sycophant::mapentry_flags_t::SHARED) {
					_prot.append("s");
				} else if ((entry.flags & sycophant::mapentry_flags_t::PRIV) == sycophant::mapentry_flags_t::PRIV) {
					_prot.append("p");
				} else {
					_prot.append("?");
				}
				return _prot;
			}()};

			return "<mapentry " + start + ":" + end + " (" + size + " bytes) " + prot + "  " + path + ">";
		});

}

extern "C" {
	// cheeky-hack to re-name the symbol because it's dragged in from one of our includes
	std::int32_t sycophant_pthread_create(pthread_t* pid, const void* attr, void*(*start)(void*), void* args) asm ("pthread_create");
	std::int32_t sycophant_pthread_join(pthread_t pid, void** retval) asm ("pthread_join");

	[[gnu::used, gnu::visibility("default")]]
	std::int32_t sycophant_pthread_create(pthread_t* pid, const void* attr, void*(*start)(void*), void* args) {
		std::int32_t ret{};
		if (*sycophant::state.old_pthread_create != nullptr) {
			ret = (*sycophant::state.old_pthread_create)(pid, attr, start, args);
			auto thrds = sycophant::state.threads.write();
			thrds->push_back(*pid);
		}

		return ret;
	}

	[[gnu::used, gnu::visibility("default")]]
	std::int32_t sycophant_pthread_join(pthread_t pid, void** retval) {
		std::int32_t ret{};
		if (*sycophant::state.old_pthread_join != nullptr) {
			ret = (*sycophant::state.old_pthread_join)(pid, retval);
			auto thrds = sycophant::state.threads.write();
			thrds->erase(std::find(std::begin(*thrds), std::end(*thrds), pid));
		}

		return ret;
	}

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

			sycophant::state.envmap.insert({e.substr(0, tok), e.substr(tok + 1, e.length())});

			if (std::strncmp("LD_PRELOAD", *env, 10) == 0) {
				std::memset(env, 0, std::strlen(*env));
			} else if (std::strncmp("SYCOPHANT", *env, 9) == 0) {
				std::memset(env, 0, std::strlen(*env));
			}
		}

		const fs::path user_modules{sycophant::expanduser("~/.config/sycophant"sv)};

		// Build out memory map
		sycophant::build_maps(*(sycophant::state.procmaps.write()));

		// Map our current process into memory
		sycophant::state.self = sycophant::fd_t("/proc/self/exe", O_RDONLY).map(sycophant::prot_t::R);
		/* Now that pre-init is over we can spin up the interpreter */

		py::scoped_interpreter guard{
			true, argc, argv, true
		};

		// Preload some modules
		sycophant::state.imports.insert({"sys",     py::module::import("sys")    });
		sycophant::state.imports.insert({"inspect", py::module::import("inspect")});

		// Check to see if the user modules exist, and add them to the path if so
		if (fs::exists(user_modules)) {
			sycophant::state.imports["sys"].attr("path").attr("insert")(0, user_modules.c_str());
		}

		// Load up the hook module if specified, otherwise the default one
		if (auto mod = sycophant::getenv("SYCOPHANT_MODULE")) {
			sycophant::state.imports.insert({"sycophant", py::module::import((mod->get()).data())});
		} else {
			sycophant::state.imports.insert({"sycophant", py::module::import("sycophant_hooks")});
		}




		// If we have the original __libc_start_main then call it now that we're all setup
		if (*sycophant::state.old_libc_start != nullptr) {
			ret = (*sycophant::state.old_libc_start)(main, argc, argv, init, fini, rtld_fini, stack_end);
		}

		return ret;
	}

	[[gnu::constructor]]
	static void sycophant_ctor() {
		/* yoink __libc_start_main for ourselves */
		sycophant::state.old_libc_start = std::make_unique<libc_start_main_t>(
			reinterpret_cast<libc_start_main_t>(dlsym(RTLD_NEXT, "__libc_start_main"))
		);

		sycophant::state.old_pthread_create = std::make_unique<pthread_create_t>(
			reinterpret_cast<pthread_create_t>(dlsym(RTLD_NEXT, "pthread_create"))
		);

		sycophant::state.old_pthread_join = std::make_unique<pthread_join_t>(
			reinterpret_cast<pthread_join_t>(dlsym(RTLD_NEXT, "pthread_join"))
		);

		if (*sycophant::state.old_libc_start == nullptr) {
			fputs("[sycophant] unable to find __libc_start_main, bailing", stdout);
			std::exit(1);
		}
	}
}
