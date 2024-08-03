import std;
import intrusive_ptr;

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

int main() {
	test();

	//Is there a way to avoid this?
	stop_garbage_collector();
}