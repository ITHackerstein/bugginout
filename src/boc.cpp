#include "Error.hpp"
#include "Parser.hpp"
#include "utils/Result.hpp"

#include <fmt/core.h>

Result<void, bo::Error> my_main() {
	auto source = R"(
/* A function
   Takes two numbers and does operations */
fn f(a: u32, b: u32): u32 {
	for { ++c; }
	for (c < 2 * 3) { ++c; }
	// FIXME: Not really a range expression
	for (c in 0+10) { ++c; }
	if (a + b == (c >> 1) & 1) {
		--a;
	} else if (a - b == (c << 1) & 1) {
		++b;
	} else {
		++a;
	}
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
