#include "Error.hpp"
#include "Parser.hpp"
#include "Typechecker.hpp"
#include "utils/Result.hpp"

#include <fmt/core.h>

Result<void, bo::Error> my_main() {
	auto source = R"(
fn main(): void {
	var a: i32 = 1_i32;
}
)"sv;

	auto parser = TRY(bo::Parser::create(source));
	auto program = TRY(parser.parse_program());
	program->dump();
	fmt::println("");
	bo::Typechecker typechecker { program };
	TRY(typechecker.check());
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
