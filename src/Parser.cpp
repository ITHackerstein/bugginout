#include "Parser.hpp"

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
	return type == Token::Type::Plus
	  || type == Token::Type::Minus
	  || type == Token::Type::Asterisk
	  || type == Token::Type::Solidus
	  || type == Token::Type::Percent;
}

// FIXME: Switch this to a table for better readability
static unsigned operator_precedence_of(Token::Type type) {
	switch (type) {
	case Token::Type::Plus:
	case Token::Type::Minus:
		return 1;
	case Token::Type::Asterisk:
	case Token::Type::Solidus:
	case Token::Type::Percent:
		return 2;
	default:
		return 0;
	}
}

static AST::BinaryOperator operator_from_token(Token::Type type) {
	switch (type) {
	case Token::Type::Plus:
		return AST::BinaryOperator::Addition;
	case Token::Type::Minus:
		return AST::BinaryOperator::Subtraction;
	case Token::Type::Asterisk:
		return AST::BinaryOperator::Multiplication;
	case Token::Type::Solidus:
		return AST::BinaryOperator::Division;
	case Token::Type::Percent:
		return AST::BinaryOperator::Modulo;
	default:
		assert(false && "Should not be here!");
	}
}

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_primary_expression() {
	// FIXME: Handle unary opeartors
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
			TRY(consume());
			auto expression = TRY(parse_expression());
			TRY(consume(Token::Type::RightParenthesis));
			return expression;
		}
	default:
		assert(false);
	}
}

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_expression_inner(unsigned minimum_precedence) {
	auto result = TRY(parse_primary_expression());

	while (match_secondary_expression()) {
		auto operator_precedence = operator_precedence_of(m_current_token.type());
		if (operator_precedence < minimum_precedence) {
			break;
		}

		auto operator_ = operator_from_token(m_current_token.type());
		TRY(consume());
		auto rhs = TRY(parse_expression_inner(operator_precedence + 1));
		result = std::make_shared<AST::BinaryExpression const>(std::move(result), std::move(rhs), operator_, Span::merge(result->span(), rhs->span()));
	}

	return result;
}

Result<std::shared_ptr<AST::BlockExpression const>, Error> Parser::parse_block_expression() {
	TRY(consume(Token::Type::LeftCurlyBracket));
	std::vector<std::shared_ptr<AST::Statement const>> statements;
	std::shared_ptr<AST::Expression const> last_expression;
	while (m_current_token.type() != Token::Type::RightCurlyBracket) {
		last_expression = TRY(parse_expression());
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

Result<std::shared_ptr<AST::Expression const>, Error> Parser::parse_expression() {
	switch (m_current_token.type()) {
	case Token::Type::LeftCurlyBracket:
		return std::static_pointer_cast<AST::Expression const>(TRY(parse_block_expression()));
	default:
		return parse_expression_inner(0);
	}
}

Result<std::shared_ptr<AST::Type const>, Error> Parser::parse_type() {
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
			auto type = std::make_shared<AST::Type const>(m_current_token.value(), m_current_token.span());
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

}
