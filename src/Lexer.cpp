#include "Lexer.hpp"

#include <array>
#include <cctype>
#include <cstdlib>

namespace bo {

constexpr std::array SUFFIXES = { "u8"sv, "i8"sv, "u16"sv, "i16"sv, "u32"sv, "i32"sv, "u64"sv, "i64"sv, "usize"sv, "isize"sv };

Lexer::Lexer(std::string_view source)
  : m_source(source), m_current_character(-1), m_current_position(0) {
	advance();
}

void Lexer::advance() {
	if (m_current_position > m_source.size()) {
		return;
	}

	m_current_character = m_current_position < m_source.size() ? m_source[m_current_position] : -1;
	++m_current_position;
}

template<typename Iterator>
bool Lexer::advance_if_any_of_is_next(Iterator first, Iterator last) {
	for (Iterator it = first; it != last; ++it) {
		auto needle = *it;

		if (m_source.size() + 1 >= m_current_position + needle.size() && m_source.substr(m_current_position - 1, needle.size()) == needle) {
			for (std::size_t _ = 0; _ < needle.size(); ++_) {
				advance();
			}

			return true;
		}
	}

	return false;
}

bool Lexer::is_identifier_start() const {
	return std::isalpha(m_current_character) || m_current_character == '_' || m_current_character == '$';
}

bool Lexer::is_identifier_middle() const {
	return is_identifier_start() || std::isdigit(m_current_character);
}

Token Lexer::next_token() {
	while (std::isspace(m_current_character)) {
		advance();
	}

	if (is_eof()) {
		return Token { Token::Type::EndOfFile, ""sv, { m_current_position, m_current_position } };
	} else if (std::isdigit(m_current_character)) {
		auto token_start = m_current_position - 1;

		auto prefix_might_be_next = m_current_character == '0';
		advance();

		auto token_type = Token::Type::DecimalLiteral;
		auto allowed_digits = "0123456789"sv;
		if (prefix_might_be_next) {
			switch (m_current_character) {
			case 'b':
				token_type = Token::Type::BinaryLiteral;
				allowed_digits = "01"sv;
				advance();
				break;
			case 'o':
				token_type = Token::Type::OctalLiteral;
				allowed_digits = "01234567"sv;
				advance();
				break;
			case 'x':
				token_type = Token::Type::HexadecimalLiteral;
				allowed_digits = "0123456789abcdef"sv;
				advance();
				break;
			}
		}

		while (allowed_digits.contains(std::tolower(m_current_character))) {
			advance();
		}

		if (m_current_character == '_') {
			advance();
			advance_if_any_of_is_next(SUFFIXES.begin(), SUFFIXES.end());
		}

		return Token { token_type, m_source.substr(token_start, m_current_position - token_start - 1), { token_start, m_current_position - 1 } };
	} else if (m_current_character == '\'') {
		auto token_start = m_current_position - 1;

		advance();

		if (is_eof()) {
			// FIXME: Handle unexpected end of file
		}

		if (m_current_character == '\'') {
			// FIXME: Handle error in this case, we can't have an empty character
		}

		if (m_current_character == '\n' || m_current_character == '\r' || m_current_character == '\t') {
			// FIXME: Handle error in this case, invalid characters inside char literal
		}

		auto is_escape_sequence_next = m_current_character == '\\';
		advance();

		if (is_escape_sequence_next) {
			if (m_current_character == '\'' || m_current_character == 'n' || m_current_character == 'r' || m_current_character == 't' || m_current_character == '\\' || m_current_character == '0') {
				advance();
			} else if (m_current_character == 'x') {
				advance();
			} else {
				// FIXME: Handle error in this case, invalid escape sequence
			}
		}

		if (m_current_character != '\'') {
			// FIXME: Handle error in this case, missing closing single quote
		}

		advance();

		return Token { Token::Type::CharLiteral, m_source.substr(token_start, m_current_position - token_start - 1), { token_start, m_current_position - 1 } };
	} else if (is_identifier_start()) {
		auto token_start = m_current_position - 1;
		auto token_type = Token::Type::Identifier;

		do {
			advance();
		} while (is_identifier_middle());

		auto token_value = m_source.substr(token_start, m_current_position - token_start - 1);
#define BO_ENUMERATE_KEYWORD(x)       \
	if (token_value == #x##sv) {        \
		token_type = Token::Type::KW_##x; \
	}
		_BO_ENUMERATE_KEYWORDS
#undef BO_ENUMERATE_KEYWORD

		return Token { token_type, token_value, { token_start, m_current_position - 1 } };
	} else if (m_current_character == '&') {
		advance();
		return Token { Token::Type::Ampersand, "&"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '*') {
		advance();
		return Token { Token::Type::Asterisk, "*"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '@') {
		advance();
		return Token { Token::Type::At, "@"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '^') {
		advance();
		return Token { Token::Type::Circumflex, "^"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == ':') {
		advance();
		return Token { Token::Type::Colon, ":"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == ',') {
		advance();
		return Token { Token::Type::Comma, ","sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '=') {
		advance();
		return Token { Token::Type::Equal, "="sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '{') {
		advance();
		return Token { Token::Type::LeftCurlyBracket, "{"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '(') {
		advance();
		return Token { Token::Type::LeftParenthesis, "("sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '-') {
		advance();
		return Token { Token::Type::Minus, "-"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '%') {
		advance();
		return Token { Token::Type::Percent, "%"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '+') {
		advance();
		return Token { Token::Type::Plus, "+"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '}') {
		advance();
		return Token { Token::Type::RightCurlyBracket, "}"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == ')') {
		advance();
		return Token { Token::Type::RightParenthesis, ")"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == ';') {
		advance();
		return Token { Token::Type::Semicolon, ";"sv, { m_current_position - 1, m_current_position - 1 } };
	} else if (m_current_character == '/') {
		advance();
		return Token { Token::Type::Solidus, "/"sv, { m_current_position - 1, m_current_position - 1 } };
	} else {
		// FIXME: Handle error in this case, unexpected character
		std::abort();
	}
}

}
