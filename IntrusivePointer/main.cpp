import std;
import intrusive_ptr;

import ptr_tracking;

void test() {
	using namespace std::string_literals;
	auto vec = std::vector<intrusive_ptr<std::string>>{};

	vec.emplace_back("Hello World");

	auto c = vec.emplace_back(17, 'c');

	vec.emplace_back("End of the world as we know it!"s);

	vec.push_back("THIS IS SPARTA");

	auto empty = intrusive_ptr<std::string>{ "" };
	vec.push_back(empty);
	*empty += "this string was created empty";

	//intentional copies
	for (auto ptr : vec) {
		std::println("{}", *ptr);
	}

	std::println("{}", *c);
}

static void test_tracking() {
	using namespace std::string_literals;
	auto vec = std::vector<std::shared_ptr<std::string>>{};

	vec.push_back(std::make_shared<std::string>("Hello World"));

	auto c = vec.emplace_back(tracked::make_tracked_ptr<std::string>(17, 'c'));

	vec.emplace_back(tracked::make_tracked_ptr<std::string>("End of the world as we know it!"s));

	vec.push_back(tracked::make_tracked_ptr<std::string>("THIS IS SPARTA"));

	auto empty = tracked::make_tracked_ptr<std::string>();
	vec.push_back(empty);
	*empty += "this string was created empty";

	//intentional copies
	for (auto ptr : vec) {
		std::println("{}", *ptr);
	}

	std::println("{}", *c);
}

int main() {
	test_tracking();

	//test();

	////Is there a way to avoid this?
	//stop_garbage_collector();
}