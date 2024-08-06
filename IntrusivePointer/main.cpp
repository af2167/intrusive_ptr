import std;
import tracked_ptr;

import ptr_tracking;

static void test() {
	using namespace std::string_literals;
	auto vec = std::vector<tracked_ptr<std::string>>{};

	vec.emplace_back(make_intrusive_ptr<std::string>("Hello World"));

	//intentional copy
	auto c = vec.emplace_back(make_intrusive_ptr<std::string>(17, 'c'));

	vec.emplace_back(make_intrusive_ptr<std::string>("End of the world as we know it!"s));

	vec.push_back(make_intrusive_ptr<std::string>("THIS IS SPARTA"));

	auto empty = make_intrusive_ptr<std::string>("");
	vec.push_back(empty);
	*empty += "this string was created empty";

	//intentional copies
	for (auto ptr : vec) {
		std::println("{}", *ptr);
	}
	vec.clear();

	std::println("{}", *c);
}

static void test_tracking() {
	using namespace std::string_literals;
	auto vec = std::vector<std::shared_ptr<std::string>>{};

	vec.push_back(std::make_shared<std::string>("Hello World"));

	//intentional copy
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
	vec.clear();

	std::println("{}", *c);
}

int main() {
	test();

	test_tracking();
}