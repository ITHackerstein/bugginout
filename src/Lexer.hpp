#pragma once

#include <string_view>

#include "Token.hpp"

namespace boc {

class Lexer {
public:
	explicit Lexer(std::string_view source);

	Token next_token();

private:
	void advance();

	bool is_eof() const { return m_current_character == -1; }

	template<typename Iterator>
	bool advance_if_any_of_is_next(Iterator first, Iterator last);

	bool is_identifier_start() const;
	bool is_identifier_middle() const;

	std::string_view m_source;
	char m_current_character;
	std::size_t m_current_position;
};

}
