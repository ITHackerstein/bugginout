#include "Error.hpp"
#include "Parser.hpp"
#include "utils/Result.hpp"

#include <fmt/core.h>

Result<void, bo::Error> my_main() {
	auto source = R"(
fn f(a: u32, b: u32): u32 {
	a + b
}

fn g(a: u32, b: u32, c: u32): u32 {
	f(f(f(a, b), c), 0x12)
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
