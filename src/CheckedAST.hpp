#pragma once

#include "AST.hpp"
#include "Span.hpp"
#include "Types.hpp"

#include <memory>
#include <vector>

namespace bo {

namespace CheckedAST {

class Program;
class Node {
public:
	virtual ~Node() = default;

	virtual void dump(Program const&) const = 0;

	virtual bool is_expression() const { return false; }
	virtual bool is_statement() const { return false; }

	Types::Id type_id() const { return m_type_id; }
	Span span() const { return m_span; }

protected:
	explicit Node(Types::Id type_id, Span span)
	  : m_type_id(type_id), m_span(span) {}

	Types::Id m_type_id;
	Span m_span;
};

class Expression : public Node {
public:
	virtual bool is_expression() const override { return true; }
	virtual bool is_parenthesized_expression() const { return false; }
	virtual bool is_integer_literal() const { return false; }
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
	explicit Expression(Types::Id type_id, Span span)
	  : Node(type_id, span) {}
};

class Statement : public Node {
public:
	virtual bool is_statement() const override { return true; }
	virtual bool is_expression_statement() const { return false; }
	virtual bool is_variable_declaration() const { return false; }
	virtual bool is_for_statement() const { return false; }
	virtual bool is_return_statement() const { return false; }

protected:
	explicit Statement(Types::Id type_id, Span span)
	  : Node(type_id, span) {}
};

class ParenthesizedExpression : public Expression {
public:
	explicit ParenthesizedExpression(std::shared_ptr<Expression const> expression, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_expression(std::move(expression)) {}

	virtual void dump(Program const&) const override;
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

	explicit IntegerLiteral(std::string_view value, std::string_view suffix, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_value(value), m_suffix(suffix) {}

	virtual void dump(Program const&) const override;
	virtual bool is_integer_literal() const override { return true; }

	std::string_view value() const { return m_value; }
	std::string_view suffix() const { return m_suffix; }

private:
	std::string_view m_value;
	std::string_view m_suffix;
};

class Identifier : public Expression {
public:
	explicit Identifier(std::size_t variable_id, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_variable_id(variable_id) {}

	virtual void dump(Program const&) const override;
	virtual bool is_identifier() const override { return true; }

	std::size_t variable_id() const { return m_variable_id; }

private:
	std::size_t m_variable_id;
};

class BinaryExpression : public Expression {
public:
	explicit BinaryExpression(std::shared_ptr<Expression const> lhs, std::shared_ptr<Expression const> rhs, AST::BinaryOperator op, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_lhs(std::move(lhs)), m_rhs(std::move(rhs)), m_op(op) {}

	virtual void dump(Program const&) const override;
	virtual bool is_binary_expression() const override { return true; }

	std::shared_ptr<Expression const> lhs() const { return m_lhs; }
	std::shared_ptr<Expression const> rhs() const { return m_rhs; }
	AST::BinaryOperator op() const { return m_op; }

private:
	std::shared_ptr<Expression const> m_lhs;
	std::shared_ptr<Expression const> m_rhs;
	AST::BinaryOperator m_op;
};

class UnaryExpression : public Expression {
public:
	explicit UnaryExpression(std::shared_ptr<Expression const> operand, AST::UnaryOperator op, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_operand(std::move(operand)), m_op(op) {}

	virtual void dump(Program const&) const override;
	virtual bool is_unary_expression() const override { return true; }

	std::shared_ptr<Expression const> operand() const { return m_operand; }
	AST::UnaryOperator op() const { return m_op; }

private:
	std::shared_ptr<Expression const> m_operand;
	AST::UnaryOperator m_op;
};

class AssignmentExpression : public Expression {
public:
	explicit AssignmentExpression(std::shared_ptr<Expression const> lhs, std::shared_ptr<Expression const> rhs, AST::AssignmentOperator op, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_lhs(std::move(lhs)), m_rhs(std::move(rhs)), m_op(op) {}

	virtual void dump(Program const&) const override;
	virtual bool is_assignment_expression() const override { return true; }

	std::shared_ptr<Expression const> lhs() const { return m_lhs; }
	std::shared_ptr<Expression const> rhs() const { return m_rhs; }
	AST::AssignmentOperator op() const { return m_op; }

private:
	std::shared_ptr<Expression const> m_lhs;
	std::shared_ptr<Expression const> m_rhs;
	AST::AssignmentOperator m_op;
};

class UpdateExpression : public Expression {
public:
	explicit UpdateExpression(std::shared_ptr<Expression const> operand, AST::UpdateOperator op, bool is_prefixed, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_operand(std::move(operand)), m_op(op), m_is_prefixed(is_prefixed) {}

	virtual void dump(Program const&) const override;
	virtual bool is_update_expression() const override { return true; }

	std::shared_ptr<Expression const> operand() const { return m_operand; }
	AST::UpdateOperator op() const { return m_op; }
	bool is_prefixed() const { return m_is_prefixed; }

private:
	std::shared_ptr<Expression const> m_operand;
	AST::UpdateOperator m_op;
	bool m_is_prefixed;
};

class PointerDereferenceExpression : public Expression {
public:
	explicit PointerDereferenceExpression(std::shared_ptr<Expression const> operand, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_operand(std::move(operand)) {}

	virtual void dump(Program const&) const override;
	virtual bool is_pointer_dereference_expression() const override { return true; }

	std::shared_ptr<Expression const> operand() const { return m_operand; }

private:
	std::shared_ptr<Expression const> m_operand;
};

class AddressOfExpression : public Expression {
public:
	explicit AddressOfExpression(std::shared_ptr<Expression const> operand, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_operand(std::move(operand)) {}

	virtual void dump(Program const&) const override;
	virtual bool is_address_of_expression() const override { return true; }

	std::shared_ptr<Expression const> operand() const { return m_operand; }

private:
	std::shared_ptr<Expression const> m_operand;
};

class RangeExpression : public Expression {
public:
	explicit RangeExpression(std::shared_ptr<Expression const> start, std::shared_ptr<Expression const> end, bool is_inclusive, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_start(std::move(start)), m_end(std::move(end)), m_is_inclusive(is_inclusive) {}

	virtual void dump(Program const&) const override;
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
	explicit BlockExpression(std::vector<std::shared_ptr<Statement const>> statements, bool contains_return_statement, std::size_t scope_id, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_statements(std::move(statements)), m_contains_return_statement(contains_return_statement), m_scope_id(scope_id) {}

	virtual void dump(Program const&) const override;
	virtual bool is_block_expression() const override { return true; }
	virtual bool has_block() const override { return true; }

	std::vector<std::shared_ptr<Statement const>> const& statements() const { return m_statements; }
	bool contains_return_statement() const { return m_contains_return_statement; }
	std::size_t scope_id() const { return m_scope_id; }

private:
	std::vector<std::shared_ptr<Statement const>> m_statements;
	bool m_contains_return_statement;
	std::size_t m_scope_id;
};

class IfExpression : public Expression {
public:
	explicit IfExpression(std::shared_ptr<Expression const> condition, std::shared_ptr<BlockExpression const> then, std::shared_ptr<Expression const> else_, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_condition(std::move(condition)), m_then(std::move(then)), m_else(std::move(else_)) {}

	virtual void dump(Program const&) const override;
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
	std::string_view name;
	std::shared_ptr<Expression const> value;
};

class Function;
class FunctionCallExpression : public Expression {
public:
	explicit FunctionCallExpression(std::shared_ptr<Function const> function, std::vector<FunctionArgument> arguments, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_function(function), m_arguments(std::move(arguments)) {}

	virtual void dump(Program const&) const override;
	virtual bool is_function_call_expression() const override { return true; }

	std::shared_ptr<Function const> function() const { return m_function; }
	std::vector<FunctionArgument> const& arguments() const { return m_arguments; }

private:
	std::shared_ptr<Function const> m_function;
	std::vector<FunctionArgument> m_arguments;
};

class ArrayExpression : public Expression {
public:
	explicit ArrayExpression(std::vector<std::shared_ptr<Expression const>> elements, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_elements(std::move(elements)) {}

	virtual void dump(Program const&) const override;
	virtual bool is_array_expression() const override { return true; }

	std::vector<std::shared_ptr<Expression const>> const& elements() const { return m_elements; }

private:
	std::vector<std::shared_ptr<Expression const>> m_elements;
};

class ArraySubscriptExpression : public Expression {
public:
	explicit ArraySubscriptExpression(std::shared_ptr<Expression const> array, std::shared_ptr<Expression const> index, Types::Id type_id, Span span)
	  : Expression(type_id, span), m_array(std::move(array)), m_index(std::move(index)) {}

	virtual void dump(Program const&) const override;
	virtual bool is_array_subscription_expression() const override { return true; }

	std::shared_ptr<Expression const> array() const { return m_array; }
	std::shared_ptr<Expression const> index() const { return m_index; }

private:
	std::shared_ptr<Expression const> m_array;
	std::shared_ptr<Expression const> m_index;
};

class ExpressionStatement : public Statement {
public:
	explicit ExpressionStatement(std::shared_ptr<Expression const> expression, bool ends_with_semicolon, Types::Id type_id, Span span)
	  : Statement(type_id, span), m_expression(std::move(expression)), m_ends_with_semicolon(ends_with_semicolon) {}

	virtual bool is_expression_statement() const override { return true; }
	virtual void dump(Program const&) const override;

	std::shared_ptr<Expression const> expression() const { return m_expression; }
	bool ends_with_semicolon() const { return m_ends_with_semicolon; }

private:
	std::shared_ptr<Expression const> m_expression;
	bool m_ends_with_semicolon;
};

class VariableDeclarationStatement : public Statement {
public:
	explicit VariableDeclarationStatement(std::size_t variable_id, std::shared_ptr<Expression const> initializer, Span span)
	  : Statement(Types::builtin_void_id, span), m_variable_id(variable_id), m_initializer(std::move(initializer)) {}

	virtual void dump(Program const&) const override;
	virtual bool is_variable_declaration() const override { return true; }

	std::size_t variable_id() const { return m_variable_id; }
	std::shared_ptr<Expression const> initializer() const { return m_initializer; }

private:
	std::size_t m_variable_id;
	std::shared_ptr<Expression const> m_initializer;
};

struct FunctionParameter {
	std::size_t variable_id;
	bool is_anonymous;
};

class Function : public Statement {
public:
	explicit Function(std::string_view name, std::vector<FunctionParameter>&& parameters, Types::Id return_type_id, std::shared_ptr<BlockExpression const> body, Span span)
	  : Statement(Types::builtin_void_id, span), m_name(name), m_parameters(std::move(parameters)), m_return_type_id(return_type_id), m_body(body) {}

	virtual void dump(Program const&) const override;

	std::string_view name() const { return m_name; }
	std::vector<FunctionParameter> const& parameters() const { return m_parameters; }
	Types::Id return_type_id() const { return m_return_type_id; }
	std::shared_ptr<BlockExpression const> body() const { return m_body; }

private:
	std::string_view m_name;
	std::vector<FunctionParameter> m_parameters;
	Types::Id m_return_type_id;
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
	  : Statement(Types::builtin_void_id, span), m_body(std::move(body)) {}

	std::shared_ptr<BlockExpression const> m_body;
};

class InfiniteForStatement : public ForStatement {
public:
	explicit InfiniteForStatement(std::shared_ptr<BlockExpression const> body, Span span)
	  : ForStatement(body, span) {}

	virtual void dump(Program const&) const override;
	virtual bool is_infinite() const override { return true; }
};

class ForWithConditionStatement : public ForStatement {
public:
	explicit ForWithConditionStatement(std::shared_ptr<Expression const> condition, std::shared_ptr<BlockExpression const> body, Span span)
	  : ForStatement(body, span), m_condition(condition) {}

	virtual void dump(Program const&) const override;
	virtual bool is_with_condition() const override { return true; }

	std::shared_ptr<Expression const> condition() const { return m_condition; }

private:
	std::shared_ptr<Expression const> m_condition;
};

class ForWithRangeStatement : public ForStatement {
public:
	explicit ForWithRangeStatement(std::size_t range_variable_id, std::shared_ptr<Expression const> range_expression, std::shared_ptr<BlockExpression const> body, Span span)
	  : ForStatement(body, span), m_range_variable_id(range_variable_id), m_range_expression(std::move(range_expression)) {}

	virtual void dump(Program const&) const override;
	virtual bool is_with_range() const override { return true; }

	std::size_t range_variable_id() const { return m_range_variable_id; }
	std::shared_ptr<Expression const> range_expression() const { return m_range_expression; }

private:
	std::size_t m_range_variable_id;
	std::shared_ptr<Expression const> m_range_expression;
};

class ReturnStatement : public Statement {
public:
	explicit ReturnStatement(std::shared_ptr<Expression const> expression, Span span)
	  : Statement(Types::builtin_void_id, span), m_expression(std::move(expression)) {}

	virtual void dump(Program const&) const override;
	virtual bool is_return_statement() const override { return true; }

	std::shared_ptr<Expression const> expression() const { return m_expression; }

private:
	std::shared_ptr<Expression const> m_expression;
};

class Scope {
public:
	explicit Scope(std::optional<std::size_t> parent)
	  : m_parent(parent) {}

	std::optional<std::size_t> parent() const { return m_parent; }

private:
	std::optional<std::size_t> m_parent;
};

struct Variable {
	Types::Id type_id;
	std::string_view name;
	Span declaration_span;
	std::size_t owner_scope_id;
};

class Program {
public:
	explicit Program();

	Types::Id find_or_add_type(Types::Type);
	Types::Id apply_mutability(Types::Id, bool);
	Types::Type const& get_type(Types::Id) const;

	std::optional<std::size_t> find_variable(std::string_view name, std::size_t scope_id) const;
	Variable const& get_variable(std::size_t id) const;
	std::size_t define_variable(Variable&&);

	std::size_t create_scope(std::optional<std::size_t> parent = std::nullopt);

	std::vector<std::shared_ptr<Function const>> const& functions() const { return m_functions; }
	std::shared_ptr<Function const> find_function(std::string_view name) const;
	std::size_t add_function(std::shared_ptr<Function const> function);

	Span span() const { return m_span; }

	void dump_type(Types::Id) const;
	void dump_variable(std::size_t id) const;
	void dump() const;

private:
	std::vector<Types::Type> m_types;
	std::vector<Variable> m_variables;
	std::vector<Scope> m_scopes;
	std::vector<std::shared_ptr<Function const>> m_functions;
	Span m_span;
};

}

}
