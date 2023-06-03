// SPDX-License-Identifier: BSD-3-Clause
/* fd.hh - RAII Wrapper for file descriptors */
#pragma once
#if !defined(SYCOPHANT_FD_HH)
#define SYCOPHANT_FD_HH

#include <type_traits>
#include <fcntl.h>
#include <utility>
#include <memory>
#include <array>
#include <string>
#include <string_view>
#include <filesystem>
#include <cstdint>
#include <cstddef>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <types.hh>
#include <mmap.hh>


namespace fs = std::filesystem;

namespace sycophant {

	using stat_t = struct stat;

	constexpr static ::mode_t normal_mode = S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH;

	namespace {
		[[nodiscard]]
		inline std::int32_t fdopen(const char* const filename, const std::int32_t flags, const ::mode_t mode) noexcept {
			return ::open(filename, flags, mode);
		}

		[[nodiscard]]
		inline ::ssize_t fdread(const std::int32_t fd, void* const buff, const std::size_t len) noexcept {
			return ::read(fd, buff, len);
		}

		[[nodiscard]]
		inline ::ssize_t fdwrite(const std::int32_t fd, const void* const buff, const std::size_t len) noexcept {
			return ::write(fd, buff, len);
		}

		[[nodiscard]]
		inline ::off_t fdseek(const std::int32_t fd, const ::off_t offset, const std::int32_t whence) noexcept {
			return ::lseek(fd, offset, whence);
		}

		[[nodiscard]]
		inline ::off_t fdtell(const std::int32_t fd) noexcept {
			return ::lseek(fd, 0, SEEK_CUR);
		}

		[[nodiscard]]
		inline std::int32_t fdtruncate(const std::int32_t fd, const ::off_t size) noexcept {
			return ::ftruncate(fd, size);
		}
	}


	struct fd_t final {
	private:
		std::int32_t _fd{-1};
		mutable bool _eof{false};
		mutable ::off_t _len{-1};

	public:
		constexpr fd_t() noexcept = default;
		constexpr fd_t(const std::int32_t fd) noexcept : _fd{fd} { }

		fd_t(const char* const filename, const std::int32_t flags, const ::mode_t mode = 0) noexcept :
			_fd{fdopen(filename, flags, mode)} { }

		fd_t(const std::string& filename, const std::int32_t flags, const ::mode_t mode = 0) noexcept :
			_fd{fdopen(filename.c_str(), flags, mode)} { }

		fd_t(const std::string_view& filename, const std::int32_t flags, const ::mode_t mode = 0) noexcept :
			_fd{fdopen(filename.data(), flags, mode)} { }

		fd_t(const fs::path& filename, const std::int32_t flags, const ::mode_t mode = 0) noexcept :
			_fd{fdopen(filename.c_str(), flags, mode)} { }

		fd_t(fd_t&& fd) noexcept : fd_t{} { swap(fd); }
		~fd_t() noexcept { if (_fd != -1) ::close(_fd); }
		void operator=(fd_t&& fd) noexcept { swap(fd); }

		[[nodiscard]]
		operator int32_t() const noexcept { return _fd; }
		[[nodiscard]]
		bool operator==(const std::int32_t desc) const noexcept { return _fd == desc; }
		[[nodiscard]]
		bool valid() const noexcept { return _fd != -1; }
		[[nodiscard]]
		bool isEOF() const noexcept { return _eof; }
		void invalidate() noexcept { _fd = -1; }

		void swap(fd_t& desc) noexcept {
			std::swap(_fd, desc._fd);
			std::swap(_eof, desc._eof);
			std::swap(_len, desc._len);
		}

		[[nodiscard]]
		::ssize_t read(void* const buff, const std::size_t len, std::nullptr_t) const noexcept {
			const auto res = fdread(_fd, buff, len);
			if (!res && len) {
				_eof = true;
			}
			return res;
		}

		[[nodiscard]]
		::ssize_t write(const void* const buff, const std::size_t len, std::nullptr_t) const noexcept {
			return fdwrite(_fd, buff, len);
		}

		[[nodiscard]]
		::off_t seek(const ::off_t offset, const std::int32_t whence) const noexcept {
			const auto res = fdseek(_fd, offset, whence);
			_eof = res == length();
			return res;
		}

		[[nodiscard]]
		::off_t tell() const noexcept {
			return fdtell(_fd);
		}

		[[nodiscard]]
		bool head() const noexcept {
			return seek(0, SEEK_SET) == 0;
		}
		[[nodiscard]]
		bool tail() const noexcept {
			const auto offset = length();
			if (offset < 0) {
				return false;

			}
			return seek(offset, SEEK_SET) == offset;
		}

		[[nodiscard]]
		fd_t dup() const noexcept {
			return ::dup(_fd);
		}

		[[nodiscard]]
		stat_t stat() const noexcept {
			stat_t stat{};
			if (!::fstat(_fd, &stat)) {
				return stat;
			}
			return {};
		}

		[[nodiscard]]
		::off_t length() const noexcept {
			if (_len != -1) {
				return _len;
			}

			stat_t stat{};
			const int res = ::fstat(_fd, &stat);
			_len = res ? -1 : stat.st_size;
			return _len;
		}

		[[nodiscard]]
		bool resize(const ::off_t size) const noexcept {
			const auto res = fdtruncate(_fd, size) == 0;
			if (!res) {
				stat_t stat{};
				const int sres = ::fstat(_fd, &stat);
				_len = sres ? -1 : stat.st_size;
			}
			return res;
		}

		[[nodiscard]]
		bool read(void* const val, const std::size_t len, std::size_t& reslen) const noexcept {
			const ::ssize_t res = read(val, len, nullptr);
			if (res < 0) {
				return false;
			}
			reslen = std::size_t(res);
			return reslen == len;
		}

		[[nodiscard]]
		bool read(void* const val, const std::size_t len) const noexcept {
			std::size_t reslen{0};
			return read(val, len, reslen);
		}

		[[nodiscard]]
		bool write(const void* const val, const std::size_t len) const noexcept {
			const ::ssize_t res = write(val, len, nullptr);
			if (res < 0) {
				return false;
			}
			return std::size_t(res) == len;
		}

		template<typename T>
		bool read(T& val) const noexcept {
			return read(&val, sizeof(T));
		}
		template<typename T>
		bool write(const T& val) const noexcept {
			return write(&val, sizeof(T));
		}
		template<typename T>
		bool read(std::unique_ptr<T>& val) const noexcept {
			return read(val.get(), sizeof(T));
		}
		template<typename T>
		bool read(const std::unique_ptr<T>& val) const noexcept {
			return read(val.get(), sizeof(T));
		}
		template<typename T>
		bool write(const std::unique_ptr<T>& val) const noexcept {
			return write(val.get(), sizeof(T));
		}
		template<typename T>
		bool read(const std::unique_ptr<T[]>& val, const std::size_t len) const noexcept {
			return read(val.get(), sizeof(T) * len);
		}
		template<typename T>
		bool write(const std::unique_ptr<T[]>& val, const std::size_t len) const noexcept {
			return write(val.get(), sizeof(T) * len);
		}
		template<typename T, size_t N>
		bool read(std::array<T, N>& val) const noexcept {
			return read(val.data(), sizeof(T) * N);
		}
		template<typename T, size_t N>
		bool write(const std::array<T, N>& val) const noexcept {
			return write(val.data(), sizeof(T) * N);
		}

		bool write(const std::string& val) const noexcept {
			return write(val.data(), val.size());
		}

		bool write(const std::string_view& val) const noexcept {
			return write(val.data(), val.size());
		}

		template<size_t len, typename T, size_t N>
		bool read(std::array<T, N>& val) const noexcept {
			static_assert(len <= N, "Can't request to read more than the std::array<> len");
			return read(val.data(), sizeof(T) * len);
		}

		template<size_t len, typename T, size_t N>
		bool write(const std::array<T, N>& val) const noexcept {
			static_assert(len <= N, "Can't request to write more than the std::array<> len");
			return write(val.data(), sizeof(T) * len);
		}


		[[nodiscard]]
		sycophant::mmap_t map(const std::int32_t prot, const std::int32_t flags = MAP_SHARED) noexcept {
			const auto len{length()};
			if (len <= 0) {
				return {};
			}
			return map(prot, static_cast<std::size_t>(len), flags);
		}

		[[nodiscard]]
		sycophant::mmap_t map(const prot_t prot, const std::int32_t flags = MAP_SHARED) noexcept {
			const auto len{length()};
			if (len <= 0) {
				return {};
			}
			return map(prot, static_cast<std::size_t>(len), flags);
		}

		[[nodiscard]]
		sycophant::mmap_t map(const std::int32_t prot, const std::size_t len, const std::int32_t flags, void* addr = nullptr) noexcept {
			if (!valid()) {
				return {};
			}
			const std::int32_t file = _fd;
			invalidate();
			return {file, len, prot, flags, addr};
		}


		[[nodiscard]]
		sycophant::mmap_t map(const prot_t prot, const std::size_t len, const std::int32_t flags, void* addr = nullptr) noexcept {
			if (!valid()) {
				return {};
			}
			const std::int32_t file = _fd;
			invalidate();
			return {file, len, prot, flags, addr};
		}

		fd_t(const fd_t&) = delete;
		fd_t &operator=(const fd_t&) = delete;
	};

	inline void swap(fd_t& a, fd_t& b) noexcept {
		a.swap(b);
	}


}

#endif /* SYCOPHANT_FD_HH */
