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

	template<typename Iterator>
	bool advance_if_any_of_is_next(Iterator first, Iterator last);

	Result<Token, Error> lex_integer_literal();
	Result<Token, Error> lex_char_literal();
	Result<Token, Error> lex_identifier_or_keyword();

	std::string_view m_source;
	char m_current_character;
	std::size_t m_current_position;
};

}
