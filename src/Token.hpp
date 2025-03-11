#pragma once

#include <string_view>

#include "Span.hpp"

using namespace std::literals;

#define _BO_ENUMERATE_KEYWORDS \
	BO_ENUMERATE_KEYWORD(anon)   \
	BO_ENUMERATE_KEYWORD(bool)   \
	BO_ENUMERATE_KEYWORD(char)   \
	BO_ENUMERATE_KEYWORD(else)   \
	BO_ENUMERATE_KEYWORD(false)  \
	BO_ENUMERATE_KEYWORD(fn)     \
	BO_ENUMERATE_KEYWORD(for)    \
	BO_ENUMERATE_KEYWORD(i16)    \
	BO_ENUMERATE_KEYWORD(i32)    \
	BO_ENUMERATE_KEYWORD(i64)    \
	BO_ENUMERATE_KEYWORD(i8)     \
	BO_ENUMERATE_KEYWORD(if)     \
	BO_ENUMERATE_KEYWORD(in)     \
	BO_ENUMERATE_KEYWORD(isize)  \
	BO_ENUMERATE_KEYWORD(mut)    \
	BO_ENUMERATE_KEYWORD(null)   \
	BO_ENUMERATE_KEYWORD(true)   \
	BO_ENUMERATE_KEYWORD(u16)    \
	BO_ENUMERATE_KEYWORD(u32)    \
	BO_ENUMERATE_KEYWORD(u64)    \
	BO_ENUMERATE_KEYWORD(u8)     \
	BO_ENUMERATE_KEYWORD(usize)  \
	BO_ENUMERATE_KEYWORD(var)

#define _BO_ENUMERATE_TOKENS                \
	_BO_ENUMERATE_KEYWORDS                    \
	BO_ENUMERATE_TOKEN(Ampersand)             \
	BO_ENUMERATE_TOKEN(AmpersandEquals)       \
	BO_ENUMERATE_TOKEN(Asterisk)              \
	BO_ENUMERATE_TOKEN(AsteriskEquals)        \
	BO_ENUMERATE_TOKEN(At)                    \
	BO_ENUMERATE_TOKEN(BinaryLiteral)         \
	BO_ENUMERATE_TOKEN(CharLiteral)           \
	BO_ENUMERATE_TOKEN(Circumflex)            \
	BO_ENUMERATE_TOKEN(CircumflexEquals)      \
	BO_ENUMERATE_TOKEN(Colon)                 \
	BO_ENUMERATE_TOKEN(Comma)                 \
	BO_ENUMERATE_TOKEN(DecimalLiteral)        \
	BO_ENUMERATE_TOKEN(DoubleAmpersand)       \
	BO_ENUMERATE_TOKEN(DoubleAmpersandEquals) \
	BO_ENUMERATE_TOKEN(DoubleEquals)          \
	BO_ENUMERATE_TOKEN(DoublePipe)            \
	BO_ENUMERATE_TOKEN(DoublePipeEquals)      \
	BO_ENUMERATE_TOKEN(EndOfFile)             \
	BO_ENUMERATE_TOKEN(Equals)                \
	BO_ENUMERATE_TOKEN(ExclamationMark)       \
	BO_ENUMERATE_TOKEN(ExclamationMarkEquals) \
	BO_ENUMERATE_TOKEN(GreaterThan)           \
	BO_ENUMERATE_TOKEN(GreaterThanEquals)     \
	BO_ENUMERATE_TOKEN(HexadecimalLiteral)    \
	BO_ENUMERATE_TOKEN(Identifier)            \
	BO_ENUMERATE_TOKEN(LeftCurlyBracket)      \
	BO_ENUMERATE_TOKEN(LeftParenthesis)       \
	BO_ENUMERATE_TOKEN(LeftShift)             \
	BO_ENUMERATE_TOKEN(LeftShiftEquals)       \
	BO_ENUMERATE_TOKEN(LessThan)              \
	BO_ENUMERATE_TOKEN(LessThanEquals)        \
	BO_ENUMERATE_TOKEN(Minus)                 \
	BO_ENUMERATE_TOKEN(MinusEquals)           \
	BO_ENUMERATE_TOKEN(MinusMinus)            \
	BO_ENUMERATE_TOKEN(OctalLiteral)          \
	BO_ENUMERATE_TOKEN(Percent)               \
	BO_ENUMERATE_TOKEN(PercentEquals)         \
	BO_ENUMERATE_TOKEN(Pipe)                  \
	BO_ENUMERATE_TOKEN(PipeEquals)            \
	BO_ENUMERATE_TOKEN(Plus)                  \
	BO_ENUMERATE_TOKEN(PlusEquals)            \
	BO_ENUMERATE_TOKEN(PlusPlus)              \
	BO_ENUMERATE_TOKEN(RightCurlyBracket)     \
	BO_ENUMERATE_TOKEN(RightParenthesis)      \
	BO_ENUMERATE_TOKEN(RightShift)            \
	BO_ENUMERATE_TOKEN(RightShiftEquals)      \
	BO_ENUMERATE_TOKEN(Semicolon)             \
	BO_ENUMERATE_TOKEN(Solidus)               \
	BO_ENUMERATE_TOKEN(SolidusEquals)         \
	BO_ENUMERATE_TOKEN(Tilde)

namespace bo {

class Token {
public:
	enum class Type {
#define BO_ENUMERATE_KEYWORD(x) BO_ENUMERATE_TOKEN(KW_##x)
#define BO_ENUMERATE_TOKEN(x) x,
		_BO_ENUMERATE_TOKENS
#undef BO_ENUMERATE_TOKEN
#undef BO_ENUMERATE_KEYWORD
		  __COUNT
	};

	static constexpr std::size_t count() { return static_cast<std::size_t>(Token::Type::__COUNT); }

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

auto format_as(Token::Type) -> std::string;
auto format_as(Token) -> std::string;

}
