#pragma once

#include "Span.hpp"

#include <memory>
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
	virtual bool is_identifier() const { return false; }
	virtual bool has_block() const { return false; }

protected:
	explicit Expression(Span span)
	  : Node(span) {}
};

class Statement : public Node {
public:
	virtual bool is_expression_statement() const { return false; }

protected:
	explicit Statement(Span span)
	  : Node(span) {}
};

enum TypeFlags : int {
	PF_IsMutable = 1 << 0,
	PF_IsWeakPointer = 1 << 1,
	PF_IsStrongPointer = 1 << 2,
};

class Identifier;
class Type : public Node {
public:
	explicit Type(std::shared_ptr<Type const> inner_type, int flags, Span span)
	  : Node(span), m_inner_type(std::move(inner_type)), m_name(nullptr), m_flags(flags) {}

	explicit Type(std::shared_ptr<Identifier const> name, int flags, Span span)
	  : Node(span), m_inner_type(nullptr), m_name(name), m_flags(flags) {}

	virtual void dump() const override;

	int flags() const { return m_flags; }

	bool is_mutable() const { return m_flags & PF_IsMutable; }
	bool is_weak_pointer() const { return m_flags & PF_IsWeakPointer; }
	bool is_strong_pointer() const { return m_flags & PF_IsStrongPointer; }
	bool is_pointer() const { return is_weak_pointer() || is_strong_pointer(); }

private:
	std::shared_ptr<Type const> m_inner_type;
	std::shared_ptr<Identifier const> m_name;
	int m_flags;
};

class ParenthesizedExpression : public Expression {
public:
	explicit ParenthesizedExpression(std::shared_ptr<Expression const> expression, Span span)
	  : Expression(span), m_expression(std::move(expression)) {}

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

	virtual bool is_identifier() const override { return true; }

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

class RangeExpression : public Expression {
public:
	explicit RangeExpression(std::shared_ptr<Expression const> start, std::shared_ptr<Expression const> end, bool is_inclusive, Span span)
	  : Expression(span), m_start(std::move(start)), m_end(std::move(end)), m_is_inclusive(is_inclusive) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_start;
	std::shared_ptr<Expression const> m_end;
	bool m_is_inclusive;
};

class BlockExpression : public Expression {
public:
	explicit BlockExpression(std::vector<std::shared_ptr<Statement const>> statements, Span span)
	  : Expression(span), m_statements(std::move(statements)) {}

	virtual void dump() const override;
	virtual bool has_block() const override { return true; }

private:
	std::vector<std::shared_ptr<Statement const>> m_statements;
};

class IfExpression : public Expression {
public:
	explicit IfExpression(std::shared_ptr<Expression const> condition, std::shared_ptr<BlockExpression const> then, std::shared_ptr<Expression const> else_, Span span)
	  : Expression(span), m_condition(std::move(condition)), m_then(std::move(then)), m_else(std::move(else_)) {}

	virtual void dump() const override;
	virtual bool has_block() const override { return true; }

private:
	std::shared_ptr<Expression const> m_condition;
	std::shared_ptr<BlockExpression const> m_then;
	std::shared_ptr<Expression const> m_else;
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

class ExpressionStatement : public Statement {
public:
	explicit ExpressionStatement(std::shared_ptr<Expression const> expression, bool ends_with_semicolon, Span span)
	  : Statement(span), m_expression(std::move(expression)), m_ends_with_semicolon(ends_with_semicolon) {}

	virtual bool is_expression_statement() const override { return true; }
	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_expression;
	bool m_ends_with_semicolon;
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

class ForStatement : public Statement {
protected:
	explicit ForStatement(std::shared_ptr<BlockExpression const> body, Span span)
	  : Statement(span), m_body(std::move(body)) {}

	std::shared_ptr<BlockExpression const> m_body;
};

class InfiniteForStatement : public ForStatement {
public:
	explicit InfiniteForStatement(std::shared_ptr<BlockExpression const> body, Span span)
	  : ForStatement(body, span) {}

	virtual void dump() const override;
};

class ForWithConditionStatement : public ForStatement {
public:
	explicit ForWithConditionStatement(std::shared_ptr<Expression const> condition, std::shared_ptr<BlockExpression const> body, Span span)
	  : ForStatement(body, span), m_condition(condition) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_condition;
};

class ForWithRangeStatement : public ForStatement {
public:
	explicit ForWithRangeStatement(std::shared_ptr<Identifier const> range_variable, std::shared_ptr<Expression const> range_expression, std::shared_ptr<BlockExpression const> body, Span span)
	  : ForStatement(body, span), m_range_variable(std::move(range_variable)), m_range_expression(std::move(range_expression)) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Identifier const> m_range_variable;
	std::shared_ptr<Expression const> m_range_expression;
};

class ReturnStatement : public Statement {
public:
	explicit ReturnStatement(std::shared_ptr<Expression const> expression, Span span)
	  : Statement(span), m_expression(std::move(expression)) {}

	virtual void dump() const override;

private:
	std::shared_ptr<Expression const> m_expression;
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
