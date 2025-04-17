#include "Error.hpp"
#include "Parser.hpp"
#include "Transpiler.hpp"
#include "Typechecker.hpp"
#include "utils/Result.hpp"

#include <fmt/core.h>

Result<void, bo::Error> my_main() {
	auto source = R"(
fn main(): void {
	for (i in 0..<10) {
		i;
	}
}
)"sv;

	auto parser = TRY(bo::Parser::create(source));
	auto program = TRY(parser.parse_program());
	bo::Typechecker typechecker;
	TRY(typechecker.check(program));
	bo::Transpiler transpiler(typechecker.program());
	auto code = TRY(transpiler.transpile());
	fmt::print("{}", code);
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
