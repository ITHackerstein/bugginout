#include "Typechecker.hpp"
#include "Types.hpp"

#include <fmt/core.h>
#include <memory>

namespace bo {

Result<void, Error> Typechecker::check(std::shared_ptr<AST::Program const> parsed_program) {
	for (auto function_declaration : parsed_program->function_declarations()) {
		auto checked_function = TRY(check_function_declaration(function_declaration));
		m_program.add_function(checked_function);
	}

	m_is_checked = true;
	return {};
}

Result<std::size_t, Error> Typechecker::define_variable(Types::Id type_id, std::string_view name, Span declaration_span) {
	if (auto previously_declared_variable_id = m_program.find_variable(name, *m_current_scope)) {
		return Error { "Variable already declared", m_program.get_variable(*previously_declared_variable_id).declaration_span };
	}

	return m_program.define_variable(CheckedAST::Variable { type_id, name, declaration_span, *m_current_scope });
}

Result<std::size_t, Error> Typechecker::check_array_size(std::shared_ptr<AST::IntegerLiteral const> size_literal) {
	int base;
	switch (size_literal->type()) {
	case AST::IntegerLiteral::Type::Decimal:
		base = 10;
		break;
	case AST::IntegerLiteral::Type::Binary:
		base = 2;
		break;
	case AST::IntegerLiteral::Type::Octal:
		base = 8;
		break;
	case AST::IntegerLiteral::Type::Hexadecimal:
		base = 16;
		break;
	}

	std::size_t size;
	auto [_, error] = std::from_chars(size_literal->value().data(), size_literal->value().data() + size_literal->value().size(), size, base);
	if (error == std::errc()) {
		return size;
	}

	return Error { "Invalid array size", size_literal->span() };
}

Result<Types::Id, Error> Typechecker::check_type(std::shared_ptr<AST::Type const> type) {
	using namespace std::literals;

	if (type->is_pointer()) {
		auto pointer_kind = type->is_weak_pointer() ? Types::Pointer::Kind::Weak : Types::Pointer::Kind::Strong;
		auto inner = TRY(check_type(type->inner_type()));
		return m_program.find_or_add_type(Types::Type::pointer(pointer_kind, inner, type->is_mutable()));
	}

	if (type->is_array()) {
		auto size = TRY(check_array_size(type->array_size()));
		auto inner = TRY(check_type(type->inner_type()));
		return m_program.find_or_add_type(Types::Type::array(size, inner, type->is_mutable()));
	}

	if (type->is_slice()) {
		auto inner = TRY(check_type(type->inner_type()));
		return m_program.find_or_add_type(Types::Type::slice(inner, type->is_mutable()));
	}

	// NOTE: This skips the check for the `unknown` which is used only internally for inference
	if (type->name()->id() == "unknown") {
		return Error { "Unknown type", type->span() };
	}

#define BO_ENUMERATE_BUILTIN_TYPE(klass_name, type_name)                                     \
	if (type->name()->id() == #type_name##sv) {                                                \
		return m_program.find_or_add_type(Types::Type::builtin_##type_name(type->is_mutable())); \
	}
	_BO_ENUMERATE_BUILTIN_TYPES
#undef BO_ENUMERATE_BUILTIN_TYPE

	return Error { "Unknown type", type->span() };
}

Result<std::shared_ptr<CheckedAST::Function const>, Error> Typechecker::check_function_declaration(std::shared_ptr<AST::FunctionDeclarationStatement const> function_declaration) {
	auto function_name = function_declaration->name()->id();
	if (m_program.find_function(function_name)) {
		return Error { "Function already declared", function_declaration->name()->span() };
	}

	auto function_return_type_id = TRY(check_type(function_declaration->return_type()));
	m_current_scope = m_program.create_scope();
	m_expected_return_type_id = function_return_type_id;

	std::vector<CheckedAST::FunctionParameter> function_parameters;
	for (auto parameter : function_declaration->parameters()) {
		auto parameter_name = parameter.name->id();
		auto parameter_type_id = TRY(check_type(parameter.type));
		if (m_program.get_type(parameter_type_id).is<Types::Void>()) {
			return Error { "Void type cannot be used as a parameter", parameter.type->span() };
		}

		auto parameter_span = parameter.name->span();
		auto variable_id = TRY(define_variable(parameter_type_id, parameter_name, parameter_span));
		function_parameters.emplace_back(variable_id, parameter.is_anonymous);
	}

	auto checked_block = TRY(check_block_expression(function_declaration->body()));
	if (!are_types_compatible_for_assignment(function_return_type_id, checked_block->type_id())) {
		return Error { "Incompatible return types", function_declaration->return_type()->span() };
	}

	auto checked_function = std::make_shared<CheckedAST::Function const>(function_name, std::move(function_parameters), function_return_type_id, checked_block, function_declaration->span());
	m_expected_return_type_id.reset();
	m_current_scope.reset();
	return checked_function;
}

Result<std::shared_ptr<CheckedAST::Statement const>, Error> Typechecker::check_statement(std::shared_ptr<AST::Statement const> statement) {
	if (statement->is_expression_statement()) {
		auto expression_statement = std::static_pointer_cast<AST::ExpressionStatement const>(statement);
		auto checked_expression = TRY(check_expression(expression_statement->expression()));
		auto checked_expression_statement_type_id = !expression_statement->ends_with_semicolon() ? checked_expression->type_id() : Types::builtin_void_id;
		auto checked_expression_statement = std::make_shared<CheckedAST::ExpressionStatement>(checked_expression, expression_statement->ends_with_semicolon(), checked_expression_statement_type_id, expression_statement->span());
		return std::static_pointer_cast<CheckedAST::Statement const>(checked_expression_statement);
	}

	if (statement->is_variable_declaration()) {
		auto variable_declaration_statement = std::static_pointer_cast<AST::VariableDeclarationStatement const>(statement);
		auto checked_variable_declaration_statement = TRY(check_variable_declaration_statement(variable_declaration_statement));
		return std::static_pointer_cast<CheckedAST::Statement const>(checked_variable_declaration_statement);
	}

	if (statement->is_for_statement()) {
		auto for_statement = std::static_pointer_cast<AST::ForStatement const>(statement);
		auto checked_for_statement = TRY(check_for_statement(for_statement));
		return std::static_pointer_cast<CheckedAST::Statement const>(checked_for_statement);
	}

	if (statement->is_return_statement()) {
		auto return_statement = std::static_pointer_cast<AST::ReturnStatement const>(statement);
		auto checked_return_statement = TRY(check_return_statement(return_statement));
		return std::static_pointer_cast<CheckedAST::Statement const>(checked_return_statement);
	}

	assert(false && "Statement not handled");
}

Result<std::shared_ptr<CheckedAST::VariableDeclarationStatement const>, Error> Typechecker::check_variable_declaration_statement(std::shared_ptr<AST::VariableDeclarationStatement const> variable_declaration_statement) {
	assert(m_current_scope);

	auto variable_name = variable_declaration_statement->identifier()->id();
	auto variable_span = variable_declaration_statement->identifier()->span();

	Types::Id variable_type_id = Types::builtin_unknown_id;
	if (variable_declaration_statement->type()) {
		variable_type_id = m_program.apply_mutability(TRY(check_type(variable_declaration_statement->type())), variable_declaration_statement->is_mutable());

		if (m_program.get_type(variable_type_id).is<Types::Void>()) {
			return Error { "Void type cannot be used as variable type", variable_declaration_statement->type()->span() };
		}
	}

	std::shared_ptr<CheckedAST::Expression const> checked_initializer;
	if (variable_declaration_statement->initializer()) {
		checked_initializer = TRY(check_expression(variable_declaration_statement->initializer(), variable_type_id));
		if (checked_initializer->type_id() == Types::builtin_void_id) {
			return Error { "Void type cannot be used as initializer", variable_declaration_statement->initializer()->span() };
		}

		if (variable_type_id == Types::builtin_unknown_id) {
			variable_type_id = m_program.apply_mutability(checked_initializer->type_id(), variable_declaration_statement->is_mutable());
		} else if (!are_types_compatible_for_assignment(variable_type_id, checked_initializer->type_id())) {
			return Error { "Variable type doesn't match expression type", variable_declaration_statement->span() };
		}
	}

	if (m_program.find_variable(variable_name, *m_current_scope)) {
		return Error { "Variable already declared", variable_declaration_statement->identifier()->span() };
	}

	auto variable_id = TRY(define_variable(variable_type_id, variable_name, variable_span));
	return std::make_shared<CheckedAST::VariableDeclarationStatement const>(variable_id, checked_initializer, variable_declaration_statement->span());
}

Result<std::shared_ptr<CheckedAST::ForStatement const>, Error> Typechecker::check_for_statement(std::shared_ptr<AST::ForStatement const> for_statement) {
	assert(m_current_scope);

	if (for_statement->is_infinite()) {
		auto old_scope = *m_current_scope;
		m_current_scope = m_program.create_scope(old_scope);
		auto checked_body = TRY(check_block_expression(for_statement->body()));
		m_current_scope = old_scope;
		auto checked_infinite_for = std::make_shared<CheckedAST::InfiniteForStatement const>(checked_body, for_statement->span());
		return std::static_pointer_cast<CheckedAST::ForStatement const>(checked_infinite_for);
	}

	if (for_statement->is_with_condition()) {
		auto for_with_condition = std::static_pointer_cast<AST::ForWithConditionStatement const>(for_statement);
		auto checked_condition = TRY(check_expression(for_with_condition->condition()));
		if (!m_program.get_type(checked_condition->type_id()).is<Types::Bool>()) {
			return Error { "For condition must be a boolean expression", for_with_condition->condition()->span() };
		}

		auto old_scope = *m_current_scope;
		m_current_scope = m_program.create_scope(old_scope);
		auto checked_body = TRY(check_block_expression(for_with_condition->body()));
		m_current_scope = old_scope;

		auto checked_for_with_condition = std::make_shared<CheckedAST::ForWithConditionStatement>(checked_condition, checked_body, for_with_condition->span());
		return std::static_pointer_cast<CheckedAST::ForStatement const>(checked_for_with_condition);
	}

	if (for_statement->is_with_range()) {
		auto for_with_range = std::static_pointer_cast<AST::ForWithRangeStatement const>(for_statement);
		auto checked_range_expression = TRY(check_expression(for_with_range->range_expression()));

		Types::Id range_variable_type_id = Types::builtin_unknown_id;
		if (m_program.get_type(checked_range_expression->type_id()).is<Types::Range>()) {
			range_variable_type_id = m_program.get_type(checked_range_expression->type_id()).as<Types::Range>().element_type_id();
		} else if (m_program.get_type(checked_range_expression->type_id()).is<Types::Array>()) {
			range_variable_type_id = m_program.get_type(checked_range_expression->type_id()).as<Types::Array>().inner_type_id();
		} else if (m_program.get_type(checked_range_expression->type_id()).is<Types::Slice>()) {
			range_variable_type_id = m_program.get_type(checked_range_expression->type_id()).as<Types::Slice>().inner_type_id();
		} else {
			return Error { "Range expression must be a range, array or slice", for_with_range->range_expression()->span() };
		}

		auto range_variable_name = for_with_range->range_variable()->id();
		auto range_variable_span = for_with_range->range_variable()->span();

		auto old_scope = *m_current_scope;
		m_current_scope = m_program.create_scope(old_scope);
		auto range_variable_id = TRY(define_variable(range_variable_type_id, range_variable_name, range_variable_span));
		auto checked_body = TRY(check_block_expression(for_with_range->body()));
		m_current_scope = old_scope;

		auto checked_for_with_range = std::make_shared<CheckedAST::ForWithRangeStatement>(range_variable_id, checked_range_expression, checked_body, for_with_range->span());
		return std::static_pointer_cast<CheckedAST::ForStatement const>(checked_for_with_range);
	}

	assert(false && "For statement not handled");
}

Result<std::shared_ptr<CheckedAST::ReturnStatement const>, Error> Typechecker::check_return_statement(std::shared_ptr<AST::ReturnStatement const> return_statement) {
	assert(m_current_scope && m_expected_return_type_id);

	std::shared_ptr<CheckedAST::Expression const> checked_return_value = nullptr;
	if (return_statement->expression()) {
		checked_return_value = TRY(check_expression(return_statement->expression()));
	}

	if (!are_types_compatible_for_assignment(*m_expected_return_type_id, checked_return_value->type_id())) {
		return Error { "Incompatible return types", return_statement->span() };
	}

	return std::make_shared<CheckedAST::ReturnStatement const>(checked_return_value, return_statement->span());
}

Result<std::shared_ptr<CheckedAST::Expression const>, Error> Typechecker::check_expression(std::shared_ptr<AST::Expression const> expression, [[maybe_unused]] Types::Id type_hint) {
	if (expression->is_parenthesized_expression()) {
		return check_expression(std::static_pointer_cast<AST::ParenthesizedExpression const>(expression)->expression(), type_hint);
	}

	if (expression->is_integer_literal()) {
		auto checked_integer_literal = TRY(check_integer_literal(std::static_pointer_cast<AST::IntegerLiteral const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_integer_literal);
	}

	if (expression->is_identifier()) {
		auto checked_identifier = TRY(check_identifier(std::static_pointer_cast<AST::Identifier const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_identifier);
	}

	if (expression->is_binary_expression()) {
		auto checked_binary_expression = TRY(check_binary_expression(std::static_pointer_cast<AST::BinaryExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_binary_expression);
	}

	if (expression->is_unary_expression()) {
		auto checked_unary_expression = TRY(check_unary_expression(std::static_pointer_cast<AST::UnaryExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_unary_expression);
	}

	if (expression->is_assignment_expression()) {
		auto checked_assignment_expression = TRY(check_assignment_expression(std::static_pointer_cast<AST::AssignmentExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_assignment_expression);
	}

	if (expression->is_update_expression()) {
		auto checked_update_expression = TRY(check_update_expression(std::static_pointer_cast<AST::UpdateExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_update_expression);
	}

	if (expression->is_pointer_dereference_expression()) {
		auto checked_pointer_dereference_expression = TRY(check_pointer_dereference_expression(std::static_pointer_cast<AST::PointerDereferenceExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_pointer_dereference_expression);
	}

	if (expression->is_address_of_expression()) {
		auto checked_address_of_expression = TRY(check_address_of_expression(std::static_pointer_cast<AST::AddressOfExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_address_of_expression);
	}

	if (expression->is_range_expression()) {
		auto checked_range_expression = TRY(check_range_expression(std::static_pointer_cast<AST::RangeExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_range_expression);
	}

	if (expression->is_block_expression()) {
		assert(m_current_scope);
		auto old_scope = *m_current_scope;
		old_scope = m_program.create_scope(old_scope);
		auto checked_block = TRY(check_block_expression(std::static_pointer_cast<AST::BlockExpression const>(expression)));
		m_current_scope = old_scope;
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_block);
	}

	if (expression->is_if_expression()) {
		auto checked_if_expression = TRY(check_if_expression(std::static_pointer_cast<AST::IfExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_if_expression);
	}

	if (expression->is_function_call_expression()) {
		auto checked_function_call_expression = TRY(check_function_call_expression(std::static_pointer_cast<AST::FunctionCallExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_function_call_expression);
	}

	if (expression->is_array_expression()) {
		auto checked_array_expression = TRY(check_array_expression(std::static_pointer_cast<AST::ArrayExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_array_expression);
	}

	if (expression->is_array_subscription_expression()) {
		auto checked_array_subscript_expression = TRY(check_array_subscript_expression(std::static_pointer_cast<AST::ArraySubscriptExpression const>(expression)));
		return std::static_pointer_cast<CheckedAST::Expression const>(checked_array_subscript_expression);
	}

	assert(false && "Expression not handled!");
}

Result<std::shared_ptr<CheckedAST::IntegerLiteral const>, Error> Typechecker::check_integer_literal(std::shared_ptr<AST::IntegerLiteral const> integer_literal) {
	auto integer_literal_type_id = Types::builtin_unknown_id;
	if (integer_literal->suffix().empty()) {
		integer_literal_type_id = Types::builtin_i32_id;
	} else if (integer_literal->suffix() == "u8") {
		integer_literal_type_id = Types::builtin_u8_id;
	} else if (integer_literal->suffix() == "u16") {
		integer_literal_type_id = Types::builtin_u16_id;
	} else if (integer_literal->suffix() == "u32") {
		integer_literal_type_id = Types::builtin_u32_id;
	} else if (integer_literal->suffix() == "u64") {
		integer_literal_type_id = Types::builtin_u64_id;
	} else if (integer_literal->suffix() == "usize") {
		integer_literal_type_id = Types::builtin_usize_id;
	} else if (integer_literal->suffix() == "i8") {
		integer_literal_type_id = Types::builtin_i8_id;
	} else if (integer_literal->suffix() == "i16") {
		integer_literal_type_id = Types::builtin_i16_id;
	} else if (integer_literal->suffix() == "i32") {
		integer_literal_type_id = Types::builtin_i32_id;
	} else if (integer_literal->suffix() == "i64") {
		integer_literal_type_id = Types::builtin_i64_id;
	} else if (integer_literal->suffix() == "isize") {
		integer_literal_type_id = Types::builtin_isize_id;
	} else {
		return Error { "Invalid suffix for integer literal", integer_literal->span() };
	}

	return std::make_shared<CheckedAST::IntegerLiteral const>(integer_literal->value(), integer_literal->suffix(), integer_literal_type_id, integer_literal->span());
}

Result<std::shared_ptr<CheckedAST::Identifier const>, Error> Typechecker::check_identifier(std::shared_ptr<AST::Identifier const> identifier) {
	assert(m_current_scope);

	if (auto variable_id = m_program.find_variable(identifier->id(), *m_current_scope)) {
		return std::make_shared<CheckedAST::Identifier const>(*variable_id, m_program.get_variable(*variable_id).type_id, identifier->span());
	}

	return Error { "Unknown identifier", identifier->span() };
}

Result<std::shared_ptr<CheckedAST::BinaryExpression const>, Error> Typechecker::check_binary_expression(std::shared_ptr<AST::BinaryExpression const> binary_expression) {
	assert(m_current_scope);

	auto checked_lhs = TRY(check_expression(binary_expression->lhs()));
	auto checked_rhs = TRY(check_expression(binary_expression->rhs()));

	if (m_program.get_type(checked_lhs->type_id()).is<Types::Void>() || m_program.get_type(checked_rhs->type_id()).is<Types::Void>()) {
		return Error { "Void type cannot be used in binary expression", binary_expression->span() };
	}

	switch (binary_expression->op()) {
	case AST::BinaryOperator::LogicalAnd:
	case AST::BinaryOperator::LogicalOr:
		{
			if (!m_program.get_type(checked_lhs->type_id()).is<Types::Bool>() || !m_program.get_type(checked_rhs->type_id()).is<Types::Bool>()) {
				return Error { "Logical operator requires boolean type", binary_expression->lhs()->span() };
			}

			return std::make_shared<CheckedAST::BinaryExpression const>(checked_lhs, checked_rhs, binary_expression->op(), Types::builtin_bool_id, binary_expression->span());
		}
	case AST::BinaryOperator::BitwiseLeftShift:
	case AST::BinaryOperator::BitwiseRightShift:
		{
			if (!m_program.get_type(checked_lhs->type_id()).is_integer() || !m_program.get_type(checked_rhs->type_id()).is_integer()) {
				return Error { "Incompatible types for binary operation", binary_expression->span() };
			}

			return std::make_shared<CheckedAST::BinaryExpression const>(checked_lhs, checked_rhs, binary_expression->op(), checked_lhs->type_id(), binary_expression->span());
		}
	case AST::BinaryOperator::Addition:
	case AST::BinaryOperator::Subtraction:
	case AST::BinaryOperator::Multiplication:
	case AST::BinaryOperator::Division:
	case AST::BinaryOperator::Modulo:
	case AST::BinaryOperator::BitwiseAnd:
	case AST::BinaryOperator::BitwiseXor:
	case AST::BinaryOperator::BitwiseOr:
		{
			if (!m_program.get_type(checked_lhs->type_id()).is_integer() || !m_program.get_type(checked_rhs->type_id()).is_integer()) {
				return Error { "Incompatible types for binary operation", binary_expression->span() };
			}

			if (m_program.get_type(checked_lhs->type_id()).is_signed() ^ m_program.get_type(checked_rhs->type_id()).is_signed()) {
				return Error { "Incompatible types for binary operation", binary_expression->span() };
			}

			if (m_program.get_type(checked_lhs->type_id()).size() != m_program.get_type(checked_rhs->type_id()).size()) {
				return Error { "Incompatible types for binary operation", binary_expression->span() };
			}

			return std::make_shared<CheckedAST::BinaryExpression const>(checked_lhs, checked_rhs, binary_expression->op(), checked_lhs->type_id(), binary_expression->span());
		}
	case AST::BinaryOperator::LessThan:
	case AST::BinaryOperator::GreaterThan:
	case AST::BinaryOperator::LessThanOrEqualTo:
	case AST::BinaryOperator::GreaterThanOrEqualTo:
		{
			if (m_program.get_type(checked_lhs->type_id()).is_integer() && m_program.get_type(checked_rhs->type_id()).is_integer()) {
				if (!(m_program.get_type(checked_lhs->type_id()).is_signed() ^ m_program.get_type(checked_rhs->type_id()).is_signed())) {
					return std::make_shared<CheckedAST::BinaryExpression const>(checked_lhs, checked_rhs, binary_expression->op(), Types::builtin_bool_id, binary_expression->span());
				}

				return Error { "Comparison between types of different signedness", binary_expression->span() };
			}

			if (m_program.get_type(checked_lhs->type_id()).is<Types::Char>() && m_program.get_type(checked_rhs->type_id()).is<Types::Char>()) {
				return std::make_shared<CheckedAST::BinaryExpression const>(checked_lhs, checked_rhs, binary_expression->op(), Types::builtin_bool_id, binary_expression->span());
			}

			return Error { "Incompatible types for binary operation", binary_expression->span() };
		}
	case AST::BinaryOperator::EqualTo:
	case AST::BinaryOperator::NotEqualTo:
		{
			if (m_program.get_type(checked_lhs->type_id()).is_integer() && m_program.get_type(checked_rhs->type_id()).is_integer()) {
				if (!(m_program.get_type(checked_lhs->type_id()).is_signed() ^ m_program.get_type(checked_rhs->type_id()).is_signed())) {
					return std::make_shared<CheckedAST::BinaryExpression const>(checked_lhs, checked_rhs, binary_expression->op(), Types::builtin_bool_id, binary_expression->span());
				}

				return Error { "Comparison between types of different signedness", binary_expression->span() };
			}

			if (m_program.get_type(checked_lhs->type_id()) == m_program.get_type(checked_rhs->type_id())) {
				return std::make_shared<CheckedAST::BinaryExpression const>(checked_lhs, checked_rhs, binary_expression->op(), Types::builtin_bool_id, binary_expression->span());
			}

			return Error { "Incompatible types for binary operation", binary_expression->span() };
		}
	}

	assert(false && "Binary expression not handled");
}

Result<std::shared_ptr<CheckedAST::UnaryExpression const>, Error> Typechecker::check_unary_expression(std::shared_ptr<AST::UnaryExpression const> unary_expression) {
	assert(m_current_scope);

	auto checked_operand = TRY(check_expression(unary_expression->operand()));

	if (m_program.get_type(checked_operand->type_id()).is<Types::Void>()) {
		return Error { "Void type cannot be used in unary expression", unary_expression->span() };
	}

	switch (unary_expression->op()) {
	case AST::UnaryOperator::Positive:
	case AST::UnaryOperator::Negative:
	case AST::UnaryOperator::BitwiseNot:
		{
			if (!m_program.get_type(checked_operand->type_id()).is_integer()) {
				return Error { "Unary operator requires integer type", unary_expression->operand()->span() };
			}

			return std::make_shared<CheckedAST::UnaryExpression const>(checked_operand, unary_expression->op(), checked_operand->type_id(), unary_expression->span());
		}
	case AST::UnaryOperator::LogicalNot:
		{
			if (!m_program.get_type(checked_operand->type_id()).is<Types::Bool>()) {
				return Error { "Unary operator requires boolean type", unary_expression->operand()->span() };
			}

			return std::make_shared<CheckedAST::UnaryExpression const>(checked_operand, unary_expression->op(), checked_operand->type_id(), unary_expression->span());
		}
	}

	assert(false && "Unary expression not handled");
}

Result<std::shared_ptr<CheckedAST::AssignmentExpression const>, Error> Typechecker::check_assignment_expression(std::shared_ptr<AST::AssignmentExpression const> assignment_expression) {
	assert(m_current_scope);

	auto checked_lhs = TRY(check_expression(assignment_expression->lhs()));
	auto checked_rhs = TRY(check_expression(assignment_expression->rhs()));

	if (m_program.get_type(checked_lhs->type_id()).is<Types::Void>() || m_program.get_type(checked_rhs->type_id()).is<Types::Void>()) {
		return Error { "Void type cannot be used in assignment expression", assignment_expression->span() };
	}

	if (!m_program.get_type(checked_lhs->type_id()).is_mutable()) {
		return Error { "Cannot assign to immutable value", assignment_expression->lhs()->span() };
	}

	switch (assignment_expression->op()) {
	case AST::AssignmentOperator::Assignment:
		{
			if (!are_types_compatible_for_assignment(checked_lhs->type_id(), checked_rhs->type_id())) {
				return Error { "Incompatible types for assignment", assignment_expression->span() };
			}

			return std::make_shared<CheckedAST::AssignmentExpression const>(checked_lhs, checked_rhs, assignment_expression->op(), checked_lhs->type_id(), assignment_expression->span());
		}
	case AST::AssignmentOperator::AdditionAssignment:
	case AST::AssignmentOperator::SubtractionAssignment:
	case AST::AssignmentOperator::MultiplicationAssignment:
	case AST::AssignmentOperator::DivisionAssignment:
	case AST::AssignmentOperator::ModuloAssignment:
	case AST::AssignmentOperator::BitwiseAndAssignment:
	case AST::AssignmentOperator::BitwiseXorAssignment:
	case AST::AssignmentOperator::BitwiseOrAssignment:
		{
			if (!m_program.get_type(checked_lhs->type_id()).is_integer() || !m_program.get_type(checked_rhs->type_id()).is_integer()) {
				return Error { "Incompatible types for binary operation", assignment_expression->span() };
			}

			if (m_program.get_type(checked_lhs->type_id()).is_signed() ^ m_program.get_type(checked_rhs->type_id()).is_signed()) {
				return Error { "Incompatible types for binary operation", assignment_expression->span() };
			}

			if (m_program.get_type(checked_lhs->type_id()).size() != m_program.get_type(checked_rhs->type_id()).size()) {
				return Error { "Incompatible types for binary operation", assignment_expression->span() };
			}

			return std::make_shared<CheckedAST::AssignmentExpression const>(checked_lhs, checked_rhs, assignment_expression->op(), checked_lhs->type_id(), assignment_expression->span());
		}
	case AST::AssignmentOperator::BitwiseLeftShiftAssignment:
	case AST::AssignmentOperator::BitwiseRightShiftAssignment:
		{
			if (!m_program.get_type(checked_lhs->type_id()).is_integer() || !m_program.get_type(checked_rhs->type_id()).is_integer()) {
				return Error { "Incompatible types for binary operation", assignment_expression->span() };
			}

			return std::make_shared<CheckedAST::AssignmentExpression const>(checked_lhs, checked_rhs, assignment_expression->op(), checked_lhs->type_id(), assignment_expression->span());
		}
	case AST::AssignmentOperator::LogicalAndAssignment:
	case AST::AssignmentOperator::LogicalOrAssignment:
		{
			if (!m_program.get_type(checked_lhs->type_id()).is<Types::Bool>() || !m_program.get_type(checked_rhs->type_id()).is<Types::Bool>()) {
				return Error { "Incompatible types for binary operation", assignment_expression->span() };
			}

			return std::make_shared<CheckedAST::AssignmentExpression const>(checked_lhs, checked_rhs, assignment_expression->op(), checked_lhs->type_id(), assignment_expression->span());
		}
	}

	assert(false && "Assignment expression not handled");
}

Result<std::shared_ptr<CheckedAST::UpdateExpression const>, Error> Typechecker::check_update_expression(std::shared_ptr<AST::UpdateExpression const> update_expression) {
	assert(m_current_scope);

	auto checked_operand = TRY(check_expression(update_expression->operand()));

	if (m_program.get_type(checked_operand->type_id()).is<Types::Void>()) {
		return Error { "Void type cannot be used in update expression", update_expression->operand()->span() };
	}

	if (!m_program.get_type(checked_operand->type_id()).is_mutable()) {
		return Error { "Update operator requires mutable type", update_expression->operand()->span() };
	}

	if (!m_program.get_type(checked_operand->type_id()).is_integer()) {
		return Error { "Update operator requires integer type", update_expression->operand()->span() };
	}

	return std::make_shared<CheckedAST::UpdateExpression const>(checked_operand, update_expression->op(), update_expression->is_prefixed(), checked_operand->type_id(), update_expression->span());
}

Result<std::shared_ptr<CheckedAST::PointerDereferenceExpression const>, Error> Typechecker::check_pointer_dereference_expression(std::shared_ptr<AST::PointerDereferenceExpression const> pointer_dereference_expression) {
	assert(m_current_scope);

	auto checked_operand = TRY(check_expression(pointer_dereference_expression->operand()));

	if (!m_program.get_type(checked_operand->type_id()).is<Types::Pointer>()) {
		return Error { "Pointer dereference requires pointer type", pointer_dereference_expression->operand()->span() };
	}

	auto inner_type_id = m_program.get_type(checked_operand->type_id()).as<Types::Pointer>().inner_type_id();
	return std::make_shared<CheckedAST::PointerDereferenceExpression const>(checked_operand, inner_type_id, pointer_dereference_expression->span());
}

Result<std::shared_ptr<CheckedAST::AddressOfExpression const>, Error> Typechecker::check_address_of_expression(std::shared_ptr<AST::AddressOfExpression const> address_of_expression) {
	assert(m_current_scope);

	auto checked_operand = TRY(check_expression(address_of_expression->operand()));

	if (m_program.get_type(checked_operand->type_id()).is<Types::Void>()) {
		return Error { "Void type cannot be used in address-of expression", address_of_expression->operand()->span() };
	}

	auto pointer_type_id = m_program.find_or_add_type(Types::Type::pointer(Types::Pointer::Kind::Strong, checked_operand->type_id(), false));
	return std::make_shared<CheckedAST::AddressOfExpression const>(checked_operand, pointer_type_id, address_of_expression->span());
}

Result<std::shared_ptr<CheckedAST::BlockExpression const>, Error> Typechecker::check_block_expression(std::shared_ptr<AST::BlockExpression const> block_expression) {
	assert(m_current_scope);

	if (block_expression->statements().empty()) {
		std::vector<std::shared_ptr<CheckedAST::Statement const>> empty_checked_statements;
		return std::make_shared<CheckedAST::BlockExpression const>(empty_checked_statements, Types::builtin_void_id, false, *m_current_scope, block_expression->span());
	}

	bool contains_return_statement = false;
	std::vector<std::shared_ptr<CheckedAST::Statement const>> checked_statements;
	for (std::size_t i = 0; i < block_expression->statements().size(); ++i) {
		auto checked_statement = TRY(check_statement(block_expression->statements()[i]));
		if (checked_statement->is_return_statement()) {
			contains_return_statement = true;
		} else if (checked_statement->is_expression_statement()) {
			auto expression_statement = std::static_pointer_cast<CheckedAST::ExpressionStatement const>(checked_statement);
			if (expression_statement->expression()->is_block_expression()) {
				auto inner_block_expression = std::static_pointer_cast<CheckedAST::BlockExpression const>(expression_statement->expression());
				contains_return_statement = contains_return_statement || inner_block_expression->contains_return_statement();
			}
		}
		checked_statements.push_back(checked_statement);
	}

	return std::make_shared<CheckedAST::BlockExpression const>(checked_statements, contains_return_statement, *m_current_scope, checked_statements.back()->type_id(), block_expression->span());
}

Result<std::shared_ptr<CheckedAST::RangeExpression const>, Error> Typechecker::check_range_expression(std::shared_ptr<AST::RangeExpression const> range_expression) {
	assert(m_current_scope);

	auto checked_range_start = TRY(check_expression(range_expression->start()));
	auto checked_range_end = TRY(check_expression(range_expression->end()));

	if (!m_program.get_type(checked_range_start->type_id()).is_integer() || !m_program.get_type(checked_range_end->type_id()).is_integer()) {
		return Error { "Range start and end types must be integers", range_expression->span() };
	}

	auto range_type_id = m_program.find_or_add_type(Types::Type::range(checked_range_start->type_id(), range_expression->is_inclusive()));
	return std::make_shared<CheckedAST::RangeExpression const>(checked_range_start, checked_range_end, range_expression->is_inclusive(), range_type_id, range_expression->span());
}

Result<std::shared_ptr<CheckedAST::IfExpression const>, Error> Typechecker::check_if_expression(std::shared_ptr<AST::IfExpression const> if_expression) {
	assert(m_current_scope);

	auto checked_condition = TRY(check_expression(if_expression->condition()));
	if (!m_program.get_type(checked_condition->type_id()).is<Types::Bool>()) {
		return Error { "If condition must be a boolean expression", if_expression->condition()->span() };
	}

	auto old_scope = *m_current_scope;
	m_current_scope = m_program.create_scope(old_scope);
	auto checked_then = TRY(check_block_expression(if_expression->then()));
	std::shared_ptr<CheckedAST::Expression const> checked_else = nullptr;

	Types::Id if_type_id = Types::builtin_void_id;
	if (if_expression->else_()) {
		checked_else = TRY(check_expression(if_expression->else_()));

		if (checked_then->type_id() != checked_else->type_id()) {
			return Error { "If branches must have the same type", if_expression->span() };
		}

		if_type_id = checked_then->type_id();
	}

	return std::make_shared<CheckedAST::IfExpression const>(checked_condition, checked_then, checked_else, if_type_id, if_expression->span());
}

Result<std::shared_ptr<CheckedAST::FunctionCallExpression const>, Error> Typechecker::check_function_call_expression(std::shared_ptr<AST::FunctionCallExpression const> function_call_expression) {
	auto function = m_program.find_function(function_call_expression->name()->id());
	if (function == nullptr) {
		return Error { "Unknown function", function_call_expression->name()->span() };
	}

	auto const& parameters = function->parameters();
	auto const& arguments = function_call_expression->arguments();

	if (parameters.size() != arguments.size()) {
		return Error { "Function call has wrong number of parameters", function_call_expression->span() };
	}

	std::vector<CheckedAST::FunctionArgument> checked_arguments;
	for (std::size_t i = 0; i < arguments.size(); ++i) {
		auto parameter_declaration = m_program.get_variable(parameters[i].variable_id);
		auto checked_argument_value = TRY(check_expression(arguments[i].value));

		if (m_program.get_type(checked_argument_value->type_id()).is<Types::Void>()) {
			return Error { "Void type cannot be used as an argument", arguments[i].value->span() };
		}

		if (!are_types_compatible_for_assignment(parameter_declaration.type_id, checked_argument_value->type_id())) {
			return Error { "Function call has wrong parameter type", arguments[i].value->span() };
		}

		if (!parameters[i].is_anonymous && arguments[i].name->id() != parameter_declaration.name) {
			return Error { "Function call has wrong parameter name", arguments[i].name->span() };
		}

		checked_arguments.emplace_back(arguments[i].name->id(), checked_argument_value);
	}

	return std::make_shared<CheckedAST::FunctionCallExpression const>(function, checked_arguments, function->return_type_id(), function_call_expression->span());
}

Result<std::shared_ptr<CheckedAST::ArrayExpression const>, Error> Typechecker::check_array_expression(std::shared_ptr<AST::ArrayExpression const> array_expression, [[maybe_unused]] Types::Id type_hint) {
	assert(m_current_scope);

	Types::Id expected_array_inner_type_id = Types::builtin_unknown_id;
	if (type_hint != Types::builtin_unknown_id) {
		if (!m_program.get_type(type_hint).is<Types::Array>()) {
			return Error { "Expected an array here", array_expression->span() };
		}

		auto expected_size = m_program.get_type(type_hint).as<Types::Array>().size();
		auto actual_size = array_expression->elements().size();
		if (expected_size != actual_size) {
			return Error { fmt::format("Expected an array of size {} here, but got {}", expected_size, actual_size), array_expression->span() };
		}

		expected_array_inner_type_id = m_program.get_type(type_hint).as<Types::Array>().inner_type_id();
	}

	Types::Id array_inner_type_id = Types::builtin_unknown_id;
	std::vector<std::shared_ptr<CheckedAST::Expression const>> checked_elements;
	for (auto element : array_expression->elements()) {
		auto checked_element = TRY(check_expression(element));
		if (m_program.get_type(checked_element->type_id()).is<Types::Void>()) {
			return Error { "Void type cannot be used as array element", element->span() };
		}

		if (array_inner_type_id == Types::builtin_unknown_id) {
			array_inner_type_id = checked_element->type_id();
		} else if (array_inner_type_id != checked_element->type_id()) {
			return Error { "Array elements must have the same type", array_expression->span() };
		}

		checked_elements.push_back(checked_element);
	}

	if (expected_array_inner_type_id == Types::builtin_unknown_id) {
		if (array_inner_type_id == Types::builtin_unknown_id) {
			return Error { "Could not infer array expression type", array_expression->span() };
		}

		auto array_type_id = m_program.find_or_add_type(Types::Type::array(checked_elements.size(), array_inner_type_id, false));
		return std::make_shared<CheckedAST::ArrayExpression const>(checked_elements, array_type_id, array_expression->span());
	}

	if (expected_array_inner_type_id != array_inner_type_id) {
		// FIXME: We should add a way to print out types
		return Error { "Expected array of type {{}}, but got {{}}", array_expression->span() };
	}

	auto array_type_id = m_program.find_or_add_type(Types::Type::array(checked_elements.size(), array_inner_type_id, false));
	return std::make_shared<CheckedAST::ArrayExpression const>(checked_elements, array_type_id, array_expression->span());
}

Result<std::shared_ptr<CheckedAST::ArraySubscriptExpression const>, Error> Typechecker::check_array_subscript_expression(std::shared_ptr<AST::ArraySubscriptExpression const> array_subscript_expression) {
	assert(m_current_scope);

	auto checked_array = TRY(check_expression(array_subscript_expression->array()));
	auto checked_index = TRY(check_expression(array_subscript_expression->index()));

	if (!m_program.get_type(checked_index->type_id()).is_integer()) {
		return Error { "Array subscript requires integer type", array_subscript_expression->index()->span() };
	}

	if (m_program.get_type(checked_array->type_id()).is<Types::Array>()) {
		return std::make_shared<CheckedAST::ArraySubscriptExpression const>(checked_array, checked_index, m_program.get_type(checked_array->type_id()).as<Types::Array>().inner_type_id(), array_subscript_expression->span());
	}

	if (m_program.get_type(checked_array->type_id()).is<Types::Slice>()) {
		return std::make_shared<CheckedAST::ArraySubscriptExpression const>(checked_array, checked_index, m_program.get_type(checked_array->type_id()).as<Types::Slice>().inner_type_id(), array_subscript_expression->span());
	}

	return Error { "Array subscript requires array or slice type", array_subscript_expression->array()->span() };
}

bool Typechecker::are_types_compatible_for_assignment(Types::Id lhs, Types::Id rhs) const {
	if (m_program.get_type(lhs).is<Types::Void>() && m_program.get_type(rhs).is<Types::Void>()) {
		return true;
	}

	if (m_program.get_type(lhs).is_integer() && m_program.get_type(rhs).is_integer()) {
		if (!(m_program.get_type(lhs).is_signed() ^ m_program.get_type(rhs).is_signed())) {
			return m_program.get_type(lhs).size() >= m_program.get_type(rhs).size();
		} else if (m_program.get_type(lhs).is_signed()) {
			return m_program.get_type(lhs).size() > m_program.get_type(rhs).size();
		}

		return false;
	}

	if (m_program.get_type(lhs).is<Types::Char>() && m_program.get_type(rhs).is<Types::Char>()) {
		return true;
	}

	if (m_program.get_type(lhs).is<Types::Bool>() && m_program.get_type(rhs).is<Types::Bool>()) {
		return true;
	}

	if (m_program.get_type(lhs).is<Types::Pointer>() && m_program.get_type(rhs).is<Types::Pointer>()) {
		auto const& lhs_pointer = m_program.get_type(lhs).as<Types::Pointer>();
		auto const& rhs_pointer = m_program.get_type(rhs).as<Types::Pointer>();

		if (lhs_pointer.kind() == Types::Pointer::Kind::Strong && rhs_pointer.kind() != Types::Pointer::Kind::Strong) {
			return false;
		}

		auto lhs_inner_type_id = lhs_pointer.inner_type_id();
		auto rhs_inner_type_id = rhs_pointer.inner_type_id();

		return are_types_compatible_for_assignment(lhs_inner_type_id, rhs_inner_type_id);
	}

	if (m_program.get_type(lhs).is<Types::Array>() && m_program.get_type(rhs).is<Types::Array>()) {
		auto const& lhs_array = m_program.get_type(lhs).as<Types::Array>();
		auto const& rhs_array = m_program.get_type(rhs).as<Types::Array>();
		if (lhs_array.size() != rhs_array.size()) {
			return false;
		}

		return lhs_array.inner_type_id() == rhs_array.inner_type_id();
	}

	if (m_program.get_type(lhs).is<Types::Slice>()) {
		if (m_program.get_type(rhs).is<Types::Array>()) {
			auto const& lhs_slice = m_program.get_type(lhs).as<Types::Slice>();
			auto const& rhs_array = m_program.get_type(rhs).as<Types::Array>();

			return lhs_slice.inner_type_id() == rhs_array.inner_type_id();
		} else if (m_program.get_type(rhs).is<Types::Slice>()) {
			auto const& lhs_slice = m_program.get_type(lhs).as<Types::Slice>();
			auto const& rhs_slice = m_program.get_type(rhs).as<Types::Slice>();

			return lhs_slice.inner_type_id() == rhs_slice.inner_type_id();
		}
	}

	return false;
}

}
