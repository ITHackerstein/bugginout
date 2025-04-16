#include "Error.hpp"
#include "Parser.hpp"
#include "Typechecker.hpp"
#include "utils/Result.hpp"

#include <fmt/core.h>

Result<void, bo::Error> my_main() {
	auto source = R"(
fn find(haystack: []i32, needle: i32): isize {
	mut i = 0_isize;
	for (element in haystack) {
		if (element == needle) {
			return i;
		}

		++i;
	}

	-1
}

fn main(): void {
	var array = [1, 2, 3, 4, 5];
	var index = find(haystack: array, needle: [1, 2, 3, 4, 5][2]);
}
)"sv;

	auto parser = TRY(bo::Parser::create(source));
	auto program = TRY(parser.parse_program());
	program->dump();
	fmt::println("");
	bo::Typechecker typechecker;
	TRY(typechecker.check(program));
	typechecker.program().dump();
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
