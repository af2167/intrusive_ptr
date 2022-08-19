module;

#include <chrono>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

export module intrusive_ptr;

constexpr static std::chrono::milliseconds sleep_for_time{ 9 };

using start_time_t = decltype(std::chrono::high_resolution_clock::now());

//SINGLETON INSTANCE
class daemon_thread {
	std::jthread _t;
	std::mutex destroyers_mutex;
	std::vector<std::function<bool(const start_time_t&)>> destroyers;

	void garbage_collection_loop() {
		while (true) {
			{
				std::unique_lock lk{ destroyers_mutex };
				if (destroyers.size() > 0) {
					using namespace std::chrono;
					auto start = high_resolution_clock::now();
					for (auto&& can_continue_destroying : destroyers) {
						if (!can_continue_destroying(start)) break;
					}
				}
			}

			std::this_thread::sleep_for(sleep_for_time);
		}
	}

	daemon_thread() noexcept : _t{ [this]() { garbage_collection_loop(); } } {}

public:
	daemon_thread(const daemon_thread&) = delete;
	daemon_thread(daemon_thread&&) = delete;
	daemon_thread& operator=(const daemon_thread&) = delete;
	daemon_thread& operator=(daemon_thread&&) = delete;

	~daemon_thread() = default;

	template <typename Callback>
	void add_destroyer(Callback&& c) requires std::predicate<Callback, const start_time_t&> {
		std::unique_lock lk{ destroyers_mutex };
		destroyers.push_back(std::forward<Callback>(c));
	}

	static daemon_thread& instance() {
		static daemon_thread _instance{};
		return _instance;
	}
};

class create_underlying {};

export template <typename T>
class weak_intrusive_ref;

export template <typename T>
class intrusive_ptr {
	std::shared_ptr<T> _inner;

	template <typename ... Args>
	intrusive_ptr(create_underlying, Args&& ... args) : _inner{ std::forward<Args>(args)... } {}

public:
	friend class weak_intrusive_ref<T>;
	friend class std::deque<intrusive_ptr<T>>;

	intrusive_ptr() = delete;
	intrusive_ptr(const intrusive_ptr&) = default;
	intrusive_ptr(intrusive_ptr&&) = default;
	intrusive_ptr& operator=(const intrusive_ptr&) = default;
	intrusive_ptr& operator=(intrusive_ptr&&) = default;
	~intrusive_ptr() = default;
	template <typename U, typename Deleter>
	intrusive_ptr& operator=(std::unique_ptr<U, Deleter>&& ptr) {
		auto p = intrusive_ptr{ std::move(ptr) };
		_inner = std::move(p._inner);
		return *this;
	}

	template <typename ... Args>
	intrusive_ptr(Args&& ...);

	void swap(intrusive_ptr& other) {
		_inner.swap(other._inner);
	}

	auto get() const noexcept { return _inner.get(); }
	T& operator*() const noexcept { return *_inner; }
	T* operator->() const noexcept { return get(); }
	auto& operator[](std::ptrdiff_t i) const { return _inner[i]; }
	long use_count() const noexcept { return _inner.use_count() - 1; }
	explicit operator bool() const noexcept { return static_cast<bool>(_inner); }
	template <typename U>
	bool owner_before(const intrusive_ptr<U>& other) const noexcept { return _inner.owner_before(other._inner); }
	template <typename U>
	bool owner_before(const weak_intrusive_ref<U>& other) const noexcept { return _inner.owner_before(other._inner); }

	bool operator==(const intrusive_ptr<T>&) const = default;
	auto operator<=>(const intrusive_ptr<T>&) const = default;
};

export template <typename T>
class weak_intrusive_ref {
	std::weak_ptr<T> _inner;
public:
	friend class intrusive_ptr<T>;

	constexpr weak_intrusive_ref() noexcept = default;
	weak_intrusive_ref(const weak_intrusive_ref& r) noexcept = default;
	weak_intrusive_ref(weak_intrusive_ref&& r) noexcept = default;
	~weak_intrusive_ref() = default;
	weak_intrusive_ref& operator=(const weak_intrusive_ref& r) noexcept = default;
	weak_intrusive_ref& operator=(weak_intrusive_ref&& r) noexcept = default;

	template <typename U>
	weak_intrusive_ref(const weak_intrusive_ref<U>& r) noexcept : _inner{ r._inner } {}
	template <typename U>
	weak_intrusive_ref(const intrusive_ptr<U>& ptr) noexcept : _inner{ ptr._inner } {}
	template <typename U>
	weak_intrusive_ref(weak_intrusive_ref<U>&& r) noexcept : _inner{ std::move(r._inner) } {}

	template <typename U>
	weak_intrusive_ref& operator=(const weak_intrusive_ref<U>& r) noexcept {
		_inner = r._inner;
		return *this;
	}
	template <typename U>
	weak_intrusive_ref& operator=(const intrusive_ptr<U>& ptr) noexcept {
		_inner = ptr._inner;
		return *this;
	}
	template <typename U>
	weak_intrusive_ref& operator=(weak_intrusive_ref<U>&& r) noexcept {
		_inner = r._inner;
		return *this;
	}

	void swap(weak_intrusive_ref& other) noexcept { _inner.swap(other._inner); }

	long use_count() const noexcept { return _inner.use_count() - 1; }
	bool expired() const noexcept { return _inner.expired(); }
	intrusive_ptr<T> lock() const noexcept { return{ _inner.lock() }; }
	template <typename U>
	bool owner_before(const weak_intrusive_ref<U>& other) const noexcept { return _inner.owner_before(other._inner); }
	template <typename U>
	bool owner_before(const intrusive_ptr<U>& other) const noexcept { return _inner.owner_before(other._inner); }
};

//SINGLETON INSTANCE per type 'T'
template <typename T>
class add_destroyer_impl {
	std::mutex d_mutex{};
	std::deque<intrusive_ptr<T>> d{};

	constexpr static bool has_garbage_collection_time_remaining(const start_time_t& start) {
		using namespace std::chrono_literals;
		using namespace std::chrono;
		return duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - start) >= 1ms;
	}

	auto destroy_items_callback() {
		return [this](const start_time_t& start) {
			std::vector<intrusive_ptr<T>> erase_container;
			{
				std::unique_lock lk{ d_mutex };
				auto remove_at = std::remove_if(d.begin(), d.end(), [](const auto& p) { return p.use_count() <= 0; });
				if (remove_at != d.end()) {
					std::move(remove_at, d.end(), std::back_inserter{ erase_container });
					d.erase(remove_at, d.end());
				}
			}

			return true;
			/*std::unique_lock lk{ d_mutex };
			for (auto current = d.begin(); current != d.end(); ++current) {
				while (current->use_count() <= 0 && current != d.end()) {
					*current = std::move(d.back());
					d.pop_back();
					if (!has_garbage_collection_time_remaining(start)) return false;
				}

				if (!has_garbage_collection_time_remaining(start)) return false;
			}

			return true;*/
		};
	}

	constexpr add_destroyer_impl() {
		daemon_thread::instance().add_destroyer(destroy_items_callback());
	}

public:
	add_destroyer_impl(const add_destroyer_impl&) = delete;
	add_destroyer_impl(add_destroyer_impl&&) = delete;
	add_destroyer_impl& operator=(const add_destroyer_impl&) = delete;
	add_destroyer_impl& operator=(add_destroyer_impl&&) = delete;

	~add_destroyer_impl() = default;

	template <typename ... Args>
	auto operator()(Args&& ... args) {
		std::unique_lock lk{ d_mutex };
		return d.emplace_back(create_underlying{}, std::forward<decltype(args)>(args)...);
	}

	static add_destroyer_impl& instance() {
		static add_destroyer_impl _instance{};
		return _instance;
	}
};

export template <typename T, typename ... Args>
intrusive_ptr<T> make_intrusive_ptr(Args&& ... args) {
	return add_destroyer_impl<T>::instance()(std::forward<Args>(args)...);
}

template <typename T>
template <typename ... Args>
intrusive_ptr<T>::intrusive_ptr(Args&& ... args) : _inner{ std::move(make_intrusive_ptr<T>(std::forward<Args>(args)...)._inner) } {}
