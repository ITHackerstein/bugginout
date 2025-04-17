#include "Error.hpp"
#include "Parser.hpp"
#include "Transpiler.hpp"
#include "Typechecker.hpp"
#include "utils/Result.hpp"

#include <fmt/core.h>

Result<void, bo::Error> my_main() {
	auto source = R"(
fn add(anon a: u32, anon b: u32): u32 { a + b }
fn add(anon a: i32, anon b: i32): i32 { a + b }

fn main(): void {
	print(add(1_u32, 2_u32));
	print(add(1_i32, 2_i32));
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
