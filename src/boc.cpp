#include "Error.hpp"
#include "Parser.hpp"
#include "utils/Result.hpp"

#include <fmt/core.h>

Result<void, bo::Error> my_main() {
	auto source = R"(
fn find(haystack: [100]i32, needle: i32): isize {
	for (i in 0..<100) {
		if (haystack[i] == needle) {
			return i;
		}
	}

	-1
}

fn main(): i32 {
	var arr = [1, 2, 3];
	var index = find(arr, 2);

	0
}
)"sv;

	auto parser = TRY(bo::Parser::create(source));
	auto program = TRY(parser.parse_program());
	program->dump();
	return {};
}

int main() {
	auto result = my_main();
	if (result.is_error()) {
		// FIXME: Use custom formatter for Error
		auto error = result.release_error();
		fmt::println("Error: {}", error.message());
		fmt::println("Span: {}", error.span());
		return 1;
	}

	return 0;
}
