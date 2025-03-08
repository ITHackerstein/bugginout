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

	bool match_secondary_expression() const;

	Result<std::shared_ptr<AST::Expression const>, Error> parse_primary_expression();
	std::shared_ptr<AST::Expression const> parse_secondary_expression(std::shared_ptr<AST::Expression const> lhs, std::shared_ptr<AST::Expression const> rhs, Token::Type left);
	Result<std::shared_ptr<AST::Expression const>, Error> parse_expression_inner(unsigned minimum_precedence);
	Result<std::shared_ptr<AST::Expression const>, Error> parse_expression();
	Result<std::shared_ptr<AST::Statement const>, Error> parse_statement();
	Result<std::shared_ptr<AST::BlockExpression const>, Error> parse_block_expression();
	Result<std::shared_ptr<AST::Type const>, Error> parse_type();
	Result<std::shared_ptr<AST::Identifier const>, Error> parse_identifier();
	Result<std::shared_ptr<AST::IntegerLiteral const>, Error> parse_integer_literal();
	Result<std::vector<AST::FunctionParameter>, Error> parse_function_parameters();
	Result<std::shared_ptr<AST::FunctionDeclarationStatement const>, Error> parse_function_declaration();
	Result<std::shared_ptr<AST::VariableDeclarationStatement const>, Error> parse_variable_declaration();

	Result<void, Error> consume(std::optional<Token::Type> = {});

	Lexer m_lexer;
	Token m_current_token;
};

}
