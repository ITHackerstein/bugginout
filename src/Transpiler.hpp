#pragma once

#include "CheckedAST.hpp"
#include "Error.hpp"
#include "utils/Result.hpp"

#include <sstream>

namespace bo {

class Transpiler {
public:
	explicit Transpiler(CheckedAST::Program const& program)
	  : m_program(program) {}

	Result<std::string, Error> transpile();

private:
	void add_new_line();
	void add_prelude();

	enum class IgnoreFirstQualifier {
		Yes,
		No
	};

	Result<void, Error> transpile_type(Types::Id, IgnoreFirstQualifier = IgnoreFirstQualifier::No);
	void transpile_binary_operator(AST::BinaryOperator);
	void transpile_unary_operator(AST::UnaryOperator);
	void transpile_assignment_operator(AST::AssignmentOperator);
	void transpile_update_operator(AST::UpdateOperator);

	enum class LastBlockStatementTreatment {
		AsExpression,
		AsReturnStatement,
		StoreInVariable,
		Ignore
	};

	Result<void, Error> transpile_statement(std::shared_ptr<CheckedAST::Statement const>);
	Result<void, Error> transpile_variable_declaration_statement(std::shared_ptr<CheckedAST::VariableDeclarationStatement const>);
	Result<void, Error> transpile_function(std::shared_ptr<CheckedAST::Function const>);
	Result<void, Error> transpile_for_statement(std::shared_ptr<CheckedAST::ForStatement const>);
	Result<void, Error> transpile_return_statement(std::shared_ptr<CheckedAST::ReturnStatement const>);
	Result<void, Error> transpile_expression(std::shared_ptr<CheckedAST::Expression const>);
	Result<void, Error> transpile_integer_literal(std::shared_ptr<CheckedAST::IntegerLiteral const>);
	Result<void, Error> transpile_char_literal(std::shared_ptr<CheckedAST::CharLiteral const>);
	Result<void, Error> transpile_boolean_literal(std::shared_ptr<CheckedAST::BooleanLiteral const>);
	Result<void, Error> transpile_identifier(std::shared_ptr<CheckedAST::Identifier const>);
	Result<void, Error> transpile_binary_expression(std::shared_ptr<CheckedAST::BinaryExpression const>);
	Result<void, Error> transpile_unary_expression(std::shared_ptr<CheckedAST::UnaryExpression const>);
	Result<void, Error> transpile_assignment_expression(std::shared_ptr<CheckedAST::AssignmentExpression const>);
	Result<void, Error> transpile_update_expression(std::shared_ptr<CheckedAST::UpdateExpression const>);
	Result<void, Error> transpile_pointer_dereference_expression(std::shared_ptr<CheckedAST::PointerDereferenceExpression const>);
	Result<void, Error> transpile_address_of_expression(std::shared_ptr<CheckedAST::AddressOfExpression const>);
	Result<void, Error> transpile_range_expression(std::shared_ptr<CheckedAST::RangeExpression const>);
	Result<void, Error> transpile_block_expression(std::shared_ptr<CheckedAST::BlockExpression const>, LastBlockStatementTreatment = LastBlockStatementTreatment::AsExpression);
	Result<void, Error> transpile_if_expression(std::shared_ptr<CheckedAST::IfExpression const>);
	Result<void, Error> transpile_function_call_expression(std::shared_ptr<CheckedAST::FunctionCallExpression const>);
	Result<void, Error> transpile_array_expression(std::shared_ptr<CheckedAST::ArrayExpression const>);
	Result<void, Error> transpile_array_subscript_expression(std::shared_ptr<CheckedAST::ArraySubscriptExpression const>);

	CheckedAST::Program const& m_program;
	std::stringstream m_code;
	int m_indent_level { 0 };
	int m_temp_variable_iota { 0 };
};

}
