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

	virtual void dump() const = 0;

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

class Type : public Node {
public:
	explicit Type(std::string_view type, bool is_mutable, Span span)
	  : Node(span), m_type(type), m_is_mutable(is_mutable) {}

	virtual void dump() const override;

private:
	std::string_view m_type;
	bool m_is_mutable;
};

class ExpressionStatement : public Statement {
public:
	explicit ExpressionStatement(std::shared_ptr<Expression const> expression, Span span)
	  : Statement(span), m_expression(std::move(expression)) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_expression;
};

class IntegerLiteral : public Expression {
public:
	explicit IntegerLiteral(std::string_view value, Span span)
	  : Expression(span), m_value(value) {}

	virtual void dump() const override;

private:
	std::string_view m_value;
};

class Identifier : public Expression {
public:
	explicit Identifier(std::string_view id, Span span)
	  : Expression(span), m_id(id) {}

	virtual void dump() const override;

private:
	std::string_view m_id;
};

#define _BO_ENUMERATE_BINARY_OPERATORS               \
	BO_ENUMERATE_BINARY_OPERATOR(Addition)             \
	BO_ENUMERATE_BINARY_OPERATOR(Subtraction)          \
	BO_ENUMERATE_BINARY_OPERATOR(Multiplication)       \
	BO_ENUMERATE_BINARY_OPERATOR(Division)             \
	BO_ENUMERATE_BINARY_OPERATOR(Modulo)               \
	BO_ENUMERATE_BINARY_OPERATOR(BitwiseLeftShift)     \
	BO_ENUMERATE_BINARY_OPERATOR(BitwiseRightShift)    \
	BO_ENUMERATE_BINARY_OPERATOR(LessThan)             \
	BO_ENUMERATE_BINARY_OPERATOR(GreaterThan)          \
	BO_ENUMERATE_BINARY_OPERATOR(LessThanOrEqualTo)    \
	BO_ENUMERATE_BINARY_OPERATOR(GreaterThanOrEqualTo) \
	BO_ENUMERATE_BINARY_OPERATOR(EqualTo)              \
	BO_ENUMERATE_BINARY_OPERATOR(NotEqualTo)           \
	BO_ENUMERATE_BINARY_OPERATOR(BitwiseAnd)           \
	BO_ENUMERATE_BINARY_OPERATOR(BitwiseXor)           \
	BO_ENUMERATE_BINARY_OPERATOR(BitwiseOr)            \
	BO_ENUMERATE_BINARY_OPERATOR(LogicalAnd)           \
	BO_ENUMERATE_BINARY_OPERATOR(LogicalOr)

enum class BinaryOperator {
#define BO_ENUMERATE_BINARY_OPERATOR(x) x,
	_BO_ENUMERATE_BINARY_OPERATORS
#undef BO_ENUMERATE_BINARY_OPERATOR
};

class BinaryExpression : public Expression {
public:
	explicit BinaryExpression(std::shared_ptr<Expression const> lhs, std::shared_ptr<Expression const> rhs, BinaryOperator op, Span span)
	  : Expression(span), m_lhs(std::move(lhs)), m_rhs(std::move(rhs)), m_op(op) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_lhs;
	std::shared_ptr<Expression const> m_rhs;
	BinaryOperator m_op;
};

#define _BO_ENUMERATE_UNARY_OPERATORS     \
	BO_ENUMERATE_UNARY_OPERATOR(Positive)   \
	BO_ENUMERATE_UNARY_OPERATOR(Negative)   \
	BO_ENUMERATE_UNARY_OPERATOR(LogicalNot) \
	BO_ENUMERATE_UNARY_OPERATOR(BitwiseNot)

enum class UnaryOperator {
#define BO_ENUMERATE_UNARY_OPERATOR(x) x,
	_BO_ENUMERATE_UNARY_OPERATORS
#undef BO_ENUMERATE_UNARY_OPERATOR
};

class UnaryExpression : public Expression {
public:
	explicit UnaryExpression(std::shared_ptr<Expression const> operand, UnaryOperator op, Span span)
	  : Expression(span), m_operand(std::move(operand)), m_op(op) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_operand;
	UnaryOperator m_op;
};

#define _BO_ENUMERATE_ASSIGNMENT_OPERATORS                      \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(Assignment)                  \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(AdditionAssignment)          \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(SubtractionAssignment)       \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(MultiplicationAssignment)    \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(DivisionAssignment)          \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(ModuloAssignment)            \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(BitwiseLeftShiftAssignment)  \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(BitwiseRightShiftAssignment) \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(BitwiseAndAssignment)        \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(BitwiseXorAssignment)        \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(BitwiseOrAssignment)         \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(LogicalAndAssignment)        \
	BO_ENUMERATE_ASSIGNMENT_OPERATOR(LogicalOrAssignment)

enum class AssignmentOperator {
#define BO_ENUMERATE_ASSIGNMENT_OPERATOR(x) x,
	_BO_ENUMERATE_ASSIGNMENT_OPERATORS
#undef BO_ENUMERATE_ASSIGNMENT_OPERATOR
};

class AssignmentExpression : public Expression {
public:
	explicit AssignmentExpression(std::shared_ptr<Expression const> lhs, std::shared_ptr<Expression const> rhs, AssignmentOperator op, Span span)
	  : Expression(span), m_lhs(std::move(lhs)), m_rhs(std::move(rhs)), m_op(op) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_lhs;
	std::shared_ptr<Expression const> m_rhs;
	AssignmentOperator m_op;
};

#define _BO_ENUMERATE_UPDATE_OPERATORS    \
	BO_ENUMERATE_UPDATE_OPERATOR(Increment) \
	BO_ENUMERATE_UPDATE_OPERATOR(Decrement)

enum class UpdateOperator {
#define BO_ENUMERATE_UPDATE_OPERATOR(x) x,
	_BO_ENUMERATE_UPDATE_OPERATORS
#undef BO_ENUMERATE_UPDATE_OPERATOR
};

class UpdateExpression : public Expression {
public:
	explicit UpdateExpression(std::shared_ptr<Expression const> operand, UpdateOperator op, bool is_prefixed, Span span)
	  : Expression(span), m_operand(std::move(operand)), m_op(op), m_is_prefixed(is_prefixed) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_operand;
	UpdateOperator m_op;
	bool m_is_prefixed;
};

class PointerDereferenceExpression : public Expression {
public:
	explicit PointerDereferenceExpression(std::shared_ptr<Expression const> operand, Span span)
	  : Expression(span), m_operand(std::move(operand)) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_operand;
};

class AddressOfExpression : public Expression {
public:
	explicit AddressOfExpression(std::shared_ptr<Expression const> operand, Span span)
	  : Expression(span), m_operand(std::move(operand)) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_operand;
};

class VariableDeclarationStatement : public Statement {
public:
	explicit VariableDeclarationStatement(std::shared_ptr<Identifier const> identifier, std::shared_ptr<Type const> type, std::shared_ptr<Expression const> expression, Span span)
	  : Statement(span), m_identifier(std::move(identifier)), m_type(type), m_expression(std::move(expression)) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Identifier const> m_identifier;
	std::shared_ptr<Type const> m_type;
	std::shared_ptr<Expression const> m_expression;
};

class BlockExpression : public Expression {
public:
	explicit BlockExpression(std::vector<std::shared_ptr<Statement const>> statements, std::shared_ptr<Expression const> expression, Span span)
	  : Expression(span), m_statements(std::move(statements)), m_last_expression_or_statement(std::move(expression)) {}

	explicit BlockExpression(std::vector<std::shared_ptr<Statement const>> statements, std::shared_ptr<Statement const> statement, Span span)
	  : Expression(span), m_statements(std::move(statements)), m_last_expression_or_statement(std::move(statement)) {}

	virtual void dump() const override;

private:
	std::vector<std::shared_ptr<Statement const>> m_statements;
	std::variant<std::shared_ptr<Expression const>, std::shared_ptr<Statement const>> m_last_expression_or_statement;
};

struct FunctionParameter {
	std::shared_ptr<Identifier const> name;
	std::shared_ptr<Type const> type;
	bool is_anonymous;
};

class FunctionDeclarationStatement : public Statement {
public:
	explicit FunctionDeclarationStatement(std::shared_ptr<Identifier const> name, std::vector<FunctionParameter> parameters, std::shared_ptr<Type const> return_type, std::shared_ptr<BlockExpression const> body, Span span)
	  : Statement(span), m_name(std::move(name)), m_parameters(std::move(parameters)), m_return_type(return_type), m_body(std::move(body)) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Identifier const> m_name;
	std::vector<FunctionParameter> m_parameters;
	std::shared_ptr<Type const> m_return_type;
	std::shared_ptr<BlockExpression const> m_body;
};

class IfExpression : public Expression {
public:
	explicit IfExpression(std::shared_ptr<Expression const> condition, std::shared_ptr<BlockExpression const> then, std::shared_ptr<Expression const> else_, Span span)
	  : Expression(span), m_condition(std::move(condition)), m_then(std::move(then)), m_else(std::move(else_)) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_condition;
	std::shared_ptr<BlockExpression const> m_then;
	std::shared_ptr<Expression const> m_else;
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

	virtual void dump() const override;
};

class ForWithConditionExpression : public ForExpression {
public:
	explicit ForWithConditionExpression(std::shared_ptr<Expression const> condition, std::shared_ptr<BlockExpression const> body, Span span)
	  : ForExpression(body, span), m_condition(condition) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_condition;
};

class ForWithRangeExpression : public ForExpression {
public:
	explicit ForWithRangeExpression(std::shared_ptr<Identifier const> range_variable, std::shared_ptr<Expression const> range_expression, std::shared_ptr<BlockExpression const> body, Span span)
	  : ForExpression(body, span), m_range_variable(std::move(range_variable)), m_range_expression(std::move(range_expression)) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Identifier const> m_range_variable;
	std::shared_ptr<Expression const> m_range_expression;
};

struct FunctionArgument {
	std::shared_ptr<Identifier const> name;
	std::shared_ptr<Expression const> value;
};

class FunctionCallExpression : public Expression {
public:
	explicit FunctionCallExpression(std::shared_ptr<Identifier const> name, std::vector<FunctionArgument> arguments, Span span)
	  : Expression(span), m_name(std::move(name)), m_arguments(std::move(arguments)) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Identifier const> m_name;
	std::vector<FunctionArgument> m_arguments;
};

class Program : public Node {
public:
	explicit Program(std::vector<std::shared_ptr<FunctionDeclarationStatement const>> functions, Span span)
	  : Node(span), m_functions(std::move(functions)) {}

	virtual void dump() const override;

private:
	// FIXME: Change to a specific node which will contain all the top level statements
	std::vector<std::shared_ptr<FunctionDeclarationStatement const>> m_functions;
};

}

}
