#pragma once

#include "AST.hpp"
#include "Error.hpp"
#include "Types.hpp"
#include "utils/Result.hpp"

#include <unordered_map>

namespace bo {

struct Variable {
	Types::Id type_id;
	std::string_view name;
	Span declaration_span;
};

class Block {
public:
	explicit Block(std::optional<size_t> parent)
	  : m_parent(parent) {}

	std::optional<Variable> find_variable(std::string_view name) const {
		auto it = m_variables.find(name);
		if (it == m_variables.end()) {
			return std::nullopt;
		}

		return it->second;
	}

	Result<void, Error> define_variable(Variable);

	std::optional<size_t> parent() const { return m_parent; }
	std::size_t return_type_id() const { return m_return_type_id; }
	std::size_t& return_type_id() { return m_return_type_id; }
	bool contains_return_statement() const { return m_contains_return_statement; }
	bool& contains_return_statement() { return m_contains_return_statement; }

private:
	std::optional<size_t> m_parent;
	std::unordered_map<std::string_view, Variable> m_variables;
	// NOTE: Not to be confused with the return type of a function,
	//       it is the type of the value of the block
	std::size_t m_return_type_id { Types::builtin_void_id };
	bool m_contains_return_statement { false };
};

class Function {
public:
	struct Parameter {
		Types::Id type_id;
		std::string_view name;
		bool is_anonymous;
		Span span;
	};

	explicit Function(std::string_view name, std::vector<Parameter>&& parameters, Types::Id return_type_id, std::size_t block)
	  : m_name(name), m_parameters(std::move(parameters)), m_return_type_id(return_type_id), m_block(block) {}

	std::string_view name() const { return m_name; }
	std::vector<Parameter> const& parameters() const { return m_parameters; }
	Types::Id return_type_id() const { return m_return_type_id; }
	std::size_t block() const { return m_block; }

private:
	std::string_view m_name;
	std::vector<Parameter> m_parameters;
	Types::Id m_return_type_id;
	std::size_t m_block;
};

class Typechecker {
public:
	explicit Typechecker(std::shared_ptr<AST::Program const>);

	Result<void, Error> check();

private:
	Types::Id find_or_add_type(Types::Type);
	Types::Id apply_mutability(Types::Id, bool);

	Result<std::size_t, Error> check_array_size(std::shared_ptr<AST::IntegerLiteral const>);
	Result<Types::Id, Error> check_type(std::shared_ptr<AST::Type const>);
	Result<void, Error> check_function_declaration(std::shared_ptr<AST::FunctionDeclarationStatement const>);
	Result<void, Error> check_block_expression(std::shared_ptr<AST::BlockExpression const>);
	Result<Types::Id, Error> check_statement(std::shared_ptr<AST::Statement const>);
	Result<void, Error> check_variable_declaration_statement(std::shared_ptr<AST::VariableDeclarationStatement const>);
	Result<void, Error> check_for_statement(std::shared_ptr<AST::ForStatement const>);
	Result<void, Error> check_return_statement(std::shared_ptr<AST::ReturnStatement const>);
	Result<Types::Id, Error> check_expression(std::shared_ptr<AST::Expression const>);
	Result<Types::Id, Error> check_integer_literal(std::shared_ptr<AST::IntegerLiteral const>);
	Result<Types::Id, Error> check_identifier(std::shared_ptr<AST::Identifier const>);
	Result<Types::Id, Error> check_binary_expression(std::shared_ptr<AST::BinaryExpression const>);
	Result<Types::Id, Error> check_unary_expression(std::shared_ptr<AST::UnaryExpression const>);
	Result<Types::Id, Error> check_assignment_expression(std::shared_ptr<AST::AssignmentExpression const>);
	Result<Types::Id, Error> check_update_expression(std::shared_ptr<AST::UpdateExpression const>);
	Result<Types::Id, Error> check_pointer_dereference_expression(std::shared_ptr<AST::PointerDereferenceExpression const>);
	Result<Types::Id, Error> check_address_of_expression(std::shared_ptr<AST::AddressOfExpression const>);
	Result<Types::Id, Error> check_if_expression(std::shared_ptr<AST::IfExpression const>);
	Result<Types::Id, Error> check_function_call_expression(std::shared_ptr<AST::FunctionCallExpression const>);
	Result<Types::Id, Error> check_array_expression(std::shared_ptr<AST::ArrayExpression const>);
	Result<Types::Id, Error> check_array_subscript_expression(std::shared_ptr<AST::ArraySubscriptExpression const>);

	bool are_types_compatible_for_assignment(Types::Id lhs, Types::Id rhs) const;
	Types::Id find_common_type_for_integers(Types::Id, Types::Id) const;

	std::size_t create_block(std::optional<size_t> parent = {});
	std::size_t define_function(Function);
	std::optional<Variable> find_variable(std::string_view name);
	Result<void, Error> define_variable(Variable);

	std::shared_ptr<AST::Program const> m_program;
	std::vector<Types::Type> m_types;
	std::vector<Block> m_blocks;
	std::optional<size_t> m_current_block;
	std::vector<Function> m_functions;
	std::optional<size_t> m_current_function;
};

}
