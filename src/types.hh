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
using pthread_t = unsigned long int;
using pthread_create_t = std::int32_t(*)(pthread_t*, const void*, void*(*)(void*), void*);
using pthread_join_t = std::int32_t(*)(pthread_t, void**);

namespace sycophant {
	template<typename F>
	struct enable_enum_bitmask_t {
		static constexpr bool enabled = false;
	};

	enum struct mapentry_flags_t : std::uint8_t {
		NONE   = 0b00000000U,
		READ   = 0b00000001U,
		WRITE  = 0b00000010U,
		EXEC   = 0b00000100U,
		PRIV   = 0b00001000U,
		SHARED = 0b00010000U,
		VIRT   = 0b00100000U,
		BACKED = 0b01000000U,
	};

	template<>
	struct enable_enum_bitmask_t<mapentry_flags_t> {
		static constexpr bool enabled = true;
	};


	struct mapentry_t final {
		std::uintptr_t   addr_s;
		std::uintptr_t   addr_e;
		std::uintptr_t   size;
		mapentry_flags_t flags;
		std::size_t      offset;
		/* There is a dev and inode here but we don't care about them */
		std::string path;
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

template<typename F>
std::enable_if_t<sycophant::enable_enum_bitmask_t<F>::enabled, F>
operator|(const F lh, const F rh) {
	using utype = typename std::underlying_type_t<F>;
	return static_cast<F>(static_cast<utype>(lh) | static_cast<utype>(rh));
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F1>::enabled &&
	std::is_integral_v<F2>, F1
>
operator|(const F1 lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F1>;
	return static_cast<F1>(static_cast<utype>(lh) | rh);
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F2>::enabled &&
	std::is_integral_v<F1>, F2
>
operator|(const F1 lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F2>;
	return static_cast<F2>(lh | static_cast<utype>(rh));
}

template<typename F>
std::enable_if_t<sycophant::enable_enum_bitmask_t<F>::enabled, F>
operator&(const F lh, const F rh) {
	using utype = typename std::underlying_type_t<F>;
	return static_cast<F>(static_cast<utype>(lh) & static_cast<utype>(rh));
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F1>::enabled &&
	std::is_integral_v<F2>, F1
>
operator&(const F1 lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F1>;
	return static_cast<F1>(static_cast<utype>(lh) & rh);
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F2>::enabled &&
	std::is_integral_v<F1>, F2
>
operator&(const F1 lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F2>;
	return static_cast<F2>(lh & static_cast<utype>(rh));
}


template<typename F>
std::enable_if_t<sycophant::enable_enum_bitmask_t<F>::enabled, F>
operator^(const F lh, const F rh) {
	using utype = typename std::underlying_type_t<F>;
	return static_cast<F>(static_cast<utype>(lh) ^ static_cast<utype>(rh));
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F1>::enabled &&
	std::is_integral_v<F2>, F1
>
operator^(const F1 lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F1>;
	return static_cast<F1>(static_cast<utype>(lh) ^ rh);
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F2>::enabled &&
	std::is_integral_v<F1>, F2
>
operator^(const F1 lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F2>;
	return static_cast<F2>(lh ^ static_cast<utype>(rh));
}

template<typename F>
std::enable_if_t<sycophant::enable_enum_bitmask_t<F>::enabled, F>
operator~(const F rh) {
	using utype = typename std::underlying_type_t<F>;
	return static_cast<F>(static_cast<utype>(~rh));
}

template<typename F>
std::enable_if_t<sycophant::enable_enum_bitmask_t<F>::enabled, F&>
operator|=(F& lh, const F rh) {
	using utype = typename std::underlying_type_t<F>;
	return lh = static_cast<F>(static_cast<utype>(lh) | static_cast<utype>(rh));
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F1>::enabled &&
	std::is_integral_v<F2>, F1&
>
operator|=(F1& lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F1>;
	return lh = static_cast<F1>(static_cast<utype>(lh) | rh);
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F2>::enabled &&
	std::is_integral_v<F1>, F1&
>
operator|=(F1& lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F2>;
	return lh = (lh | static_cast<utype>(rh));
}

template<typename F>
std::enable_if_t<sycophant::enable_enum_bitmask_t<F>::enabled, F&>
operator&=(F& lh, const F rh) {
	using utype = typename std::underlying_type_t<F>;
	return lh = static_cast<F>(static_cast<utype>(lh) & static_cast<utype>(rh));
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F1>::enabled &&
	std::is_integral_v<F2>, F1&
>
operator&=(F1& lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F1>;
	return lh = static_cast<F1>(static_cast<utype>(lh) & rh);
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F2>::enabled &&
	std::is_integral_v<F1>, F1&
>
operator&=(F1& lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F2>;
	return lh = (lh & static_cast<utype>(rh));
}

template<typename F>
std::enable_if_t<sycophant::enable_enum_bitmask_t<F>::enabled, F&>
operator^=(F& lh, const F rh) {
	using utype = typename std::underlying_type_t<F>;
	return lh = static_cast<F>(static_cast<utype>(lh) | static_cast<utype>(rh));
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F1>::enabled &&
	std::is_integral_v<F2>, F1&
>
operator^=(F1& lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F1>;
	return lh = static_cast<F1>(static_cast<utype>(lh) ^ rh);
}

template<typename F1, typename F2>
std::enable_if_t<
	sycophant::enable_enum_bitmask_t<F2>::enabled &&
	std::is_integral_v<F1>, F1&
>
operator^=(F1& lh, const F2 rh) {
	using utype = typename std::underlying_type_t<F2>;
	return lh = (lh ^ static_cast<utype>(rh));
}

#endif /* SYCOPHANT_TYPES_HH */
