#pragma once

#include "AST.hpp"
#include "CheckedAST.hpp"
#include "Error.hpp"
#include "Types.hpp"
#include "utils/Result.hpp"

namespace bo {

class Typechecker {
public:
	explicit Typechecker() = default;

	Result<void, Error> check(std::shared_ptr<AST::Program const> program);

	bool is_checked() const { return m_is_checked; }
	CheckedAST::Program const& program() const {
		assert(m_is_checked);
		return m_program;
	}

private:
	Result<std::size_t, Error> define_variable(Types::Id, std::string_view name, Span declaration_span);

	Result<std::size_t, Error> check_array_size(std::shared_ptr<AST::IntegerLiteral const>);
	Result<Types::Id, Error> check_type(std::shared_ptr<AST::Type const>);
	Result<std::shared_ptr<CheckedAST::Function const>, Error> check_function_declaration(std::shared_ptr<AST::FunctionDeclarationStatement const>);
	Result<std::shared_ptr<CheckedAST::BlockExpression const>, Error> check_block_expression(std::shared_ptr<AST::BlockExpression const>);
	Result<std::shared_ptr<CheckedAST::Statement const>, Error> check_statement(std::shared_ptr<AST::Statement const>);
	Result<std::shared_ptr<CheckedAST::VariableDeclarationStatement const>, Error> check_variable_declaration_statement(std::shared_ptr<AST::VariableDeclarationStatement const>);
	Result<std::shared_ptr<CheckedAST::ForStatement const>, Error> check_for_statement(std::shared_ptr<AST::ForStatement const>);
	Result<std::shared_ptr<CheckedAST::ReturnStatement const>, Error> check_return_statement(std::shared_ptr<AST::ReturnStatement const>);
	Result<std::shared_ptr<CheckedAST::Expression const>, Error> check_expression(std::shared_ptr<AST::Expression const>, Types::Id type_hint = Types::builtin_unknown_id);
	Result<std::shared_ptr<CheckedAST::IntegerLiteral const>, Error> check_integer_literal(std::shared_ptr<AST::IntegerLiteral const>);
	Result<std::shared_ptr<CheckedAST::Identifier const>, Error> check_identifier(std::shared_ptr<AST::Identifier const>);
	Result<std::shared_ptr<CheckedAST::BinaryExpression const>, Error> check_binary_expression(std::shared_ptr<AST::BinaryExpression const>);
	Result<std::shared_ptr<CheckedAST::UnaryExpression const>, Error> check_unary_expression(std::shared_ptr<AST::UnaryExpression const>);
	Result<std::shared_ptr<CheckedAST::AssignmentExpression const>, Error> check_assignment_expression(std::shared_ptr<AST::AssignmentExpression const>);
	Result<std::shared_ptr<CheckedAST::UpdateExpression const>, Error> check_update_expression(std::shared_ptr<AST::UpdateExpression const>);
	Result<std::shared_ptr<CheckedAST::PointerDereferenceExpression const>, Error> check_pointer_dereference_expression(std::shared_ptr<AST::PointerDereferenceExpression const>);
	Result<std::shared_ptr<CheckedAST::AddressOfExpression const>, Error> check_address_of_expression(std::shared_ptr<AST::AddressOfExpression const>);
	Result<std::shared_ptr<CheckedAST::RangeExpression const>, Error> check_range_expression(std::shared_ptr<AST::RangeExpression const>);
	Result<std::shared_ptr<CheckedAST::IfExpression const>, Error> check_if_expression(std::shared_ptr<AST::IfExpression const>);
	Result<std::shared_ptr<CheckedAST::FunctionCallExpression const>, Error> check_function_call_expression(std::shared_ptr<AST::FunctionCallExpression const>);
	Result<std::shared_ptr<CheckedAST::ArrayExpression const>, Error> check_array_expression(std::shared_ptr<AST::ArrayExpression const>, Types::Id type_hint = Types::builtin_unknown_id);
	Result<std::shared_ptr<CheckedAST::ArraySubscriptExpression const>, Error> check_array_subscript_expression(std::shared_ptr<AST::ArraySubscriptExpression const>);

	bool are_types_compatible_for_assignment(Types::Id lhs, Types::Id rhs) const;

	CheckedAST::Program m_program;
	bool m_is_checked { false };

	std::optional<std::size_t> m_current_scope;
	std::optional<Types::Id> m_expected_return_type_id;
};

}
