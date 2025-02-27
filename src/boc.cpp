#include "Lexer.hpp"

#include <fmt/core.h>

Result<void, bo::Error> my_main() {
	bo::Lexer lexer { "/* fn add(anon a: u32, anon b: u32): u32 {\na * b + a - b / a;\n}\n\nfn main() { var c = add(1_u16, 0xdeadbeef_u32); }*/" };
	while (true) {
		auto token = TRY(lexer.next_token());
		fmt::print("{}\n", token);

		if (token.type() == bo::Token::Type::EndOfFile) {
			break;
		}
	}

	return {};
}

int main() {
	auto result = my_main();
	if (result.is_error()) {
		// FIXME: Use custom formatter for Error
		auto error = result.release_error();
		fmt::println("Error: {}", error.message());
		return 1;
	}

	return 0;
}
