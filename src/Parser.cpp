#include "Parser.hpp"

#include "OperatorData.hpp"

#include <fmt/core.h>

namespace bo {

Result<Parser, Error> Parser::create(std::string_view source) {
	Lexer lexer { source };
	auto current_token = TRY(lexer.next_token());
	return Parser { std::move(lexer), std::move(current_token) };
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
		auto function_declaration = TRY(parse_function_declaration());
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
	  || type == Token::Type::LeftShift
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
                                                                                                                                                                           \
		auto operand                                                                                                                                                           \
		  = TRY(parse_primary_expression());                                                                                                                                   \
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
			auto operand = TRY(parse_primary_expression());
			span = Span::merge(span, operand->span());
			return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::PointerDereferenceExpression const>(std::move(operand), span));
		}
	case Token::Type::Ampersand:
		{
			auto span = m_current_token.span();
			auto operand = TRY(parse_primary_expression());
			span = Span::merge(span, operand->span());
			return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::AddressOfExpression const>(std::move(operand), span));
		}
	default:
		assert(false && "Should not be here!");
	}
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
			// FIXME: Should add the parenthesis to the span
			TRY(consume());
			auto expression = TRY(parse_expression());
			TRY(consume(Token::Type::RightParenthesis));
			return expression;
		}
	default:
		assert(false);
	}
}

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_secondary_expression(std::shared_ptr<AST::Expression const> lhs, Token const& operator_token, unsigned minimum_precedence) {
#define MAKE_BINARY_EXPRESSION(operator_)                                                                                                                                        \
	{                                                                                                                                                                              \
		auto rhs = TRY(parse_expression_inner(minimum_precedence));                                                                                                                  \
		auto span = Span::merge(lhs->span(), rhs->span());                                                                                                                           \
		return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::BinaryExpression const>(std::move(lhs), std::move(rhs), AST::BinaryOperator::operator_, span)); \
	}

#define MAKE_ASSIGNMENT_EXPRESSION(operator_)                                                                                                                                            \
	{                                                                                                                                                                                      \
		auto rhs = TRY(parse_expression_inner(minimum_precedence));                                                                                                                          \
		auto span = Span::merge(lhs->span(), rhs->span());                                                                                                                                   \
		return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::AssignmentExpression const>(std::move(lhs), std::move(rhs), AST::AssignmentOperator::operator_, span)); \
	}

	switch (operator_token.type()) {
	case Token::Type::PlusPlus:
		return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::UpdateExpression const>(std::move(lhs), AST::UpdateOperator::Increment, false, Span::merge(lhs->span(), operator_token.span())));
	case Token::Type::MinusMinus:
		return std::static_pointer_cast<AST::Expression const>(std::make_shared<AST::UpdateExpression const>(std::move(lhs), AST::UpdateOperator::Decrement, false, Span::merge(lhs->span(), operator_token.span())));
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
	case Token::Type::LeftShift:
		MAKE_BINARY_EXPRESSION(BitwiseLeftShift);
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

	while (match_secondary_expression()) {
		auto operator_token = m_current_token;
		auto operator_precedence = OperatorData::precedence_of(operator_token.type());
		if (operator_precedence < minimum_precedence) {
			break;
		}

		if (OperatorData::associativity_of(operator_token.type()) == Associativity::Left) {
			++operator_precedence;
		}

		TRY(consume());
		result = TRY(parse_secondary_expression(std::move(result), operator_token, operator_precedence));
	}

	return result;
}

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_expression() {
	switch (m_current_token.type()) {
	case Token::Type::LeftCurlyBracket:
		return std::static_pointer_cast<AST::Expression const>(TRY(parse_block_expression()));
	case Token::Type::KW_if:
		return std::static_pointer_cast<AST::Expression const>(TRY(parse_if_expression()));
	case Token::Type::KW_for:
		return std::static_pointer_cast<AST::Expression const>(TRY(parse_for_expression()));
	default:
		return parse_expression_inner(0);
	}
}

Result<std::shared_ptr<AST::Statement const>, Error> Parser::parse_statement() {
	switch (m_current_token.type()) {
	case Token::Type::KW_var:
		return std::static_pointer_cast<AST::Statement const>(TRY(parse_variable_declaration()));
	default:
		{
			auto expression = TRY(parse_expression());
			TRY(consume(Token::Type::Semicolon));
			auto expression_statement = std::make_shared<AST::ExpressionStatement const>(std::move(expression), expression->span());
			return std::static_pointer_cast<AST::Statement const>(std::move(expression_statement));
		}
	}
}

Result<std::shared_ptr<AST::BlockExpression const>, Error> Parser::parse_block_expression() {
	TRY(consume(Token::Type::LeftCurlyBracket));
	std::vector<std::shared_ptr<AST::Statement const>> statements;
	std::shared_ptr<AST::Expression const> last_expression;
	while (m_current_token.type() != Token::Type::RightCurlyBracket) {
		if (m_current_token.type() == Token::Type::KW_var) {
			statements.push_back(TRY(parse_variable_declaration()));
			continue;
		}

		last_expression = TRY(parse_expression());
		if (last_expression->can_be_statement_without_semicolon()) {
			statements.push_back(std::make_shared<AST::ExpressionStatement const>(std::move(last_expression), last_expression->span()));
			last_expression = nullptr;
		}

		if (m_current_token.type() == Token::Type::Semicolon) {
			TRY(consume());
			statements.push_back(std::make_shared<AST::ExpressionStatement const>(std::move(last_expression), last_expression->span()));
			last_expression = nullptr;
		}
	}
	TRY(consume(Token::Type::RightCurlyBracket));

	if (last_expression == nullptr) {
		auto last_statement = std::move(statements.back());
		statements.pop_back();

		auto span = last_statement->span();
		for (auto const& statement : statements) {
			span = Span::merge(span, statement->span());
		}
		return std::make_shared<AST::BlockExpression const>(std::move(statements), std::move(last_statement), span);
	} else {
		auto span = last_expression->span();
		for (auto const& statement : statements) {
			span = Span::merge(span, statement->span());
		}

		return std::make_shared<AST::BlockExpression const>(std::move(statements), std::move(last_expression), span);
	}
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

Result<std::shared_ptr<AST::ForExpression const>, Error> Parser::parse_for_expression() {
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
			// FIXME: Should define what a range expression is
			auto range_expression = TRY(parse_expression());
			span = Span::merge(span, range_expression->span());

			TRY(consume(Token::Type::RightParenthesis));

			auto body = TRY(parse_block_expression());
			span = Span::merge(span, body->span());
			return std::static_pointer_cast<AST::ForExpression const>(std::make_shared<AST::ForWithRangeExpression const>(std::move(identifier), std::move(range_expression), std::move(body), span));
		}

		TRY(consume(Token::Type::RightParenthesis));
		auto body = TRY(parse_block_expression());
		span = Span::merge(span, body->span());
		return std::static_pointer_cast<AST::ForExpression const>(std::make_shared<AST::ForWithConditionExpression const>(std::move(condition), std::move(body), span));
	}

	auto body = TRY(parse_block_expression());
	span = Span::merge(span, body->span());
	return std::static_pointer_cast<AST::ForExpression const>(std::make_shared<AST::InfiniteForExpression const>(std::move(body), span));
}

Result<std::shared_ptr<AST::Type const>, Error> Parser::parse_type() {
	bool is_mutable = false;
	if (m_current_token.type() == Token::Type::KW_mut) {
		is_mutable = true;
		TRY(consume());
	}

	// FIXME: In the future we will have to check if the type is defined, or probably delegate that job to the C++ compiler
	switch (m_current_token.type()) {
	case Token::Type::KW_bool:
	case Token::Type::KW_char:
	case Token::Type::KW_i16:
	case Token::Type::KW_i32:
	case Token::Type::KW_i64:
	case Token::Type::KW_i8:
	case Token::Type::KW_isize:
	case Token::Type::KW_u16:
	case Token::Type::KW_u32:
	case Token::Type::KW_u64:
	case Token::Type::KW_u8:
	case Token::Type::KW_usize:
	case Token::Type::Identifier:
		{
			auto type = std::make_shared<AST::Type const>(m_current_token.value(), is_mutable, m_current_token.span());
			TRY(consume());
			return type;
		}
	default:
		return Error { fmt::format("Expected type, got {:?}!", m_current_token.value()), m_current_token.span() };
	}
}

Result<std::shared_ptr<AST::Identifier const>, Error> Parser::parse_identifier() {
	// FIXME: Switch to something better
	if (m_current_token.type() != Token::Type::Identifier) {
		return Error { fmt::format("Expected identifier, got {:?}!", m_current_token.value()), m_current_token.span() };
	}

	auto identifier_value = m_current_token.value();
	auto identifier_span = m_current_token.span();
	TRY(consume());
	return std::make_shared<AST::Identifier const>(identifier_value, identifier_span);
}

Result<std::shared_ptr<AST::IntegerLiteral const>, Error> Parser::parse_integer_literal() {
	if (m_current_token.type() != Token::Type::DecimalLiteral
	    && m_current_token.type() != Token::Type::BinaryLiteral
	    && m_current_token.type() != Token::Type::OctalLiteral
	    && m_current_token.type() != Token::Type::HexadecimalLiteral) {
		return Error { fmt::format("Expected integer literal, got {:?}!", m_current_token.value()), m_current_token.span() };
	}

	auto literal_value = m_current_token.value();
	auto literal_span = m_current_token.span();
	TRY(consume());
	return std::make_shared<AST::IntegerLiteral const>(literal_value, literal_span);
}

Result<std::vector<AST::FunctionParameter>, Error> Parser::parse_function_parameters() {
	std::vector<AST::FunctionParameter> parameters;

	TRY(consume(Token::Type::LeftParenthesis));
	while (true) {
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

Result<std::shared_ptr<AST::FunctionDeclarationStatement const>, Error> Parser::parse_function_declaration() {
	auto span = m_current_token.span();

	TRY(consume(Token::Type::KW_fn));
	auto function_name = TRY(parse_identifier());
	auto function_parameters = TRY(parse_function_parameters());
	TRY(consume(Token::Type::Colon));
	auto function_return_type = TRY(parse_type());
	auto function_body = TRY(parse_block_expression());

	span = Span::merge(span, m_current_token.span());

	return std::make_shared<AST::FunctionDeclarationStatement const>(function_name, function_parameters, function_return_type, function_body, span);
}

Result<std::shared_ptr<AST::VariableDeclarationStatement const>, Error> Parser::parse_variable_declaration() {
	auto span = m_current_token.span();
	TRY(consume(Token::Type::KW_var));
	auto identifier = TRY(parse_identifier());
	std::shared_ptr<AST::Type const> type = nullptr;
	std::shared_ptr<AST::Expression const> expression = nullptr;

	if (m_current_token.type() == Token::Type::Equals) {
		TRY(consume());
		expression = TRY(parse_expression());
	} else if (m_current_token.type() == Token::Type::Colon) {
		TRY(consume());
		type = TRY(parse_type());

		if (m_current_token.type() == Token::Type::Equals) {
			TRY(consume());
			expression = TRY(parse_expression());
		}
	} else {
		return Error { fmt::format("Expected ':' or '=', got {:?}!", m_current_token.value()), m_current_token.span() };
	}

	TRY(consume(Token::Type::Semicolon));
	span = Span::merge(span, m_current_token.span());
	return std::make_shared<AST::VariableDeclarationStatement const>(std::move(identifier), std::move(type), std::move(expression), span);
}

}
