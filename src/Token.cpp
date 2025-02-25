#include "Token.hpp"

#include <fmt/core.h>

namespace boc {

auto format_as(Token token) -> std::string {
	std::string_view token_type;
	switch (token.type()) {
#define BO_ENUMERATE_KEYWORD(x) BO_ENUMERATE_TOKEN(KW_##x)
#define BO_ENUMERATE_TOKEN(x) \
	case Token::Type::x:        \
		token_type = #x##sv;      \
		break;
		_BO_ENUMERATE_TOKENS
#undef BO_ENUMERATE_TOKEN
#undef BO_ENUMERATE_KEYWORD
	}

	return fmt::format("Token {{ type: {}, value: {:?}, span: {} }}", token_type, token.value(), token.span());
}

}
