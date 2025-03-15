#include "Lexer.hpp"

#include <cctype>
#include <cstdlib>

namespace bo {

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

Result<Token::Type, Error> Lexer::lex_integer_literal() {
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
		if (!is_identifier_start()) {
			return Error { "unexpected character while parsing integer literal suffix", Span { m_current_position - 1, m_current_position - 1 } };
		}

		lex_identifier();
	}

	return token_type;
}

Result<void, Error> Lexer::lex_char_literal() {
	assert(m_current_character == '\'');

	auto token_start = m_current_position - 1;
	advance();

	if (is_eof()) {
		return Error { "unexpected end of file while parsing char literal", Span { token_start, m_current_position - 1 } };
	}

	if (m_current_character == '\'') {
		return Error { "empty char literals are not valid", Span { token_start, m_current_position - 1 } };
	}

	if (m_current_character == '\n' || m_current_character == '\r' || m_current_character == '\t') {
		return Error { "unexpected character inside char literal", Span { token_start, m_current_position - 1 } };
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
				return Error { "invalid escape sequence inside char literal", Span { escape_sequence_start, m_current_position - 1 } };
			}

			advance();

			if (!std::isxdigit(m_current_character)) {
				return Error { "invalid escape sequence inside char literal", Span { escape_sequence_start, m_current_position - 1 } };
			}

			advance();
		} else {
			return Error { "invalid escape sequence inside char literal", Span { escape_sequence_start, m_current_position - 1 } };
		}
	}

	if (m_current_character != '\'') {
		return Error { "missing closing single quote for char literal", Span { token_start, m_current_position - 1 } };
	}
	advance();

	return {};
}

void Lexer::lex_identifier() {
	assert(is_identifier_start());

	do {
		advance();
	} while (is_identifier_middle());
}

Result<Token::Type, Error> Lexer::lex_operator() {
	if (m_current_character == '&') {
		advance();
		if (m_current_character == '&') {
			advance();
			if (m_current_character == '=') {
				advance();
				return Token::Type::DoubleAmpersandEquals;
			}
			return Token::Type::DoubleAmpersand;
		}
		if (m_current_character == '=') {
			advance();
			return Token::Type::AmpersandEquals;
		}
		return Token::Type::Ampersand;
	}

	if (m_current_character == '*') {
		advance();
		if (m_current_character == '=') {
			advance();
			return Token::Type::AsteriskEquals;
		}
		return Token::Type::Asterisk;
	}

	if (m_current_character == '@') {
		advance();
		return Token::Type::At;
	}

	if (m_current_character == '^') {
		advance();
		if (m_current_character == '=') {
			advance();
			return Token::Type::CircumflexEquals;
		}
		return Token::Type::Circumflex;
	}

	if (m_current_character == ':') {
		advance();
		return Token::Type::Colon;
	}

	if (m_current_character == ',') {
		advance();
		return Token::Type::Comma;
	}

	if (m_current_character == '=') {
		advance();
		if (m_current_character == '=') {
			advance();
			return Token::Type::DoubleEquals;
		}
		return Token::Type::Equals;
	}

	if (m_current_character == '{') {
		advance();
		return Token::Type::LeftCurlyBracket;
	}

	if (m_current_character == '(') {
		advance();
		return Token::Type::LeftParenthesis;
	}

	if (m_current_character == '-') {
		advance();
		if (m_current_character == '-') {
			advance();
			return Token::Type::MinusMinus;
		}
		if (m_current_character == '=') {
			advance();
			return Token::Type::MinusEquals;
		}
		return Token::Type::Minus;
	}

	if (m_current_character == '%') {
		advance();
		if (m_current_character == '=') {
			advance();
			return Token::Type::PercentEquals;
		}
		return Token::Type::Percent;
	}

	if (m_current_character == '+') {
		advance();
		if (m_current_character == '+') {
			advance();
			return Token::Type::PlusPlus;
		}
		if (m_current_character == '=') {
			advance();
			return Token::Type::PlusEquals;
		}
		return Token::Type::Plus;
	}

	if (m_current_character == '}') {
		advance();
		return Token::Type::RightCurlyBracket;
	}

	if (m_current_character == ')') {
		advance();
		return Token::Type::RightParenthesis;
	}

	if (m_current_character == ';') {
		advance();
		return Token::Type::Semicolon;
	}

	if (m_current_character == '/') {
		advance();
		if (m_current_character == '=') {
			advance();
			return Token::Type::SolidusEquals;
		}
		return Token::Type::Solidus;
	}

	if (m_current_character == '!') {
		advance();
		if (m_current_character == '=') {
			advance();
			return Token::Type::ExclamationMarkEquals;
		}
		return Token::Type::ExclamationMark;
	}

	if (m_current_character == '~') {
		advance();
		return Token::Type::Tilde;
	}

	if (m_current_character == '<') {
		advance();
		if (m_current_character == '<') {
			advance();
			if (m_current_character == '=') {
				advance();
				return Token::Type::LeftShiftEquals;
			}
			return Token::Type::LeftShift;
		}
		if (m_current_character == '=') {
			advance();
			return Token::Type::LessThanEquals;
		}
		return Token::Type::LessThan;
	}

	if (m_current_character == '>') {
		advance();
		if (m_current_character == '>') {
			advance();
			if (m_current_character == '=') {
				advance();
				return Token::Type::RightShiftEquals;
			}
			return Token::Type::RightShift;
		}
		if (m_current_character == '=') {
			advance();
			return Token::Type::GreaterThanEquals;
		}
		return Token::Type::GreaterThan;
	}

	if (m_current_character == '|') {
		advance();
		if (m_current_character == '|') {
			advance();
			if (m_current_character == '=') {
				advance();
				return Token::Type::DoublePipeEquals;
			}
			return Token::Type::DoublePipe;
		}
		return Token::Type::Pipe;
	}

	if (m_current_character == '.' && m_current_position < m_source.size() && m_source[m_current_position] == '.') {
		if (m_current_position + 1 < m_source.size() && m_source[m_current_position + 1] == '<') {
			advance();
			advance();
			advance();
			return Token::Type::DotDotLessThan;
		} else if (m_current_position + 1 < m_source.size() && m_source[m_current_position + 1] == '=') {
			advance();
			advance();
			advance();
			return Token::Type::DotDotEquals;
		}
	}

	return Error { "unexpected character while lexing", Span { m_current_position - 1, m_current_position - 1 } };
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
		return Token { Token::Type::EndOfFile, ""sv, Span { m_current_position, m_current_position } };
	}

	Token::Type token_type;
	auto token_start = m_current_position - 1;

	if (std::isdigit(m_current_character)) {
		token_type = TRY(lex_integer_literal());
	} else if (m_current_character == '\'') {
		TRY(lex_char_literal());
		token_type = Token::Type::CharLiteral;
	} else if (is_identifier_start()) {
		lex_identifier();
		token_type = Token::Type::Identifier;
	} else {
		token_type = TRY(lex_operator());
	}

	auto token_value = m_source.substr(token_start, m_current_position - token_start - 1);

	if (token_type == Token::Type::Identifier) {
#define BO_ENUMERATE_KEYWORD(x)       \
	if (token_value == #x##sv) {        \
		token_type = Token::Type::KW_##x; \
	}
		_BO_ENUMERATE_KEYWORDS
#undef BO_ENUMERATE_KEYWORD
	}

	return Token { token_type, token_value, Span { token_start, m_current_position - 2 } };
}

}
