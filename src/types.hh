// SPDX-License-Identifier: BSD-3-Clause
/* types.hh - Common type definitions used in sycophant */
#pragma once
#if !defined(SYCOPHANT_TYPES_HH)
#define SYCOPHANT_TYPES_HH

#include <sys/mman.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <unistd.h>

using void_t = void(*)();
using main_t = std::int32_t(*)(std::int32_t,  char**, char**);
using init_t = main_t;
using libc_start_main_t = std::int32_t(*)(main_t, std::int32_t, char**, void_t, void_t, void_t, void_t);
using dlsym_t = void*(*)(void*, const char*);


namespace sycophant {
	struct mapentry_t final {
		std::uintptr_t addr_s;
		std::uintptr_t addr_e;
		std::uintptr_t size;
		std::uint8_t prot;
		std::size_t offset;
		/* There is a dev and inode here but we don't care about them */
		std::string path;

		bool is_virtual;
		bool is_backed;
	};


	template<typename T, bool = std::is_unsigned_v<T>>
	struct promoted_type;
	template<typename T>
	struct promoted_type<T, true> {
		using type = std::uint32_t;
	};
	template<typename T>
	struct promoted_type<T, false> {
		using type = std::int32_t;
	};

	template<>
	struct promoted_type<std::uint64_t, true> {
		using type = std::uint64_t;
	};
	template<>
	struct promoted_type<std::int64_t, false> {
		using type = std::int64_t;
	};
	template<>
	struct promoted_type<std::size_t, !std::is_same_v<std::uint64_t, std::size_t>> {
		using type = size_t;
	};
	template<>
	struct promoted_type<::ssize_t, std::is_same_v<std::int64_t, ssize_t>> {
		using type = ssize_t;
	};

	template<typename T>
	using promoted_type_t = typename promoted_type<T>::type;

	namespace internal {
		template<typename> struct is_char : std::false_type { };
		template<> struct is_char<char> : std::true_type { };

		template<typename> struct is_boolean : public std::false_type { };
		template<> struct is_boolean<bool> : public std::true_type { };
	}

	template<typename T>
	struct is_char : public std::integral_constant<bool, internal::is_char<std::remove_cv_t<T>>::value> { };

	template<typename T>
	constexpr bool is_char_v = is_char<T>::value;

	template<typename T>
	struct is_boolean : public std::integral_constant<bool, internal::is_boolean<std::remove_cv_t<T>>::value> { };

	template<typename T>
	constexpr bool is_boolean_v = is_boolean<T>::value;
}

#endif /* SYCOPHANT_TYPES_HH */
