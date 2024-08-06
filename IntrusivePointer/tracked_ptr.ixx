export module tracked_ptr;

import std;
import ptr_tracking;

export template <typename T>
class weak_intrusive_ref;

export template <typename T>
class tracked_ptr {
	std::shared_ptr<T> _inner;

public:
	friend class weak_intrusive_ref<T>;

	tracked_ptr(std::shared_ptr<T> ptr) : _inner{ std::move(ptr) } {
		tracked::add_to_tracking(_inner);
	}

	tracked_ptr() = delete;
	tracked_ptr(const tracked_ptr&) = default;
	tracked_ptr(tracked_ptr&&) = default;
	tracked_ptr& operator=(const tracked_ptr&) = default;
	tracked_ptr& operator=(tracked_ptr&&) = default;
	~tracked_ptr() = default;
	template <typename U, typename Deleter>
	tracked_ptr& operator=(std::unique_ptr<U, Deleter>&& ptr) {
		auto p = tracked_ptr{ std::move(ptr) };
		_inner = std::move(p._inner);
		return *this;
	}

	void swap(tracked_ptr& other) {
		_inner.swap(other._inner);
	}

	auto get() const noexcept { return _inner.get(); }
	T& operator*() const noexcept { return *_inner; }
	T* operator->() const noexcept { return get(); }
	auto& operator[](std::ptrdiff_t i) const { return _inner[i]; }
	long use_count() const noexcept { return _inner.use_count() - 1; }
	explicit operator bool() const noexcept { return static_cast<bool>(_inner); }
	template <typename U>
	bool owner_before(const tracked_ptr<U>& other) const noexcept { return _inner.owner_before(other._inner); }
	template <typename U>
	bool owner_before(const weak_intrusive_ref<U>& other) const noexcept { return _inner.owner_before(other._inner); }

	bool operator==(const tracked_ptr<T>&) const = default;
	auto operator<=>(const tracked_ptr<T>&) const = default;
};

export template <typename T>
class weak_intrusive_ref {
	std::weak_ptr<T> _inner;
public:
	friend class tracked_ptr<T>;

	weak_intrusive_ref() noexcept = default;
	weak_intrusive_ref(const weak_intrusive_ref& r) noexcept = default;
	weak_intrusive_ref(weak_intrusive_ref&& r) noexcept = default;
	~weak_intrusive_ref() = default;
	weak_intrusive_ref& operator=(const weak_intrusive_ref& r) noexcept = default;
	weak_intrusive_ref& operator=(weak_intrusive_ref&& r) noexcept = default;

	template <typename U>
	weak_intrusive_ref(const weak_intrusive_ref<U>& r) noexcept : _inner{ r._inner } {}
	template <typename U>
	weak_intrusive_ref(const tracked_ptr<U>& ptr) noexcept : _inner{ ptr._inner } {}
	template <typename U>
	weak_intrusive_ref(weak_intrusive_ref<U>&& r) noexcept : _inner{ std::move(r._inner) } {}

	template <typename U>
	weak_intrusive_ref& operator=(const weak_intrusive_ref<U>& r) noexcept {
		_inner = r._inner;
		return *this;
	}
	template <typename U>
	weak_intrusive_ref& operator=(const tracked_ptr<U>& ptr) noexcept {
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
	tracked_ptr<T> lock() const noexcept { return{ _inner.lock() }; }
	template <typename U>
	bool owner_before(const weak_intrusive_ref<U>& other) const noexcept { return _inner.owner_before(other._inner); }
	template <typename U>
	bool owner_before(const tracked_ptr<U>& other) const noexcept { return _inner.owner_before(other._inner); }
};

export template <typename T, typename ... Args>
auto make_intrusive_ptr(Args&& ... args) {
	return tracked_ptr<T>{ std::make_shared<T>(std::forward<Args>(args)...) };
}
