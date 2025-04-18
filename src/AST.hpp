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
	virtual bool is_expression() const { return false; }
	virtual bool is_statement() const { return false; }

	Span span() const { return m_span; }

protected:
	explicit Node(Span span)
	  : m_span(span) {}

	Span m_span;
};

class Expression : public Node {
public:
	virtual bool is_expression() const override { return true; }
	virtual bool is_parenthesized_expression() const { return false; }
	virtual bool is_integer_literal() const { return false; }
	virtual bool is_char_literal() const { return false; }
	virtual bool is_boolean_literal() const { return false; }
	virtual bool is_identifier() const { return false; }
	virtual bool is_binary_expression() const { return false; }
	virtual bool is_unary_expression() const { return false; }
	virtual bool is_assignment_expression() const { return false; }
	virtual bool is_update_expression() const { return false; }
	virtual bool is_pointer_dereference_expression() const { return false; }
	virtual bool is_address_of_expression() const { return false; }
	virtual bool is_range_expression() const { return false; }
	virtual bool is_block_expression() const { return false; }
	virtual bool is_if_expression() const { return false; }
	virtual bool is_function_call_expression() const { return false; }
	virtual bool is_array_expression() const { return false; }
	virtual bool is_array_subscription_expression() const { return false; }
	virtual bool has_block() const { return false; }

protected:
	explicit Expression(Span span)
	  : Node(span) {}
};

class Statement : public Node {
public:
	virtual bool is_statement() const override { return true; }
	virtual bool is_expression_statement() const { return false; }
	virtual bool is_variable_declaration() const { return false; }
	virtual bool is_for_statement() const { return false; }
	virtual bool is_return_statement() const { return false; }

protected:
	explicit Statement(Span span)
	  : Node(span) {}
};

#define _BO_ENUMERATE_TYPE_FLAGS             \
	BO_ENUMERATE_TYPE_FLAG(IsMutable, 0)       \
	BO_ENUMERATE_TYPE_FLAG(IsWeakPointer, 1)   \
	BO_ENUMERATE_TYPE_FLAG(IsStrongPointer, 2) \
	BO_ENUMERATE_TYPE_FLAG(IsArray, 3)         \
	BO_ENUMERATE_TYPE_FLAG(IsSlice, 4)

enum TypeFlags : int {
#define BO_ENUMERATE_TYPE_FLAG(x, y) PF_##x = 1 << (y),
	_BO_ENUMERATE_TYPE_FLAGS
#undef BO_ENUMERATE_TYPE_FLAG
};

class Identifier;
class IntegerLiteral;
class Type : public Node {
public:
	explicit Type(std::shared_ptr<Type const> inner_type, std::shared_ptr<IntegerLiteral const> array_size, std::shared_ptr<Identifier const> name, int flags, Span span)
	  : Node(span), m_inner_type(std::move(inner_type)), m_array_size(std::move(array_size)), m_name(std::move(name)), m_flags(flags) {}

	virtual void dump() const override;

	int flags() const { return m_flags; }

	bool is_mutable() const { return m_flags & PF_IsMutable; }
	bool is_weak_pointer() const { return m_flags & PF_IsWeakPointer; }
	bool is_strong_pointer() const { return m_flags & PF_IsStrongPointer; }
	bool is_pointer() const { return is_weak_pointer() || is_strong_pointer(); }
	bool is_array() const { return m_flags & PF_IsArray; }
	bool is_slice() const { return m_flags & PF_IsSlice; }

	std::shared_ptr<Type const> inner_type() const { return m_inner_type; }
	std::shared_ptr<IntegerLiteral const> array_size() const { return m_array_size; }
	std::shared_ptr<Identifier const> name() const { return m_name; }

private:
	std::shared_ptr<Type const> m_inner_type;
	std::shared_ptr<IntegerLiteral const> m_array_size;
	std::shared_ptr<Identifier const> m_name;
	int m_flags;
};

class ParenthesizedExpression : public Expression {
public:
	explicit ParenthesizedExpression(std::shared_ptr<Expression const> expression, Span span)
	  : Expression(span), m_expression(std::move(expression)) {}

	virtual void dump() const override;
	virtual bool is_parenthesized_expression() const override { return true; }

	std::shared_ptr<Expression const> expression() const { return m_expression; }

private:
	std::shared_ptr<Expression const> m_expression;
};

class IntegerLiteral : public Expression {
public:
	enum class Type {
		Decimal,
		Binary,
		Octal,
		Hexadecimal
	};

	explicit IntegerLiteral(std::string_view value, Type type, std::string_view suffix, Span span)
	  : Expression(span), m_value(value), m_type(type), m_suffix(suffix) {}

	virtual void dump() const override;
	virtual bool is_integer_literal() const override { return true; }

	std::string_view value() const { return m_value; }
	Type type() const { return m_type; }
	std::string_view suffix() const { return m_suffix; }

private:
	std::string_view m_value;
	Type m_type;
	std::string_view m_suffix;
};

class CharLiteral : public Expression {
public:
	explicit CharLiteral(std::string_view value, Span span)
	  : Expression(span), m_value(value) {}

	virtual void dump() const override;
	virtual bool is_char_literal() const override { return true; }

	std::string_view value() const { return m_value; }

private:
	std::string_view m_value;
};

class BooleanLiteral : public Expression {
public:
	explicit BooleanLiteral(bool value, Span span)
	  : Expression(span), m_value(value) {}

	virtual void dump() const override;
	virtual bool is_boolean_literal() const override { return true; }

	bool value() const { return m_value; }

private:
	bool m_value;
};

class Identifier : public Expression {
public:
	explicit Identifier(std::string_view id, Span span)
	  : Expression(span), m_id(id) {}

	virtual void dump() const override;
	virtual bool is_identifier() const override { return true; }

	std::string_view id() const { return m_id; }

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
	virtual bool is_binary_expression() const override { return true; }

	std::shared_ptr<Expression const> lhs() const { return m_lhs; }
	std::shared_ptr<Expression const> rhs() const { return m_rhs; }
	BinaryOperator op() const { return m_op; }

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
	virtual bool is_unary_expression() const override { return true; }

	std::shared_ptr<Expression const> operand() const { return m_operand; }
	UnaryOperator op() const { return m_op; }

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
	virtual bool is_assignment_expression() const override { return true; }

	std::shared_ptr<Expression const> lhs() const { return m_lhs; }
	std::shared_ptr<Expression const> rhs() const { return m_rhs; }
	AssignmentOperator op() const { return m_op; }

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
	virtual bool is_update_expression() const override { return true; }

	std::shared_ptr<Expression const> operand() const { return m_operand; }
	UpdateOperator op() const { return m_op; }
	bool is_prefixed() const { return m_is_prefixed; }

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
	virtual bool is_pointer_dereference_expression() const override { return true; }

	std::shared_ptr<Expression const> operand() const { return m_operand; }

private:
	std::shared_ptr<Expression const> m_operand;
};

class AddressOfExpression : public Expression {
public:
	explicit AddressOfExpression(std::shared_ptr<Expression const> operand, Span span)
	  : Expression(span), m_operand(std::move(operand)) {}

	virtual void dump() const override;
	virtual bool is_address_of_expression() const override { return true; }

	std::shared_ptr<Expression const> operand() const { return m_operand; }

private:
	std::shared_ptr<Expression const> m_operand;
};

class RangeExpression : public Expression {
public:
	explicit RangeExpression(std::shared_ptr<Expression const> start, std::shared_ptr<Expression const> end, bool is_inclusive, Span span)
	  : Expression(span), m_start(std::move(start)), m_end(std::move(end)), m_is_inclusive(is_inclusive) {}

	virtual void dump() const override;
	virtual bool is_range_expression() const override { return true; }

	std::shared_ptr<Expression const> start() const { return m_start; }
	std::shared_ptr<Expression const> end() const { return m_end; }
	bool is_inclusive() const { return m_is_inclusive; }

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
	virtual bool is_block_expression() const override { return true; }
	virtual bool has_block() const override { return true; }

	std::vector<std::shared_ptr<Statement const>> const& statements() const { return m_statements; }

private:
	std::vector<std::shared_ptr<Statement const>> m_statements;
};

class IfExpression : public Expression {
public:
	explicit IfExpression(std::shared_ptr<Expression const> condition, std::shared_ptr<BlockExpression const> then, std::shared_ptr<Expression const> else_, Span span)
	  : Expression(span), m_condition(std::move(condition)), m_then(std::move(then)), m_else(std::move(else_)) {}

	virtual void dump() const override;
	virtual bool is_if_expression() const override { return true; }
	virtual bool has_block() const override { return true; }

	std::shared_ptr<Expression const> condition() const { return m_condition; }
	std::shared_ptr<BlockExpression const> then() const { return m_then; }
	std::shared_ptr<Expression const> else_() const { return m_else; }

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
	virtual bool is_function_call_expression() const override { return true; }

	std::shared_ptr<Identifier const> name() const { return m_name; }
	std::vector<FunctionArgument> const& arguments() const { return m_arguments; }

private:
	std::shared_ptr<Identifier const> m_name;
	std::vector<FunctionArgument> m_arguments;
};

class ArrayExpression : public Expression {
public:
	explicit ArrayExpression(std::vector<std::shared_ptr<Expression const>> elements, Span span)
	  : Expression(span), m_elements(std::move(elements)) {}

	virtual void dump() const override;
	virtual bool is_array_expression() const override { return true; }

	std::vector<std::shared_ptr<Expression const>> const& elements() const { return m_elements; }

private:
	std::vector<std::shared_ptr<Expression const>> m_elements;
};

class ArraySubscriptExpression : public Expression {
public:
	explicit ArraySubscriptExpression(std::shared_ptr<Expression const> array, std::shared_ptr<Expression const> index, Span span)
	  : Expression(span), m_array(std::move(array)), m_index(std::move(index)) {}

	virtual void dump() const override;
	virtual bool is_array_subscription_expression() const override { return true; }

	std::shared_ptr<Expression const> array() const { return m_array; }
	std::shared_ptr<Expression const> index() const { return m_index; }

private:
	std::shared_ptr<Expression const> m_array;
	std::shared_ptr<Expression const> m_index;
};

class ExpressionStatement : public Statement {
public:
	explicit ExpressionStatement(std::shared_ptr<Expression const> expression, bool ends_with_semicolon, Span span)
	  : Statement(span), m_expression(std::move(expression)), m_ends_with_semicolon(ends_with_semicolon) {}

	virtual bool is_expression_statement() const override { return true; }
	virtual void dump() const override;

	std::shared_ptr<Expression const> expression() const { return m_expression; }
	bool ends_with_semicolon() const { return m_ends_with_semicolon; }

private:
	std::shared_ptr<Expression const> m_expression;
	bool m_ends_with_semicolon;
};

class VariableDeclarationStatement : public Statement {
public:
	explicit VariableDeclarationStatement(bool is_mutable, std::shared_ptr<Identifier const> identifier, std::shared_ptr<Type const> type, std::shared_ptr<Expression const> initializer, Span span)
	  : Statement(span), m_is_mutable(is_mutable), m_identifier(std::move(identifier)), m_type(std::move(type)), m_initializer(std::move(initializer)) {}

	virtual void dump() const override;
	virtual bool is_variable_declaration() const override { return true; }

	bool is_mutable() const { return m_is_mutable; }
	std::shared_ptr<Identifier const> identifier() const { return m_identifier; }
	std::shared_ptr<Type const> type() const { return m_type; }
	std::shared_ptr<Expression const> initializer() const { return m_initializer; }

private:
	bool m_is_mutable;
	std::shared_ptr<Identifier const> m_identifier;
	std::shared_ptr<Type const> m_type;
	std::shared_ptr<Expression const> m_initializer;
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

	std::shared_ptr<Identifier const> name() const { return m_name; }
	std::vector<FunctionParameter> const& parameters() const { return m_parameters; }
	std::shared_ptr<Type const> return_type() const { return m_return_type; }
	std::shared_ptr<BlockExpression const> body() const { return m_body; }

private:
	std::shared_ptr<Identifier const> m_name;
	std::vector<FunctionParameter> m_parameters;
	std::shared_ptr<Type const> m_return_type;
	std::shared_ptr<BlockExpression const> m_body;
};

class ForStatement : public Statement {
public:
	virtual bool is_for_statement() const override { return true; }
	virtual bool is_infinite() const { return false; }
	virtual bool is_with_condition() const { return false; }
	virtual bool is_with_range() const { return false; }

	std::shared_ptr<BlockExpression const> body() const { return m_body; }

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
	virtual bool is_infinite() const override { return true; }
};

class ForWithConditionStatement : public ForStatement {
public:
	explicit ForWithConditionStatement(std::shared_ptr<Expression const> condition, std::shared_ptr<BlockExpression const> body, Span span)
	  : ForStatement(body, span), m_condition(condition) {}

	virtual void dump() const override;
	virtual bool is_with_condition() const override { return true; }

	std::shared_ptr<Expression const> condition() const { return m_condition; }

private:
	std::shared_ptr<Expression const> m_condition;
};

class ForWithRangeStatement : public ForStatement {
public:
	explicit ForWithRangeStatement(std::shared_ptr<Identifier const> range_variable, std::shared_ptr<Expression const> range_expression, std::shared_ptr<BlockExpression const> body, Span span)
	  : ForStatement(body, span), m_range_variable(std::move(range_variable)), m_range_expression(std::move(range_expression)) {}

	virtual void dump() const override;
	virtual bool is_with_range() const override { return true; }

	std::shared_ptr<Identifier const> range_variable() const { return m_range_variable; }
	std::shared_ptr<Expression const> range_expression() const { return m_range_expression; }

private:
	std::shared_ptr<Identifier const> m_range_variable;
	std::shared_ptr<Expression const> m_range_expression;
};

class ReturnStatement : public Statement {
public:
	explicit ReturnStatement(std::shared_ptr<Expression const> expression, Span span)
	  : Statement(span), m_expression(std::move(expression)) {}

	virtual void dump() const override;
	virtual bool is_return_statement() const override { return true; }

	std::shared_ptr<Expression const> expression() const { return m_expression; }

private:
	std::shared_ptr<Expression const> m_expression;
};

class Program : public Node {
public:
	explicit Program(std::vector<std::shared_ptr<FunctionDeclarationStatement const>> functions, Span span)
	  : Node(span), m_functions(std::move(functions)) {}

	virtual void dump() const override;

	std::vector<std::shared_ptr<FunctionDeclarationStatement const>> const& function_declarations() const { return m_functions; }

private:
	// FIXME: Change to a specific node which will contain all the top level statements
	std::vector<std::shared_ptr<FunctionDeclarationStatement const>> m_functions;
};

}

}
