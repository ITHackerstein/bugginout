#pragma once

#include "Token.hpp"

#include <array>
#include <cassert>

namespace bo {

consteval std::array<unsigned, Token::count()> generate_precedence_table() {
	std::array<unsigned, Token::count()> table;
	table.fill(0);

	unsigned p = 1;
	{
		table[static_cast<std::size_t>(Token::Type::Equals)] = p;
		table[static_cast<std::size_t>(Token::Type::PlusEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::MinusEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::AsteriskEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::SolidusEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::PercentEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::LeftShiftEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::RightShiftEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::AmpersandEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::CircumflexEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::PipeEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::DoubleAmpersandEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::DoublePipeEquals)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::DotDotEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::DotDotLessThan)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::DoublePipe)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::DoubleAmpersand)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::DoubleEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::ExclamationMarkEquals)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::LessThan)] = p;
		table[static_cast<std::size_t>(Token::Type::GreaterThan)] = p;
		table[static_cast<std::size_t>(Token::Type::LessThanEquals)] = p;
		table[static_cast<std::size_t>(Token::Type::GreaterThanEquals)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::Pipe)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::Circumflex)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::Ampersand)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::LeftShift)] = p;
		table[static_cast<std::size_t>(Token::Type::RightShift)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::Plus)] = p;
		table[static_cast<std::size_t>(Token::Type::Minus)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::Asterisk)] = p;
		table[static_cast<std::size_t>(Token::Type::Solidus)] = p;
		table[static_cast<std::size_t>(Token::Type::Percent)] = p;
		++p;
	}

	{
		// NOTE: Skipping unary &
		table[static_cast<std::size_t>(Token::Type::At)] = p;
		table[static_cast<std::size_t>(Token::Type::Tilde)] = p;
		table[static_cast<std::size_t>(Token::Type::ExclamationMark)] = p;
		// NOTE: Skipping unary +, -
		table[static_cast<std::size_t>(Token::Type::PlusPlus)] = p;
		table[static_cast<std::size_t>(Token::Type::MinusMinus)] = p;
		++p;
	}

	{
		table[static_cast<std::size_t>(Token::Type::LeftParenthesis)] = p;
		table[static_cast<std::size_t>(Token::Type::LeftSquareBracket)] = p;
		++p;
	}

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

	static constexpr unsigned unary_precedence_of(Token::Type type) {
		// FIXME: Should probably look for a better way of storing those values.
		switch (type) {
		case Token::Type::Ampersand:
		case Token::Type::Plus:
		case Token::Type::Minus:
			return 13;
		default:
			return precedence_of(type);
		}
	}

	static constexpr Associativity associativity_of(Token::Type type) {
		switch (type) {
		case Token::Type::PlusPlus:
		case Token::Type::MinusMinus:
		case Token::Type::Asterisk:
		case Token::Type::Solidus:
		case Token::Type::Percent:
		case Token::Type::Plus:
		case Token::Type::Minus:
		case Token::Type::LeftShift:
		case Token::Type::RightShift:
		case Token::Type::LessThan:
		case Token::Type::GreaterThan:
		case Token::Type::LessThanEquals:
		case Token::Type::GreaterThanEquals:
		case Token::Type::DoubleEquals:
		case Token::Type::ExclamationMarkEquals:
		case Token::Type::Ampersand:
		case Token::Type::Circumflex:
		case Token::Type::Pipe:
		case Token::Type::DoubleAmpersand:
		case Token::Type::DoublePipe:
			return Associativity::Left;
		default:
			return Associativity::Right;
		}
	}

private:
	static constexpr auto s_precedence_table = generate_precedence_table();
};

}
