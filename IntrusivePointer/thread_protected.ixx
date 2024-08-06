export module thread_protected;

import std;

export template <typename T, typename Mutex = std::mutex>
class thread_protected {
	T t_{};
	Mutex m_{};
public:
	~thread_protected() = default;
	thread_protected() = default;

	thread_protected(thread_protected&&) = delete;
	thread_protected(const thread_protected&) = delete;
	thread_protected& operator=(thread_protected&&) = delete;
	thread_protected& operator=(const thread_protected&) = delete;

	/*thread_protected(const T& t) : t_{ t } {}
	thread_protected(T&& t) : t_{ std::move(t) } {}

	template <typename ... Args>
	thread_protected(std::in_place_t, Args&& ... args) : t_{ std::forward<Args>(args)... } {}*/

	template <typename F>
	decltype(auto) run_locked(F&& f) {
		std::unique_lock lk{ m_ };
		return std::invoke(std::forward<F>(f), t_);
	}
};