#pragma once

#include "AST.hpp"
#include "Lexer.hpp"

namespace bo {

class Parser {
public:
	static Result<Parser, Error> create(std::string_view source);

	Result<std::shared_ptr<AST::Program const>, Error> parse_program();

private:
	explicit Parser(Lexer&& lexer, Token&& current_token)
	  : m_lexer(std::move(lexer)), m_current_token(std::move(current_token)) {}

	template<typename Fn>
	auto restrict(Fn fn, int restrictions);

	bool match_unary_expression() const;
	bool match_secondary_expression() const;

	Result<std::shared_ptr<AST::Type const>, Error> parse_type();
	Result<std::shared_ptr<AST::Identifier const>, Error> parse_identifier(bool allow_keywords = false);
	Result<std::shared_ptr<AST::IntegerLiteral const>, Error> parse_integer_literal();

	Result<std::shared_ptr<AST::Expression const>, Error> parse_unary_expression();
	Result<std::shared_ptr<AST::Expression const>, Error> parse_primary_expression();
	Result<std::shared_ptr<AST::Expression const>, Error> parse_secondary_expression(std::shared_ptr<AST::Expression const> lhs, Token const& operator_token, unsigned minimum_precedence);
	Result<std::shared_ptr<AST::Expression const>, Error> parse_expression_inner(unsigned minimum_precedence);
	Result<std::shared_ptr<AST::Expression const>, Error> parse_expression();
	Result<std::shared_ptr<AST::Expression const>, Error> parse_expression_with_restrictions(int restrictions);
	Result<std::shared_ptr<AST::BlockExpression const>, Error> parse_block_expression();
	Result<std::shared_ptr<AST::IfExpression const>, Error> parse_if_expression();
	Result<std::vector<AST::FunctionArgument>, Error> parse_function_arguments();

	Result<std::shared_ptr<AST::Statement const>, Error> parse_statement();
	Result<std::shared_ptr<AST::VariableDeclarationStatement const>, Error> parse_variable_declaration_statement();
	Result<std::shared_ptr<AST::ForStatement const>, Error> parse_for_statement();
	Result<std::vector<AST::FunctionParameter>, Error> parse_function_parameters();
	Result<std::shared_ptr<AST::FunctionDeclarationStatement const>, Error> parse_function_declaration_statement();
	Result<std::shared_ptr<AST::ReturnStatement const>, Error> parse_return_statement();

	Result<void, Error> consume(std::optional<Token::Type> = {});

	Lexer m_lexer;
	Token m_current_token;

	enum Restrictions : int {
		R_None = 0,
		R_NoExpressionsWithBlocks = 1 << 0,
	};

	int m_restrictions { 0 };
};

}
