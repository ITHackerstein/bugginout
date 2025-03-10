#pragma once

#include "Token.hpp"

#include <array>
#include <cassert>

namespace bo {

consteval std::array<unsigned, Token::count()> generate_precedence_table() {
	std::array<unsigned, Token::count()> table;
	table.fill(0);

	table[static_cast<std::size_t>(Token::Type::Equals)] = 1;
	table[static_cast<std::size_t>(Token::Type::PlusEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::MinusEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::AsteriskEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::SolidusEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::PercentEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::LeftShiftEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::RightShiftEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::AmpersandEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::CircumflexEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::PipeEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::DoubleAmpersandEquals)] = 1;
	table[static_cast<std::size_t>(Token::Type::DoublePipeEquals)] = 1;

	table[static_cast<std::size_t>(Token::Type::DoublePipe)] = 2;

	table[static_cast<std::size_t>(Token::Type::DoubleAmpersand)] = 3;

	table[static_cast<std::size_t>(Token::Type::Pipe)] = 4;

	table[static_cast<std::size_t>(Token::Type::Circumflex)] = 5;

	table[static_cast<std::size_t>(Token::Type::Ampersand)] = 6;

	table[static_cast<std::size_t>(Token::Type::DoubleEquals)] = 7;
	table[static_cast<std::size_t>(Token::Type::ExclamationMarkEquals)] = 7;

	table[static_cast<std::size_t>(Token::Type::LessThan)] = 8;
	table[static_cast<std::size_t>(Token::Type::GreaterThan)] = 8;
	table[static_cast<std::size_t>(Token::Type::LessThanEquals)] = 8;
	table[static_cast<std::size_t>(Token::Type::GreaterThanEquals)] = 8;

	table[static_cast<std::size_t>(Token::Type::LeftShift)] = 9;
	table[static_cast<std::size_t>(Token::Type::RightShift)] = 9;

	table[static_cast<std::size_t>(Token::Type::Plus)] = 10;
	table[static_cast<std::size_t>(Token::Type::Minus)] = 10;

	table[static_cast<std::size_t>(Token::Type::Asterisk)] = 11;
	table[static_cast<std::size_t>(Token::Type::Solidus)] = 11;
	table[static_cast<std::size_t>(Token::Type::Percent)] = 11;

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
