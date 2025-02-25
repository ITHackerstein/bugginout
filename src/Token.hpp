#pragma once

#include <string_view>

#include "Span.hpp"

using namespace std::literals;

#define _BO_ENUMERATE_KEYWORDS \
	BO_ENUMERATE_KEYWORD(anon)   \
	BO_ENUMERATE_KEYWORD(char)   \
	BO_ENUMERATE_KEYWORD(fn)     \
	BO_ENUMERATE_KEYWORD(i16)    \
	BO_ENUMERATE_KEYWORD(i32)    \
	BO_ENUMERATE_KEYWORD(i64)    \
	BO_ENUMERATE_KEYWORD(i8)     \
	BO_ENUMERATE_KEYWORD(isize)  \
	BO_ENUMERATE_KEYWORD(null)   \
	BO_ENUMERATE_KEYWORD(u16)    \
	BO_ENUMERATE_KEYWORD(u32)    \
	BO_ENUMERATE_KEYWORD(u64)    \
	BO_ENUMERATE_KEYWORD(u8)

#define _BO_ENUMERATE_TOKENS             \
	_BO_ENUMERATE_KEYWORDS                 \
	BO_ENUMERATE_TOKEN(Ampersand)          \
	BO_ENUMERATE_TOKEN(Asterisk)           \
	BO_ENUMERATE_TOKEN(At)                 \
	BO_ENUMERATE_TOKEN(BinaryLiteral)      \
	BO_ENUMERATE_TOKEN(CharLiteral)        \
	BO_ENUMERATE_TOKEN(Circumflex)         \
	BO_ENUMERATE_TOKEN(Colon)              \
	BO_ENUMERATE_TOKEN(Comma)              \
	BO_ENUMERATE_TOKEN(DecimalLiteral)     \
	BO_ENUMERATE_TOKEN(EndOfFile)          \
	BO_ENUMERATE_TOKEN(Equal)              \
	BO_ENUMERATE_TOKEN(HexadecimalLiteral) \
	BO_ENUMERATE_TOKEN(Identifier)         \
	BO_ENUMERATE_TOKEN(LeftCurlyBracket)   \
	BO_ENUMERATE_TOKEN(LeftParenthesis)    \
	BO_ENUMERATE_TOKEN(Minus)              \
	BO_ENUMERATE_TOKEN(OctalLiteral)       \
	BO_ENUMERATE_TOKEN(Percent)            \
	BO_ENUMERATE_TOKEN(Plus)               \
	BO_ENUMERATE_TOKEN(RightCurlyBracket)  \
	BO_ENUMERATE_TOKEN(RightParenthesis)   \
	BO_ENUMERATE_TOKEN(Semicolon)          \
	BO_ENUMERATE_TOKEN(Solidus)

namespace boc {

class Token {
public:
	enum class Type {
#define BO_ENUMERATE_KEYWORD(x) BO_ENUMERATE_TOKEN(KW_##x)
#define BO_ENUMERATE_TOKEN(x) x,
		_BO_ENUMERATE_TOKENS
#undef BO_ENUMERATE_TOKEN
#undef BO_ENUMERATE_KEYWORD
	};

	explicit Token(Type type, std::string_view value, Span span)
	  : m_type(type), m_value(value), m_span(span) {}

	Type type() const { return m_type; }
	std::string_view value() const { return m_value; }
	Span span() const { return m_span; }

private:
	Type m_type;
	std::string_view m_value;
	Span m_span;
};

auto format_as(Token) -> std::string;

}
