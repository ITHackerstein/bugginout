#include "Parser.hpp"

#include "OperatorData.hpp"

#include <fmt/core.h>
#include <memory>

namespace bo {

Result<Parser, Error> Parser::create(std::string_view source) {
	Lexer lexer { source };
	auto current_token = TRY(lexer.next_token());
	return Parser { std::move(lexer), std::move(current_token) };
}

template<typename Fn>
auto Parser::restrict(Fn fn, int restrictions) {
	auto previous_restrictions = m_restrictions;
	m_restrictions = restrictions;
	auto result = fn();
	m_restrictions = previous_restrictions;
	return std::move(result);
}

Result<void, Error> Parser::consume(std::optional<Token::Type> token_type) {
	if (token_type && m_current_token.type() != *token_type) {
		return Error { fmt::format("Expected {:?}, got {:?}!", *token_type, m_current_token.type()), m_current_token.span() };
	}

	m_current_token = TRY(m_lexer.next_token());
	return {};
}

Result<std::shared_ptr<AST::Program const>, Error> Parser::parse_program() {
	std::vector<std::shared_ptr<AST::FunctionDeclarationStatement const>> functions;

	Span span { 0, 0 };
	while (m_current_token.type() != Token::Type::EndOfFile) {
		auto function_declaration = TRY(parse_function_declaration_statement());
		span = Span::merge(span, function_declaration->span());
		functions.push_back(std::move(function_declaration));
	}

	// FIXME: Should check if contains 'main' function
	return std::make_shared<AST::Program const>(std::move(functions), span);
}

bool Parser::match_secondary_expression() const {
	auto type = m_current_token.type();
	return type == Token::Type::PlusPlus
	  || type == Token::Type::MinusMinus
	  || type == Token::Type::Asterisk
	  || type == Token::Type::Solidus
	  || type == Token::Type::Percent
	  || type == Token::Type::Plus
	  || type == Token::Type::Minus
	  || type == Token::Type::LeftParenthesis
	  || type == Token::Type::LeftShift
	  || type == Token::Type::LeftSquareBracket
	  || type == Token::Type::RightShift
	  || type == Token::Type::LessThan
	  || type == Token::Type::GreaterThan
	  || type == Token::Type::LessThanEquals
	  || type == Token::Type::GreaterThanEquals
	  || type == Token::Type::DoubleEquals
	  || type == Token::Type::ExclamationMarkEquals
	  || type == Token::Type::Ampersand
	  || type == Token::Type::Pipe
	  || type == Token::Type::DoubleAmpersand
	  || type == Token::Type::DoublePipe
	  || type == Token::Type::Equals
	  || type == Token::Type::PlusEquals
	  || type == Token::Type::MinusEquals
	  || type == Token::Type::AsteriskEquals
	  || type == Token::Type::SolidusEquals
	  || type == Token::Type::PercentEquals
	  || type == Token::Type::LeftShiftEquals
	  || type == Token::Type::RightShiftEquals
	  || type == Token::Type::AmpersandEquals
	  || type == Token::Type::CircumflexEquals
	  || type == Token::Type::PipeEquals
	  || type == Token::Type::DotDotEquals
	  || type == Token::Type::DotDotLessThan
	  || type == Token::Type::DoubleAmpersandEquals
	  || type == Token::Type::DoublePipeEquals;
}

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_unary_expression() {
#define MAKE_UNARY_EXPRESSION(operator_)                                                                                                                           \
	{                                                                                                                                                                \
		auto span = m_current_token.span();                                                                                                                            \
		TRY(consume());                                                                                                                                                \
		auto operand = TRY(parse_primary_expression());                                                                                                                \
		span = Span::merge(span, operand->span());                                                                                                                     \
		return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::UnaryExpression const>(std::move(operand), AST::UnaryOperator::operator_, span)); \
	}

#define MAKE_UPDATE_EXPRESSION(operator_)                                                                                                                                  \
	{                                                                                                                                                                        \
		auto span = m_current_token.span();                                                                                                                                    \
		TRY(consume());                                                                                                                                                        \
		auto operand = TRY(parse_primary_expression());                                                                                                                        \
		span = Span::merge(span, operand->span());                                                                                                                             \
		return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::UpdateExpression const>(std::move(operand), AST::UpdateOperator::operator_, true, span)); \
	}

	switch (m_current_token.type()) {
	case Token::Type::PlusPlus:
		MAKE_UPDATE_EXPRESSION(Increment);
	case Token::Type::MinusMinus:
		MAKE_UPDATE_EXPRESSION(Decrement);
	case Token::Type::Plus:
		MAKE_UNARY_EXPRESSION(Positive);
	case Token::Type::Minus:
		MAKE_UNARY_EXPRESSION(Negative);
	case Token::Type::ExclamationMark:
		MAKE_UNARY_EXPRESSION(LogicalNot);
	case Token::Type::Tilde:
		MAKE_UNARY_EXPRESSION(BitwiseNot);
	case Token::Type::At:
		{
			auto span = m_current_token.span();
			TRY(consume());
			auto operand = TRY(parse_primary_expression());
			span = Span::merge(span, operand->span());
			return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::PointerDereferenceExpression const>(std::move(operand), span));
		}
	case Token::Type::Ampersand:
		{
			auto span = m_current_token.span();
			TRY(consume());
			auto operand = TRY(parse_primary_expression());
			span = Span::merge(span, operand->span());
			return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::AddressOfExpression const>(std::move(operand), span));
		}
	default:
		assert(false && "Should not be here!");
	}

#undef MAKE_UPDATE_EXPRESSION
#undef MAKE_UNARY_EXPRESSION
}

bool Parser::match_unary_expression() const {
	switch (m_current_token.type()) {
	case Token::Type::PlusPlus:
	case Token::Type::MinusMinus:
	case Token::Type::Plus:
	case Token::Type::Minus:
	case Token::Type::ExclamationMark:
	case Token::Type::Tilde:
	case Token::Type::At:
	case Token::Type::Ampersand:
		return true;
	default:
		return false;
	}
}

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_primary_expression() {
	if (match_unary_expression()) {
		return TRY(parse_unary_expression());
	}

	switch (m_current_token.type()) {
	case Token::Type::Identifier:
		return std::static_pointer_cast<AST::Expression const>(TRY(parse_identifier()));
	case Token::Type::DecimalLiteral:
	case Token::Type::BinaryLiteral:
	case Token::Type::OctalLiteral:
	case Token::Type::HexadecimalLiteral:
		return std::static_pointer_cast<AST::Expression const>(TRY(parse_integer_literal()));
	case Token::Type::LeftParenthesis:
		{
			auto span = m_current_token.span();
			TRY(consume());
			auto expression = TRY(parse_expression());
			span = Span::merge(span, expression->span());
			span = Span::merge(span, m_current_token.span());
			TRY(consume(Token::Type::RightParenthesis));

			return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::ParenthesizedExpression const>(std::move(expression), span));
		}
	case Token::Type::LeftSquareBracket:
		return std::static_pointer_cast<AST::Expression const>(TRY(parse_array_expression()));
	case Token::Type::LeftCurlyBracket:
		return std::static_pointer_cast<AST::Expression const>(TRY(parse_block_expression()));
	case Token::Type::KW_if:
		return std::static_pointer_cast<AST::Expression const>(TRY(parse_if_expression()));
	default:
		assert(false);
	}
}

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_secondary_expression(std::shared_ptr<AST::Expression const> lhs, unsigned minimum_precedence) {
#define MAKE_BINARY_EXPRESSION(operator_)                                                                                                                                        \
	{                                                                                                                                                                              \
		TRY(consume());                                                                                                                                                              \
		auto rhs = TRY(parse_expression_inner(minimum_precedence));                                                                                                                  \
		auto span = Span::merge(lhs->span(), rhs->span());                                                                                                                           \
		return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::BinaryExpression const>(std::move(lhs), std::move(rhs), AST::BinaryOperator::operator_, span)); \
	}

#define MAKE_ASSIGNMENT_EXPRESSION(operator_)                                                                                                                                            \
	{                                                                                                                                                                                      \
		TRY(consume());                                                                                                                                                                      \
		auto rhs = TRY(parse_expression_inner(minimum_precedence));                                                                                                                          \
		auto span = Span::merge(lhs->span(), rhs->span());                                                                                                                                   \
		return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::AssignmentExpression const>(std::move(lhs), std::move(rhs), AST::AssignmentOperator::operator_, span)); \
	}

#define MAKE_RANGE_EXPRESSION(is_inclusive)                                                                                                                     \
	{                                                                                                                                                             \
		TRY(consume());                                                                                                                                             \
		auto rhs = TRY(parse_expression_inner(minimum_precedence));                                                                                                 \
		auto span = Span::merge(lhs->span(), rhs->span());                                                                                                          \
		return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::RangeExpression const>(std::move(lhs), std::move(rhs), (is_inclusive), span)); \
	}

#define MAKE_UPDATE_EXPRESSION(operator_)                                                                                                                               \
	{                                                                                                                                                                     \
		auto span = Span::merge(m_current_token.span(), lhs->span());                                                                                                       \
		TRY(consume());                                                                                                                                                     \
		return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::UpdateExpression const>(std::move(lhs), AST::UpdateOperator::operator_, false, span)); \
	}

	switch (m_current_token.type()) {
	case Token::Type::PlusPlus:
		MAKE_UPDATE_EXPRESSION(Increment);
	case Token::Type::MinusMinus:
		MAKE_UPDATE_EXPRESSION(Decrement);
	case Token::Type::Asterisk:
		MAKE_BINARY_EXPRESSION(Multiplication);
	case Token::Type::Solidus:
		MAKE_BINARY_EXPRESSION(Division);
	case Token::Type::Percent:
		MAKE_BINARY_EXPRESSION(Modulo);
	case Token::Type::Plus:
		MAKE_BINARY_EXPRESSION(Addition);
	case Token::Type::Minus:
		MAKE_BINARY_EXPRESSION(Subtraction);
	case Token::Type::LeftParenthesis:
		{
			if (!lhs->is_identifier()) {
				return Error { "Expected identifier before function call", lhs->span() };
			}

			auto identifier = std::static_pointer_cast<AST::Identifier const>(lhs);
			return std::static_pointer_cast<AST::Expression const>(TRY(parse_function_call_expression(std::move(identifier))));
		}
	case Token::Type::LeftShift:
		MAKE_BINARY_EXPRESSION(BitwiseLeftShift);
	case Token::Type::LeftSquareBracket:
		{
			auto span = Span::merge(lhs->span(), m_current_token.span());
			TRY(consume());

			auto subscript = TRY(parse_expression());
			span = Span::merge(span, subscript->span());

			span = Span::merge(span, m_current_token.span());
			TRY(consume(Token::Type::RightSquareBracket));

			return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::ArraySubscriptExpression const>(std::move(lhs), std::move(subscript), span));
		}
	case Token::Type::RightShift:
		MAKE_BINARY_EXPRESSION(BitwiseRightShift);
	case Token::Type::LessThan:
		MAKE_BINARY_EXPRESSION(LessThan);
	case Token::Type::GreaterThan:
		MAKE_BINARY_EXPRESSION(GreaterThan);
	case Token::Type::LessThanEquals:
		MAKE_BINARY_EXPRESSION(LessThanOrEqualTo);
	case Token::Type::GreaterThanEquals:
		MAKE_BINARY_EXPRESSION(GreaterThanOrEqualTo);
	case Token::Type::DoubleEquals:
		MAKE_BINARY_EXPRESSION(EqualTo);
	case Token::Type::ExclamationMarkEquals:
		MAKE_BINARY_EXPRESSION(NotEqualTo);
	case Token::Type::Ampersand:
		MAKE_BINARY_EXPRESSION(BitwiseAnd);
	case Token::Type::Circumflex:
		MAKE_BINARY_EXPRESSION(BitwiseXor);
	case Token::Type::Pipe:
		MAKE_BINARY_EXPRESSION(BitwiseOr);
	case Token::Type::DotDotEquals:
		MAKE_RANGE_EXPRESSION(true);
	case Token::Type::DotDotLessThan:
		MAKE_RANGE_EXPRESSION(false);
	case Token::Type::DoubleAmpersand:
		MAKE_BINARY_EXPRESSION(LogicalAnd);
	case Token::Type::DoublePipe:
		MAKE_BINARY_EXPRESSION(LogicalOr);
	case Token::Type::Equals:
		MAKE_ASSIGNMENT_EXPRESSION(Assignment);
	case Token::Type::PlusEquals:
		MAKE_ASSIGNMENT_EXPRESSION(AdditionAssignment);
	case Token::Type::MinusEquals:
		MAKE_ASSIGNMENT_EXPRESSION(SubtractionAssignment);
	case Token::Type::AsteriskEquals:
		MAKE_ASSIGNMENT_EXPRESSION(MultiplicationAssignment);
	case Token::Type::SolidusEquals:
		MAKE_ASSIGNMENT_EXPRESSION(DivisionAssignment);
	case Token::Type::PercentEquals:
		MAKE_ASSIGNMENT_EXPRESSION(ModuloAssignment);
	case Token::Type::LeftShiftEquals:
		MAKE_ASSIGNMENT_EXPRESSION(BitwiseLeftShiftAssignment);
	case Token::Type::RightShiftEquals:
		MAKE_ASSIGNMENT_EXPRESSION(BitwiseRightShiftAssignment);
	case Token::Type::AmpersandEquals:
		MAKE_ASSIGNMENT_EXPRESSION(BitwiseAndAssignment);
	case Token::Type::CircumflexEquals:
		MAKE_ASSIGNMENT_EXPRESSION(BitwiseXorAssignment);
	case Token::Type::PipeEquals:
		MAKE_ASSIGNMENT_EXPRESSION(BitwiseOrAssignment);
	case Token::Type::DoubleAmpersandEquals:
		MAKE_ASSIGNMENT_EXPRESSION(LogicalAndAssignment);
	case Token::Type::DoublePipeEquals:
		MAKE_ASSIGNMENT_EXPRESSION(LogicalOrAssignment);
	default:
		assert(false && "Should not be here!");
	}
}

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_expression_inner(unsigned minimum_precedence) {
	auto result = TRY(parse_primary_expression());

	if (m_restrictions & R_NoExpressionsWithBlocks && result->has_block()) {
		if (match_unary_expression()) {
			return result;
		}

		if (match_secondary_expression()) {
			return Error { "Expression needs parenthesis!", result->span() };
		}
	}

	while (match_secondary_expression()) {
		auto operator_token = m_current_token;
		auto operator_precedence = OperatorData::precedence_of(operator_token.type());
		if (operator_precedence < minimum_precedence) {
			break;
		}

		if (OperatorData::associativity_of(operator_token.type()) == Associativity::Left) {
			++operator_precedence;
		}

		result = TRY(parse_secondary_expression(std::move(result), operator_precedence));
	}

	return result;
}

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_expression() {
	return parse_expression_with_restrictions(R_None);
}

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_expression_with_restrictions(int restrictions) {
	return restrict([this]() { return parse_expression_inner(0); }, restrictions);
}

Result<std::shared_ptr<AST::Statement const>, Error> Parser::parse_statement() {
	switch (m_current_token.type()) {
	case Token::Type::KW_var:
	case Token::Type::KW_mut:
		return std::static_pointer_cast<AST::Statement const>(TRY(parse_variable_declaration_statement()));
	case Token::Type::KW_for:
		return std::static_pointer_cast<AST::Statement const>(TRY(parse_for_statement()));
	case Token::Type::KW_return:
		return std::static_pointer_cast<AST::Statement const>(TRY(parse_return_statement()));
	default:
		{
			auto expression = TRY(parse_expression_with_restrictions(R_NoExpressionsWithBlocks));
			if (m_current_token.type() == Token::Type::Semicolon) {
				auto span = Span::merge(m_current_token.span(), expression->span());
				TRY(consume());

				return std::static_pointer_cast<AST::Statement const>(std::make_shared<AST::ExpressionStatement const>(std::move(expression), true, span));
			}

			if (expression->has_block() || m_current_token.type() == Token::Type::RightCurlyBracket) {
				return std::static_pointer_cast<AST::Statement const>(std::make_shared<AST::ExpressionStatement const>(std::move(expression), false, expression->span()));
			}

			return Error { "Expected semicolon after expression", expression->span() };
		}
	}
}

Result<std::shared_ptr<AST::BlockExpression const>, Error> Parser::parse_block_expression() {
	auto span = m_current_token.span();
	TRY(consume(Token::Type::LeftCurlyBracket));

	std::vector<std::shared_ptr<AST::Statement const>> statements;
	while (m_current_token.type() != Token::Type::RightCurlyBracket) {
		statements.push_back(TRY(parse_statement()));
	}

	span = Span::merge(span, m_current_token.span());
	TRY(consume(Token::Type::RightCurlyBracket));

	return std::make_shared<AST::BlockExpression const>(std::move(statements), span);
}

Result<std::shared_ptr<AST::IfExpression const>, Error> Parser::parse_if_expression() {
	auto span = m_current_token.span();

	TRY(consume(Token::Type::KW_if));

	TRY(consume(Token::Type::LeftParenthesis));
	auto condition = TRY(parse_expression());
	TRY(consume(Token::Type::RightParenthesis));
	span = Span::merge(span, condition->span());

	auto then = TRY(parse_block_expression());
	span = Span::merge(span, then->span());

	if (m_current_token.type() == Token::Type::KW_else) {
		TRY(consume());
		if (m_current_token.type() == Token::Type::KW_if) {
			auto else_if = TRY(parse_if_expression());
			span = Span::merge(span, else_if->span());
			return std::make_shared<AST::IfExpression const>(std::move(condition), std::move(then), std::move(else_if), span);
		}

		auto else_ = TRY(parse_block_expression());
		span = Span::merge(span, else_->span());
		return std::make_shared<AST::IfExpression const>(std::move(condition), std::move(then), std::move(else_), span);
	}

	return std::make_shared<AST::IfExpression const>(std::move(condition), std::move(then), nullptr, span);
}

Result<std::shared_ptr<AST::ArrayExpression const>, Error> Parser::parse_array_expression() {
	auto span = m_current_token.span();
	TRY(consume(Token::Type::LeftSquareBracket));

	std::vector<std::shared_ptr<AST::Expression const>> elements;
	while (m_current_token.type() != Token::Type::RightSquareBracket) {
		auto element = TRY(parse_expression());
		span = Span::merge(span, element->span());

		elements.push_back(std::move(element));

		if (m_current_token.type() != Token::Type::Comma) {
			break;
		}

		span = Span::merge(span, m_current_token.span());
		TRY(consume());
	}

	span = Span::merge(span, m_current_token.span());
	TRY(consume(Token::Type::RightSquareBracket));

	return std::make_shared<AST::ArrayExpression const>(std::move(elements), span);
}

Result<std::shared_ptr<AST::FunctionCallExpression const>, Error> Parser::parse_function_call_expression(std::shared_ptr<AST::Identifier const> function_name) {
	auto span = Span::merge(function_name->span(), m_current_token.span());
	TRY(consume(Token::Type::LeftParenthesis));

	std::vector<AST::FunctionArgument> arguments;
	while (m_current_token.type() != Token::Type::RightParenthesis) {
		auto argument = TRY(parse_expression());

		std::shared_ptr<AST::Identifier const> argument_name = nullptr;
		std::shared_ptr<AST::Expression const> argument_value = nullptr;

		if (argument->is_identifier()) {
			argument_name = std::static_pointer_cast<AST::Identifier const>(argument);

			if (m_current_token.type() == Token::Type::Colon) {
				TRY(consume());
				argument_value = TRY(parse_expression());
			} else {
				argument_value = argument_name;
			}
		} else {
			argument_value = argument;
		}

		arguments.emplace_back(argument_name, argument_value);

		if (m_current_token.type() != Token::Type::Comma) {
			break;
		}
		TRY(consume());
	}

	span = Span::merge(span, m_current_token.span());
	TRY(consume(Token::Type::RightParenthesis));

	return std::make_shared<AST::FunctionCallExpression const>(std::move(function_name), std::move(arguments), span);
}

Result<std::shared_ptr<AST::ForStatement const>, Error> Parser::parse_for_statement() {
	auto span = m_current_token.span();

	TRY(consume(Token::Type::KW_for));
	if (m_current_token.type() == Token::Type::LeftParenthesis) {
		TRY(consume());
		auto condition = TRY(parse_expression());
		span = Span::merge(span, condition->span());

		if (m_current_token.type() == Token::Type::KW_in) {
			TRY(consume());
			if (!condition->is_identifier()) {
				return Error { "Expected identifier in for-in loop!", condition->span() };
			}

			auto identifier = std::static_pointer_cast<AST::Identifier const>(condition);
			auto range_expression = TRY(parse_expression());
			span = Span::merge(span, range_expression->span());

			TRY(consume(Token::Type::RightParenthesis));

			auto body = TRY(parse_block_expression());
			span = Span::merge(span, body->span());
			return std::static_pointer_cast<AST::ForStatement const>(std::make_shared<AST::ForWithRangeStatement const>(std::move(identifier), std::move(range_expression), std::move(body), span));
		}

		TRY(consume(Token::Type::RightParenthesis));
		auto body = TRY(parse_block_expression());
		span = Span::merge(span, body->span());
		return std::static_pointer_cast<AST::ForStatement const>(std::make_shared<AST::ForWithConditionStatement const>(std::move(condition), std::move(body), span));
	}

	auto body = TRY(parse_block_expression());
	span = Span::merge(span, body->span());
	return std::static_pointer_cast<AST::ForStatement const>(std::make_shared<AST::InfiniteForStatement const>(std::move(body), span));
}

Result<std::shared_ptr<AST::Type const>, Error> Parser::parse_type(bool allow_top_level_mut) {
	auto span = m_current_token.span();
	std::shared_ptr<AST::Type const> inner_type = nullptr;
	std::shared_ptr<AST::IntegerLiteral const> array_size = nullptr;
	std::shared_ptr<AST::Identifier const> name = nullptr;
	int flags = 0;

	if (m_current_token.type() == Token::Type::KW_mut) {
		if (!allow_top_level_mut) {
			return Error { "'mut' is not allowed here", m_current_token.span() };
		}

		span = Span::merge(span, m_current_token.span());
		TRY(consume());
		flags |= AST::PF_IsMutable;
	}

	if (m_current_token.type() == Token::Type::Asterisk || m_current_token.type() == Token::Type::Circumflex) {
		span = Span::merge(span, m_current_token.span());
		flags |= m_current_token.type() == Token::Type::Asterisk ? AST::PF_IsWeakPointer : AST::PF_IsStrongPointer;
		TRY(consume());

		inner_type = TRY(parse_type());
		span = Span::merge(span, inner_type->span());
	} else if (m_current_token.type() == Token::Type::LeftSquareBracket) {
		span = Span::merge(span, m_current_token.span());
		TRY(consume());

		if (m_current_token.type() != Token::Type::RightSquareBracket) {
			array_size = TRY(parse_integer_literal());
			span = Span::merge(span, m_current_token.span());

			flags |= AST::PF_IsArray;
		} else {
			flags |= AST::PF_IsSlice;
		}

		span = Span::merge(span, m_current_token.span());
		TRY(consume(Token::Type::RightSquareBracket));

		inner_type = TRY(parse_type());
		span = Span::merge(span, inner_type->span());
	} else {
		name = TRY(parse_identifier(true));
		span = Span::merge(span, name->span());
	}

	return std::make_shared<AST::Type const>(std::move(inner_type), std::move(array_size), std::move(name), flags, span);
}

Result<std::shared_ptr<AST::Identifier const>, Error> Parser::parse_identifier(bool allow_keywords) {
	// FIXME: Switch to something better
	if (m_current_token.type() != Token::Type::Identifier && (!allow_keywords || !m_current_token.is_keyword())) {
		if (!allow_keywords && m_current_token.is_keyword()) {
			return Error { fmt::format("Expected identifier, got {:?}!", m_current_token.value()), m_current_token.span() };
		}
	}

	auto identifier_value = m_current_token.value();
	auto identifier_span = m_current_token.span();
	TRY(consume());
	return std::make_shared<AST::Identifier const>(identifier_value, identifier_span);
}

Result<std::shared_ptr<AST::IntegerLiteral const>, Error> Parser::parse_integer_literal() {
	AST::IntegerLiteral::Type literal_type;
	switch (m_current_token.type()) {
	case Token::Type::DecimalLiteral:
		literal_type = AST::IntegerLiteral::Type::Decimal;
		break;
	case Token::Type::BinaryLiteral:
		literal_type = AST::IntegerLiteral::Type::Binary;
		break;
	case Token::Type::OctalLiteral:
		literal_type = AST::IntegerLiteral::Type::Octal;
		break;
	case Token::Type::HexadecimalLiteral:
		literal_type = AST::IntegerLiteral::Type::Hexadecimal;
		break;
	default:
		return Error { fmt::format("Expected integer literal, got {:?}!", m_current_token.value()), m_current_token.span() };
	}

	auto literal_value = m_current_token.value();
	auto literal_span = m_current_token.span();
	TRY(consume());

	auto literal_suffix_start = literal_value.find('_');
	auto literal_suffix = literal_suffix_start == std::string_view::npos ? ""sv : literal_value.substr(literal_suffix_start + 1);
	literal_value = literal_value.substr(0, literal_suffix_start);

	return std::make_shared<AST::IntegerLiteral const>(literal_value, literal_type, literal_suffix, literal_span);
}

Result<std::vector<AST::FunctionParameter>, Error> Parser::parse_function_parameters() {
	std::vector<AST::FunctionParameter> parameters;

	TRY(consume(Token::Type::LeftParenthesis));
	while (m_current_token.type() != Token::Type::RightParenthesis) {
		bool is_anonymous = false;
		if (m_current_token.type() == Token::Type::KW_anon) {
			is_anonymous = true;
			TRY(consume());
		}

		auto parameter_name = TRY(parse_identifier());
		TRY(consume(Token::Type::Colon));
		auto parameter_type = TRY(parse_type());
		parameters.emplace_back(std::move(parameter_name), std::move(parameter_type), is_anonymous);

		if (m_current_token.type() != Token::Type::Comma) {
			break;
		}

		TRY(consume());
	}
	TRY(consume(Token::Type::RightParenthesis));

	return parameters;
}

Result<std::shared_ptr<AST::FunctionDeclarationStatement const>, Error> Parser::parse_function_declaration_statement() {
	auto span = m_current_token.span();

	TRY(consume(Token::Type::KW_fn));
	auto function_name = TRY(parse_identifier());
	auto function_parameters = TRY(parse_function_parameters());
	TRY(consume(Token::Type::Colon));
	auto function_return_type = TRY(parse_type(false));
	auto function_body = TRY(parse_block_expression());

	span = Span::merge(span, m_current_token.span());

	return std::make_shared<AST::FunctionDeclarationStatement const>(function_name, function_parameters, function_return_type, function_body, span);
}

Result<std::shared_ptr<AST::VariableDeclarationStatement const>, Error> Parser::parse_variable_declaration_statement() {
	auto span = m_current_token.span();

	bool is_mutable = false;
	if (m_current_token.type() == Token::Type::KW_mut) {
		TRY(consume());
		is_mutable = true;
	} else {
		TRY(consume(Token::Type::KW_var));
	}

	auto identifier = TRY(parse_identifier());
	std::shared_ptr<AST::Type const> type = nullptr;
	std::shared_ptr<AST::Expression const> initializer = nullptr;

	if (m_current_token.type() == Token::Type::Equals) {
		TRY(consume());
		initializer = TRY(parse_expression());
	} else if (m_current_token.type() == Token::Type::Colon) {
		TRY(consume());
		type = TRY(parse_type(false));

		if (m_current_token.type() == Token::Type::Equals) {
			TRY(consume());
			initializer = TRY(parse_expression());
		}
	} else {
		return Error { fmt::format("Expected ':' or '=', got {:?}!", m_current_token.value()), m_current_token.span() };
	}

	span = Span::merge(span, m_current_token.span());
	TRY(consume(Token::Type::Semicolon));
	return std::make_shared<AST::VariableDeclarationStatement const>(is_mutable, std::move(identifier), std::move(type), std::move(initializer), span);
}

Result<std::shared_ptr<AST::ReturnStatement const>, Error> Parser::parse_return_statement() {
	auto span = m_current_token.span();
	TRY(consume(Token::Type::KW_return));

	if (m_current_token.type() == Token::Type::Semicolon) {
		span = Span::merge(span, m_current_token.span());
		TRY(consume());
		return std::make_shared<AST::ReturnStatement const>(nullptr, span);
	}

	auto expression = TRY(parse_expression());
	span = Span::merge(span, expression->span());

	span = Span::merge(span, m_current_token.span());
	TRY(consume(Token::Type::Semicolon));

	return std::make_shared<AST::ReturnStatement const>(std::move(expression), span);
}

}
