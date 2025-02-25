#include "Lexer.hpp"

#include <fmt/core.h>

int main() {
	boc::Lexer lexer { "fn add(anon a: u32, anon b: u32): u32 {\na * b + a - b / a;\n}\n\nfn main() { var c = add(1_u16, 0xdeadbeef_u32); }" };
	while (true) {
		auto token = lexer.next_token();
		fmt::print("{}\n", token);

		if (token.type() == boc::Token::Type::EndOfFile) {
			break;
		}
	}

	return 0;
}
