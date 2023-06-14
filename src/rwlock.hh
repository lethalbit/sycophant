// SPDX-License-Identifier: BSD-3-Clause
/* rwlock.hh - A Read-many-write-one lock */
#pragma once
#if !defined(SYCOPHANT_RWLOCK_HH)
#define SYCOPHANT_RWLOCK_HH

#include <utility>
#include <mutex>
#include <shared_mutex>


namespace sycophant {

	template<typename T>
	struct rwlock_t final {
	private:
		template<typename U, template<typename> typename lock_t>
		struct lockresult_t final {
		private:
			lock_t<std::shared_mutex> _lock;
			U& _obj;
		public:
			constexpr lockresult_t(std::shared_mutex& mut, U& obj) noexcept :
				_lock{mut}, _obj{obj} { }

			[[nodiscard]]
			U* operator->() noexcept { return &_obj; }
			[[nodiscard]]
			U& operator*() noexcept { return _obj; }
		};

		using lockro_t = lockresult_t<const T, std::shared_lock>;
		using lockrw_t = lockresult_t<T, std::unique_lock>;

		std::shared_mutex _mutex{};
		T _obj;
	public:
		template<typename ...args_t>
		constexpr rwlock_t(args_t&& ...args) noexcept :
			_obj{std::forward<args_t>(args)...} { }

		[[nodiscard]]
		lockro_t read() noexcept {
			return {_mutex, _obj};
		}

		[[nodiscard]]
		lockrw_t write() noexcept {
			return {_mutex, _obj};
		}
	};
}

#endif /* SYCOPHANT_RWLOCK_HH */
