// SPDX-License-Identifier: BSD-3-Clause
/* mmap.hh - RAII Wrapper mmap */
#pragma once
#if !defined(SYCOPHANT_MMAP_HH)
#define SYCOPHANT_MMAP_HH

#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <string_view>
#include <stdexcept>

namespace sycophant {

	struct mmap_t final {
	private:
		std::size_t _len{0};
		void* _addr{nullptr};
		std::int32_t _fd{-1};

		mmap_t(
			const mmap_t& map, const std::size_t len, const std::int32_t prot,
			const std::int32_t flags = MAP_SHARED, void* addr = nullptr
		) noexcept : _len{len}, _addr{[&]() noexcept -> void* {
			const auto ptr = ::mmap(addr, len, prot, flags, map._fd, 0);
			return ptr == MAP_FAILED ? nullptr : ptr;
		}()}, _fd{-1} { }

		template<typename T>
		std::enable_if_t<std::is_standard_layout_v<T> && !std::is_same_v<T, void*>, T*>
		index(const std::size_t idx) const {
			if (idx < _len) {
				const auto addr = reinterpret_cast<std::uintptr_t>(_addr);
				return new(reinterpret_cast<void*>(addr + (idx * sizeof(T)))) T{};
			}
			throw std::out_of_range("mmap_t index out of range");
		}

		template<typename T>
		std::enable_if_t<std::is_same_v<T, void*>, void*>
		index(const std::size_t idx) const {
			if (idx < _len) {
				const auto addr = reinterpret_cast<std::uintptr_t>(_addr);
				return reinterpret_cast<void *>(addr + idx);
			}
			throw std::out_of_range("mmap_t index out of range");
		}

	public:
		constexpr mmap_t() noexcept = default;
		mmap_t(
			const std::int32_t fd, const std::size_t len, const std::int32_t prot,
			const std::int32_t flags = MAP_SHARED, void* addr = nullptr
		) noexcept : _len{len}, _addr{[&]() noexcept -> void* {
			const auto ptr = ::mmap(addr, len, prot, flags, fd, 0);
			return ptr == MAP_FAILED ? nullptr : ptr;
		}()}, _fd{fd} { }

		mmap_t(mmap_t&& map) noexcept : mmap_t{} { swap(map); }
		void operator =(mmap_t&& map) noexcept { swap(map); }
		mmap_t(const mmap_t&) = delete;
		mmap_t &operator=(const mmap_t&) = delete;

		~mmap_t() noexcept {
			if (_addr)
				::munmap(_addr, _len);
			if (_fd != -1)
				::close(_fd);
		}

		[[nodiscard]]
		bool valid() const noexcept {
			return _addr != nullptr;
		}

		void swap(mmap_t& map) noexcept {
			std::swap(_fd, map._fd);
			std::swap(_addr, map._addr);
			std::swap(_len, map._len);
		}

		[[nodiscard]]
		mmap_t dup(const std::int32_t prot, const std::size_t len, const std::int32_t flags, void* addr) const noexcept {
			if (!valid()) {
				return {};
			}
			return {*this, len, prot, flags, addr};
		}

		[[nodiscard]]
		bool chperm(const std::int32_t prot) noexcept {
			return ::mprotect(_addr, _len, prot) == 0;
		}

		template<typename T>
		T* address() noexcept {
			return static_cast<T*>(_addr);
		}

		template<typename T>
		const T* address() const noexcept {
			return static_cast<const T*>(_addr);
		}

		[[nodiscard]]
		std::size_t length() const noexcept {
			return _len;
		}

		template<typename T>
		T* operator[](const std::size_t idx) {
			return index<T>(idx);
		}
		template<typename T>
		const T* operator[](const off_t idx) const {
			return index<const T>(idx);
		}
		template<typename T>
		T* at(const std::size_t idx) {
			return index<T>(idx);
		}
		template<typename T>
		const T* at(const std::size_t idx) const {
			return index<const T>(idx);
		}

		void* address(const std::size_t offset) noexcept {
			return index<void *>(offset);
		}
		const void* address(const std::size_t offset) const noexcept {
			return index<const void *>(offset);
		}

		[[nodiscard]]
		std::uintptr_t numeric_address() const noexcept {
			return reinterpret_cast<std::uintptr_t>(_addr);
		}

		[[nodiscard]]
		bool lock() const noexcept {
			return lock(_len);
		}

		[[nodiscard]]
		bool lock(std::size_t len) const noexcept {
			return ::mlock(_addr, len) == 0;
		}

		[[nodiscard]]
		bool lock_at(const std::size_t idx, const std::size_t len) const noexcept {
			const auto addr = reinterpret_cast<std::uintptr_t>(_addr);
			return ::mlock(reinterpret_cast<void *>(addr + idx), len) == 0;
		}

			[[nodiscard]]
		bool unlock() const noexcept {
			return unlock(_len);
		}

		[[nodiscard]]
		bool unlock(std::size_t len) const noexcept {
			return ::munlock(_addr, len) == 0;
		}

		[[nodiscard]]
		bool unlock_at(const std::size_t idx, const std::size_t len) const noexcept {
			const auto addr = reinterpret_cast<std::uintptr_t>(_addr);
			return ::munlock(reinterpret_cast<void *>(addr + idx), len) == 0;
		}

		[[nodiscard]]
		bool remap(const std::int32_t flags, const std::size_t new_len) noexcept {
			const auto old_len = _len;
			_len = new_len;
			return (_addr = ::mremap(_addr, old_len, _len, flags)) != MAP_FAILED;
		}

		[[nodiscard]]
		bool remap(const std::int32_t flags, const std::size_t new_len, const std::uintptr_t new_addr) noexcept {
			const auto old_len = _len;
			_len = new_len;
			const void *wanted_addr = reinterpret_cast<void *>(new_addr);
			return (_addr = ::mremap(_addr, old_len, _len, flags, wanted_addr)) != MAP_FAILED;
		}

		[[nodiscard]]
		bool sync(const std::int32_t flags = MS_SYNC | MS_INVALIDATE) const noexcept {
			return sync(_len, flags);
		}
		[[nodiscard]]
		bool sync(const std::size_t length, const std::int32_t flags = MS_SYNC | MS_INVALIDATE) const noexcept {
			return msync(_addr, length, flags) == 0;
		}
		[[nodiscard]]
		bool advise(const std::int32_t advice) const noexcept {
			return advise(advice, _len);
		}
		[[nodiscard]]
		bool advise(const std::int32_t advice, const std::size_t len) const noexcept {
			return ::madvise(_addr, len, advice) == 0;
		}
		[[nodiscard]]
		bool advise_at(const std::int32_t advice, const std::size_t len, const std::size_t idx) const noexcept {
			const auto addr = reinterpret_cast<std::uintptr_t>(_addr);
			return ::madvise(reinterpret_cast<void *>(addr + idx), len, advice) == 0;
		}

		template<typename T>
		void copy_to(const std::size_t idx, T& val) const {
			const auto src = index<const void*>(idx);
			std::memcpy(&val, src, sizeof(T));
		}

		template<typename T, std::size_t N>
		void copy_to(const std::size_t idx, std::array<T, N>& val) const {
			constexpr auto len{sizeof(T) * N};
			const auto src = index<const void*>(idx);
			std::memcpy(val.data(), src, len);
		}

		void copy_to(const std::size_t idx, std::string& val) const {
			const auto src = index<const void*>(idx);
			std::memcpy(const_cast<char *>(val.data()), src, val.size());
		}

		void copy_to(const std::size_t idx, std::string_view& val) const {
			const auto src = index<const void*>(idx);
			std::memcpy(const_cast<char *>(val.data()), src, val.size());
		}

		template<typename T>
		void copy_from(const std::size_t idx, const T& val) const {
			const auto dest = index<void*>(idx);
			std::memcpy(dest, &val, sizeof(T));
		}

		template<typename T, std::size_t N>
		void copy_to(const std::size_t idx, const std::array<T, N>& val) const {
			constexpr auto len{sizeof(T) * N};
			const auto dest = index<void*>(idx);
			std::memcpy(dest, val.data(), len);
		}

		void copy_to(const std::size_t idx, const std::string& val) const {
			const auto dest = index<void*>(idx);
			std::memcpy(dest, val.data(), val.size());
		}

		void copy_to(const std::size_t idx, const std::string_view& val) const {
			const auto dest = index<void*>(idx);
			std::memcpy(dest, val.data(), val.size());
		}

		bool operator==(const mmap_t& b) const noexcept {
			return _fd == b._fd && _addr == b._addr && _len == b._len;
		}
		bool operator!=(const mmap_t& b) const noexcept {
			return !(*this == b);
		}
	};

	inline void swap(mmap_t& a, mmap_t& b) noexcept { a.swap(b); }

}

#endif /* SYCOPHANT_MMAP_HH */
