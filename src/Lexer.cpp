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

bool Lexer::is_eof() const {
	return m_current_character == -1;
}

bool Lexer::is_line_comment_start() const {
	return m_current_character == '/' && m_current_position < m_source.size() && m_source[m_current_position] == '/';
}

bool Lexer::is_block_comment_start() const {
	return m_current_character == '/' && m_current_position < m_source.size() && m_source[m_current_position] == '*';
}

bool Lexer::is_block_comment_end() const {
	return m_current_character == '*' && m_current_position < m_source.size() && m_source[m_current_position] == '/';
}

bool Lexer::is_identifier_start() const {
	return std::isalpha(m_current_character) || m_current_character == '_' || m_current_character == '$';
}

bool Lexer::is_identifier_middle() const {
	return is_identifier_start() || std::isdigit(m_current_character);
}

Result<Token, Error> Lexer::lex_integer_literal() {
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

	return Token { token_type, m_source.substr(token_start, m_current_position - token_start - 1), { token_start, m_current_position - 2 } };
}

Result<Token, Error> Lexer::lex_char_literal() {
	auto token_start = m_current_position - 1;
	advance();

	if (is_eof()) {
		return Error { "unexpected end of file while parsing char literal", { token_start, m_current_position - 1 } };
	}

	if (m_current_character == '\'') {
		return Error { "empty char literals are not valid", { token_start, m_current_position - 1 } };
	}

	if (m_current_character == '\n' || m_current_character == '\r' || m_current_character == '\t') {
		return Error { "unexpected character inside char literal", { token_start, m_current_position - 1 } };
	}

	auto is_escape_sequence_next = m_current_character == '\\';
	advance();

	if (is_escape_sequence_next) {
		auto escape_sequence_start = m_current_position - 2;

		if (m_current_character == '\'' || m_current_character == 'n' || m_current_character == 'r' || m_current_character == 't' || m_current_character == '\\' || m_current_character == '0') {
			advance();
		} else if (m_current_character == 'x') {
			advance();

			if (m_current_character < '0' || m_current_character > '7') {
				return Error { "invalid escape sequence inside char literal", { escape_sequence_start, m_current_position - 1 } };
			}

			advance();

			if (!std::isxdigit(m_current_character)) {
				return Error { "invalid escape sequence inside char literal", { escape_sequence_start, m_current_position - 1 } };
			}

			advance();
		} else {
			return Error { "invalid escape sequence inside char literal", { escape_sequence_start, m_current_position - 1 } };
		}
	}

	if (m_current_character != '\'') {
		return Error { "missing closing single quote for char literal", { token_start, m_current_position - 1 } };
	}

	advance();

	return Token { Token::Type::CharLiteral, m_source.substr(token_start, m_current_position - token_start - 1), { token_start, m_current_position - 2 } };
}

Result<Token, Error> Lexer::lex_identifier_or_keyword() {
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

	return Token { token_type, token_value, { token_start, m_current_position - 2 } };
}

Result<Token, Error> Lexer::next_token() {
	while (true) {
		if (std::isspace(m_current_character)) {
			do {
				advance();
			} while (std::isspace(m_current_character));
		} else if (is_line_comment_start()) {
			advance();

			do {
				advance();
			} while (!is_eof() && m_current_character != '\n');
		} else if (is_block_comment_start()) {
			advance();

			do {
				advance();
			} while (!is_eof() && !is_block_comment_end());

			advance();
			advance();
		} else {
			break;
		}
	}

	if (is_eof()) {
		return Token { Token::Type::EndOfFile, ""sv, { m_current_position, m_current_position } };
	} else if (std::isdigit(m_current_character)) {
		return lex_integer_literal();
	} else if (m_current_character == '\'') {
		return lex_char_literal();
	} else if (is_identifier_start()) {
		return lex_identifier_or_keyword();
	} else if (m_current_character == '&') {
		advance();
		return Token { Token::Type::Ampersand, "&"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '*') {
		advance();
		return Token { Token::Type::Asterisk, "*"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '@') {
		advance();
		return Token { Token::Type::At, "@"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '^') {
		advance();
		return Token { Token::Type::Circumflex, "^"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == ':') {
		advance();
		return Token { Token::Type::Colon, ":"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == ',') {
		advance();
		return Token { Token::Type::Comma, ","sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '=') {
		advance();
		return Token { Token::Type::Equal, "="sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '{') {
		advance();
		return Token { Token::Type::LeftCurlyBracket, "{"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '(') {
		advance();
		return Token { Token::Type::LeftParenthesis, "("sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '-') {
		advance();
		return Token { Token::Type::Minus, "-"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '%') {
		advance();
		return Token { Token::Type::Percent, "%"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '+') {
		advance();
		return Token { Token::Type::Plus, "+"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '}') {
		advance();
		return Token { Token::Type::RightCurlyBracket, "}"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == ')') {
		advance();
		return Token { Token::Type::RightParenthesis, ")"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == ';') {
		advance();
		return Token { Token::Type::Semicolon, ";"sv, { m_current_position - 2, m_current_position - 2 } };
	} else if (m_current_character == '/') {
		advance();
		return Token { Token::Type::Solidus, "/"sv, { m_current_position - 2, m_current_position - 2 } };
	} else {
		return Error { "unexpected character while lexing", Span { m_current_position - 1, m_current_position - 1 } };
	}
}

}
