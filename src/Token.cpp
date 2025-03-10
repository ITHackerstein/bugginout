#include "Token.hpp"

#include <fmt/core.h>

namespace bo {

auto format_as(Token::Type token_type) -> std::string {
	switch (token_type) {
#define BO_ENUMERATE_KEYWORD(x) BO_ENUMERATE_TOKEN(KW_##x)
#define BO_ENUMERATE_TOKEN(x) \
	case Token::Type::x:        \
		return #x##s;             \
		break;
		_BO_ENUMERATE_TOKENS
#undef BO_ENUMERATE_TOKEN
#undef BO_ENUMERATE_KEYWORD
	default:
		// NOTE: This can't happen
		return ""s;
	}
}

auto format_as(Token token) -> std::string {
	return fmt::format("Token {{ type: {}, value: {:?}, span: {} }}", token.type(), token.value(), token.span());
}

}
