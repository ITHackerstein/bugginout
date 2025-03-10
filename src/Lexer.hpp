#pragma once

#include <string_view>

#include "Error.hpp"
#include "Token.hpp"
#include "utils/Result.hpp"

namespace bo {

class Lexer {
public:
	explicit Lexer(std::string_view source);

	Result<Token, Error> next_token();

private:
	void advance();

	bool is_eof() const;
	bool is_line_comment_start() const;
	bool is_block_comment_start() const;
	bool is_block_comment_end() const;
	bool is_identifier_start() const;
	bool is_identifier_middle() const;

	Result<Token::Type, Error> lex_integer_literal();
	Result<void, Error> lex_char_literal();
	void lex_identifier();
	Result<Token::Type, Error> lex_operator();

	std::string_view m_source;
	char m_current_character;
	std::size_t m_current_position;
};

}
