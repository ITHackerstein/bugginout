#pragma once

#include "Token.hpp"

#include <array>
#include <cassert>

namespace bo {

consteval std::array<unsigned, Token::count()> generate_precedence_table() {
	std::array<unsigned, Token::count()> table;
	table.fill(0);

	table[static_cast<std::size_t>(Token::Type::Equal)] = 1;

	table[static_cast<std::size_t>(Token::Type::Plus)] = 2;
	table[static_cast<std::size_t>(Token::Type::Minus)] = 2;

	table[static_cast<std::size_t>(Token::Type::Asterisk)] = 3;
	table[static_cast<std::size_t>(Token::Type::Solidus)] = 3;
	table[static_cast<std::size_t>(Token::Type::Percent)] = 3;

	return table;
}

enum class Associativity {
	Left,
	Right
};

class OperatorData {
public:
	OperatorData() = delete;

	static constexpr unsigned precedence_of(Token::Type type) {
		auto precedence = s_precedence_table[static_cast<std::size_t>(type)];
		assert(precedence != 0 && "Invalid operator!");
		return precedence;
	}

	static constexpr Associativity associativity_of(Token::Type type) {
		switch (type) {
		case Token::Type::Plus:
		case Token::Type::Minus:
		case Token::Type::Asterisk:
		case Token::Type::Solidus:
		case Token::Type::Percent:
			return Associativity::Left;
		default:
			return Associativity::Right;
		}
	}

private:
	static constexpr auto s_precedence_table = generate_precedence_table();
};

}
