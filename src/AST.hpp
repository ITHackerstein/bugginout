#pragma once

#include "Span.hpp"

#include <memory>
#include <variant>
#include <vector>

namespace bo {

namespace AST {

class Node {
public:
	virtual ~Node() = default;

	virtual void dump(unsigned indent) const = 0;

	Span span() const { return m_span; }

protected:
	explicit Node(Span span)
	  : m_span(span) {}

private:
	Span m_span;
};

class Expression : public Node {
public:
	explicit Expression(Span span)
	  : Node(span) {}

private:
};

class Statement : public Node {
public:
	explicit Statement(Span span)
	  : Node(span) {}

private:
};

class ExpressionStatement : public Statement {
public:
	explicit ExpressionStatement(std::shared_ptr<Expression const> expression, Span span)
	  : Statement(span), m_expression(std::move(expression)) {}

	virtual void dump(unsigned indent) const override;

private:
	std::shared_ptr<Expression const> m_expression;
};

class IntegerLiteral : public Expression {
public:
	explicit IntegerLiteral(std::string_view value, Span span)
	  : Expression(span), m_value(value) {}

	virtual void dump(unsigned indent) const override;

private:
	std::string_view m_value;
};

class Identifier : public Expression {
public:
	explicit Identifier(std::string_view id, Span span)
	  : Expression(span), m_id(id) {}

	virtual void dump(unsigned indent) const override;

private:
	std::string_view m_id;
};

#define _BO_ENUMERATE_BINARY_OPERATORS         \
	BO_ENUMERATE_BINARY_OPERATOR(Addition)       \
	BO_ENUMERATE_BINARY_OPERATOR(Subtraction)    \
	BO_ENUMERATE_BINARY_OPERATOR(Multiplication) \
	BO_ENUMERATE_BINARY_OPERATOR(Division)       \
	BO_ENUMERATE_BINARY_OPERATOR(Modulo)

enum class BinaryOperator {
#define BO_ENUMERATE_BINARY_OPERATOR(x) x,
	_BO_ENUMERATE_BINARY_OPERATORS
#undef BO_ENUMERATE_BINARY_OPERATOR
};

class BinaryExpression : public Expression {
public:
	explicit BinaryExpression(std::shared_ptr<Expression const> lhs, std::shared_ptr<Expression const> rhs, BinaryOperator op, Span span)
	  : Expression(span), m_lhs(std::move(lhs)), m_rhs(std::move(rhs)), m_op(op) {}

	virtual void dump(unsigned indent) const override;

private:
	std::shared_ptr<Expression const> m_lhs;
	std::shared_ptr<Expression const> m_rhs;
	BinaryOperator m_op;
};

class VariableDeclarationStatement : public Statement {
public:
	explicit VariableDeclarationStatement(std::shared_ptr<Identifier const> identifier, std::shared_ptr<Expression const> expression, Span span)
	  : Statement(span), m_identifier(std::move(identifier)), m_expression(std::move(expression)) {}

	virtual void dump(unsigned indent) const override;

private:
	std::shared_ptr<Identifier const> m_identifier;
	std::shared_ptr<Expression const> m_expression;
};

class BlockExpression : public Expression {
public:
	explicit BlockExpression(std::vector<std::shared_ptr<Statement const>> statements, std::shared_ptr<Expression const> expression, Span span)
	  : Expression(span), m_statements(std::move(statements)), m_last_expression_or_statement(std::move(expression)) {}

	explicit BlockExpression(std::vector<std::shared_ptr<Statement const>> statements, std::shared_ptr<Statement const> statement, Span span)
	  : Expression(span), m_statements(std::move(statements)), m_last_expression_or_statement(std::move(statement)) {}

	virtual void dump(unsigned indent) const override;

private:
	std::vector<std::shared_ptr<Statement const>> m_statements;
	std::variant<std::shared_ptr<Expression const>, std::shared_ptr<Statement const>> m_last_expression_or_statement;
};

struct FunctionParameter {
	std::shared_ptr<Identifier const> name;
	std::shared_ptr<Identifier const> type;
	bool is_anonymous;
};

class FunctionDeclarationStatement : public Statement {
public:
	explicit FunctionDeclarationStatement(std::shared_ptr<Identifier const> name, std::vector<FunctionParameter> parameters, std::shared_ptr<Identifier const> return_type, std::shared_ptr<BlockExpression const> body, Span span)
	  : Statement(span), m_name(std::move(name)), m_parameters(std::move(parameters)), m_return_type(return_type), m_body(std::move(body)) {}

	virtual void dump(unsigned indent) const override;

private:
	std::shared_ptr<Identifier const> m_name;
	std::vector<FunctionParameter> m_parameters;
	std::shared_ptr<Identifier const> m_return_type;
	std::shared_ptr<BlockExpression const> m_body;
};

class IfExpression : public Expression {
public:
	explicit IfExpression(std::shared_ptr<Expression const> condition, std::shared_ptr<BlockExpression const> then_block, std::optional<std::shared_ptr<BlockExpression const>> else_block, Span span)
	  : Expression(span), m_condition(std::move(condition)), m_then_block(std::move(then_block)), m_else_block(std::move(else_block)) {}

	virtual void dump(unsigned indent) const override;

private:
	std::shared_ptr<Expression const> m_condition;
	std::shared_ptr<BlockExpression const> m_then_block;
	std::optional<std::shared_ptr<BlockExpression const>> m_else_block;
};

class ForExpression : public Expression {
protected:
	explicit ForExpression(std::shared_ptr<BlockExpression const> body, Span span)
	  : Expression(span), m_body(std::move(body)) {}

	std::shared_ptr<BlockExpression const> m_body;
};

class InfiniteForExpression : public ForExpression {
public:
	explicit InfiniteForExpression(std::shared_ptr<BlockExpression const> body, Span span)
	  : ForExpression(body, span) {}

	virtual void dump(unsigned indent) const override;
};

class ForWithConditionExpression : public ForExpression {
public:
	explicit ForWithConditionExpression(std::shared_ptr<Expression const> condition, std::shared_ptr<BlockExpression const> body, Span span)
	  : ForExpression(body, span), m_condition(condition) {}

	virtual void dump(unsigned indent) const override;

private:
	std::shared_ptr<Expression const> m_condition;
};

class ForWithRangeExpression : public ForExpression {
public:
	explicit ForWithRangeExpression(std::shared_ptr<Identifier const> range_variable, std::shared_ptr<Expression const> range_expression, std::shared_ptr<BlockExpression const> body, Span span)
	  : ForExpression(body, span), m_range_variable(std::move(range_variable)), m_range_expression(std::move(range_expression)) {}

	virtual void dump(unsigned indent) const override;

private:
	std::shared_ptr<Identifier const> m_range_variable;
	std::shared_ptr<Expression const> m_range_expression;
};

struct FunctionArgument {
	std::optional<std::shared_ptr<Identifier const>> name;
	std::shared_ptr<Expression const> value;
};

class FunctionCallExpression : public Expression {
public:
	explicit FunctionCallExpression(std::shared_ptr<Identifier const> name, std::vector<FunctionArgument> arguments, Span span)
	  : Expression(span), m_name(std::move(name)), m_arguments(std::move(arguments)) {}

	virtual void dump(unsigned indent) const override;

private:
	std::shared_ptr<Identifier const> m_name;
	std::vector<FunctionArgument> m_arguments;
};

class Program : public Node {
public:
	explicit Program(std::vector<std::shared_ptr<FunctionDeclarationStatement const>> functions, Span span)
	  : Node(span), m_functions(std::move(functions)) {}

	virtual void dump(unsigned indent) const override;

private:
	// FIXME: Change to a specific node which will contain all the top level statements
	std::vector<std::shared_ptr<FunctionDeclarationStatement const>> m_functions;
};

}

}
