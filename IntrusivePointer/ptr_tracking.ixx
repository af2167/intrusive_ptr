export module ptr_tracking;

import std;
import thread_protected;

auto is_dead = [](const auto& p) { return p.use_count() <= 1; };

using container = std::vector<std::shared_ptr<void>>;

//SINGLETON INSTANCE
class daemon_thread {
	std::thread t_;
	std::atomic<bool> is_stopped_ = false;
	thread_protected<container> gen0;
	container gen1;
	container gen2;
	int gen1_skips = 0;
	int gen2_skips = 0;

	void collect() {
		container to_next_gen;

		gen0.run_locked([&to_next_gen](auto& vec) {
			to_next_gen.reserve(vec.capacity());
			vec.swap(to_next_gen);
		});

		std::erase_if(to_next_gen, is_dead);
		++gen1_skips;

		if (gen1_skips < 10) {
			std::ranges::move(to_next_gen, std::back_inserter(gen1));
			return;
		}

		gen1_skips = 0;

		to_next_gen.reserve(gen1.capacity());
		gen1.swap(to_next_gen);

		std::erase_if(to_next_gen, is_dead);
		++gen2_skips;

		std::ranges::move(to_next_gen, std::back_inserter(gen2));

		if (gen2_skips < 10) {
			return;
		}

		gen2_skips = 0;

		std::erase_if(gen2, is_dead);
	}

	void garbage_collection_loop() {
		using namespace std::literals::chrono_literals;

		while (!is_stopped_) {
			std::this_thread::sleep_for(1ms);
			collect();
		}
	}

	daemon_thread() noexcept : t_{ [this]() { garbage_collection_loop(); } } {}

public:
	daemon_thread(const daemon_thread&) = delete;
	daemon_thread(daemon_thread&&) = delete;
	daemon_thread& operator=(const daemon_thread&) = delete;
	daemon_thread& operator=(daemon_thread&&) = delete;

	~daemon_thread() {
		is_stopped_ = true;
		t_.join();
	}

	void add_tracked(std::shared_ptr<void>&& ptr) {
		gen0.run_locked([&ptr](auto& vec) {
			vec.push_back(std::move(ptr));
		});
	}

	static daemon_thread& instance() {
		static daemon_thread _instance{};
		return _instance;
	}
};

export namespace tracked {
	void add_to_tracking(std::shared_ptr<void> ptr) {
		daemon_thread::instance().add_tracked(std::move(ptr));
	}

	template <typename T, typename ... Args>
	auto make_tracked_ptr(Args&& ... args) {
		auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
		add_to_tracking(ptr);
		return ptr;
	}
}