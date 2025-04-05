#include "Typechecker.hpp"
#include "Types.hpp"

#include <fmt/core.h>

namespace bo {

Result<void, Error> Block::define_variable(Variable variable) {
	auto it = m_variables.find(variable.name);
	if (it != m_variables.end()) {
		return Error { "Variable already defined", it->second.declaration_span };
	}

	m_variables.emplace(variable.name, variable);
	return {};
}

Typechecker::Typechecker(std::shared_ptr<AST::Program const> program)
  : m_program(std::move(program)) {
#define BO_ENUMERATE_BUILTIN_TYPE(klass_name, type_name) m_types.push_back(Types::Type::builtin_##type_name());
	  _BO_ENUMERATE_BUILTIN_TYPES
#undef BO_ENUMERATE_BUILTIN_TYPE
  }

  Result<void, Error> Typechecker::check() {
	for (auto function_declaration : m_program->function_declarations()) {
		TRY(check_function_declaration(function_declaration));
	}

	return {};
}

Types::Id Typechecker::find_or_add_type(Types::Type type) {
	auto it = std::find(m_types.begin(), m_types.end(), type);

	if (it != m_types.end()) {
		return static_cast<Types::Id>(std::distance(m_types.begin(), it));
	}

	m_types.push_back(type);
	return static_cast<Types::Id>(m_types.size() - 1);
}

Types::Id Typechecker::apply_mutability(Types::Id type_id, bool is_mutable) {
	return find_or_add_type(Types::Type::apply_mutability(m_types[type_id], is_mutable));
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
		return find_or_add_type(Types::Type::pointer(pointer_kind, inner, type->is_mutable()));
	}

	if (type->is_array()) {
		auto size = TRY(check_array_size(type->array_size()));
		auto inner = TRY(check_type(type->inner_type()));
		return find_or_add_type(Types::Type::array(size, inner, type->is_mutable()));
	}

	if (type->is_slice()) {
		auto inner = TRY(check_type(type->inner_type()));
		return find_or_add_type(Types::Type::slice(inner, type->is_mutable()));
	}

#define BO_ENUMERATE_BUILTIN_TYPE(klass_name, type_name)                           \
	if (type->name()->id() == #type_name##sv) {                                      \
		return find_or_add_type(Types::Type::builtin_##type_name(type->is_mutable())); \
	}
	_BO_ENUMERATE_BUILTIN_TYPES
#undef BO_ENUMERATE_BUILTIN_TYPE

	return Error { "Unknown type", type->span() };
}

Result<void, Error> Typechecker::check_function_declaration(std::shared_ptr<AST::FunctionDeclarationStatement const> function_declaration) {
	auto function_name = function_declaration->name()->id();
	if (std::find_if(m_functions.begin(), m_functions.end(), [function_name](auto const& f) { return f.name() == function_name; }) != m_functions.end()) {
		return Error { "Function already declared", function_declaration->name()->span() };
	}

	auto function_return_type_id = TRY(check_type(function_declaration->return_type()));
	m_current_block = create_block();

	std::vector<Function::Parameter> function_parameters;
	for (auto parameter : function_declaration->parameters()) {
		auto parameter_name = parameter.name->id();
		auto parameter_type_id = TRY(check_type(parameter.type));
		auto parameter_span = parameter.name->span();
		function_parameters.emplace_back(parameter_type_id, parameter_name, parameter.is_anonymous, parameter_span);
	}

	m_current_function = define_function(Function { function_name, std::move(function_parameters), function_return_type_id, *m_current_block });
	for (auto const& parameter : m_functions[*m_current_function].parameters()) {
		TRY(define_variable(Variable { parameter.type_id, parameter.name, parameter.span }));
	}

	TRY(check_block_expression(function_declaration->body()));

	if (!m_blocks[*m_current_block].contains_return_statement() && !are_types_compatible_for_assignment(m_blocks[*m_current_block].return_type_id(), function_return_type_id)) {
		return Error { "Incompatible return types", function_declaration->return_type()->span() };
	}

	m_current_block.reset();
	m_current_function.reset();
	return {};
}

Result<Types::Id, Error> Typechecker::check_statement(std::shared_ptr<AST::Statement const> statement) {
	if (statement->is_expression_statement()) {
		auto expression_statement = std::static_pointer_cast<AST::ExpressionStatement const>(statement);
		auto expression_type = TRY(check_expression(expression_statement->expression()));
		return expression_statement->ends_with_semicolon() ? Types::builtin_void_id : expression_type;
	}

	if (statement->is_variable_declaration()) {
		auto variable_declaration_statement = std::static_pointer_cast<AST::VariableDeclarationStatement const>(statement);
		TRY(check_variable_declaration_statement(variable_declaration_statement));
		return Types::builtin_void_id;
	}

	if (statement->is_for_statement()) {
		auto for_statement = std::static_pointer_cast<AST::ForStatement const>(statement);
		TRY(check_for_statement(for_statement));
		return Types::builtin_void_id;
	}

	if (statement->is_return_statement()) {
		auto return_statement = std::static_pointer_cast<AST::ReturnStatement const>(statement);
		TRY(check_return_statement(return_statement));
		return Types::builtin_void_id;
	}

	assert(false && "Statement not handled");
}

Result<void, Error> Typechecker::check_variable_declaration_statement(std::shared_ptr<AST::VariableDeclarationStatement const> variable_declaration_statement) {
	assert(m_current_function && m_current_block);

	auto variable_name = variable_declaration_statement->identifier()->id();
	auto variable_span = variable_declaration_statement->identifier()->span();
	auto variable_type_id = variable_declaration_statement->type() ? TRY(check_type(variable_declaration_statement->type())) : Types::builtin_void_id;

	if (variable_declaration_statement->initializer()) {
		auto expression_type_id = TRY(check_expression(variable_declaration_statement->initializer()));
		if (variable_type_id == Types::builtin_void_id) {
			variable_type_id = apply_mutability(expression_type_id, variable_declaration_statement->starts_with_mut());
		} else if (!are_types_compatible_for_assignment(variable_type_id, expression_type_id)) {
			return Error { "Variable type doesn't match expression type", variable_declaration_statement->span() };
		}
	}

	TRY(define_variable(Variable { variable_type_id, variable_name, variable_span }));
	return {};
}

Result<void, Error> Typechecker::check_for_statement(std::shared_ptr<AST::ForStatement const> for_statement) {
	assert(m_current_function && m_current_block);

	if (for_statement->is_infinite()) {
		auto old_current_block = *m_current_block;
		m_current_block = create_block(old_current_block);
		TRY(check_block_expression(for_statement->body()));
		m_current_block = old_current_block;
		return {};
	}

	if (for_statement->is_with_condition()) {
		auto for_with_condition = std::static_pointer_cast<AST::ForWithConditionStatement const>(for_statement);
		auto condition_type_id = TRY(check_expression(for_with_condition->condition()));
		if (!m_types[condition_type_id].is<Types::Bool>()) {
			return Error { "For condition must be a boolean expression", for_with_condition->condition()->span() };
		}

		auto old_current_block = *m_current_block;
		m_current_block = create_block(old_current_block);
		TRY(check_block_expression(for_with_condition->body()));
		m_current_block = old_current_block;
		return {};
	}

	if (for_statement->is_with_range()) {
		auto for_with_range = std::static_pointer_cast<AST::ForWithRangeStatement const>(for_statement);
		auto range_expression = for_with_range->range_expression();

		Types::Id range_variable_type_id = Types::builtin_void_id;
		if (range_expression->is_range_expression()) {
			auto actual_range_expression = std::static_pointer_cast<AST::RangeExpression const>(range_expression);
			auto start_type_id = TRY(check_expression(actual_range_expression->start()));
			auto end_type_id = TRY(check_expression(actual_range_expression->end()));

			if (!m_types[start_type_id].is_integer() || !m_types[end_type_id].is_integer()) {
				return Error { "Range start and end types must be integers", actual_range_expression->span() };
			}

			range_variable_type_id = start_type_id;
		} else {
			auto range_type_id = TRY(check_expression(for_with_range->range_expression()));
			if (!m_types[range_type_id].is<Types::Array>() && !m_types[range_type_id].is<Types::Slice>()) {
				return Error { "For range must be an array or a slice", for_with_range->range_expression()->span() };
			}

			range_variable_type_id = m_types[range_type_id].is<Types::Array>() ? m_types[range_type_id].as<Types::Array>().inner_type_id() : m_types[range_type_id].as<Types::Slice>().inner_type_id();
		}

		auto range_variable_name = for_with_range->range_variable()->id();
		auto range_variable_span = for_with_range->range_variable()->span();

		auto old_current_block = *m_current_block;
		m_current_block = create_block(old_current_block);
		TRY(define_variable(Variable { range_variable_type_id, range_variable_name, range_variable_span }));
		TRY(check_block_expression(for_with_range->body()));
		m_current_block = old_current_block;
		return {};
	}

	assert(false && "For statement not handled");
}

Result<void, Error> Typechecker::check_return_statement(std::shared_ptr<AST::ReturnStatement const> return_statement) {
	assert(m_current_function && m_current_block);

	auto return_type_id = return_statement->expression() ? TRY(check_expression(return_statement->expression())) : Types::builtin_void_id;
	if (!are_types_compatible_for_assignment(m_functions[*m_current_function].return_type_id(), return_type_id)) {
		return Error { "Incompatible return types", return_statement->span() };
	}

	auto block_id = m_current_block;
	while (block_id) {
		auto& block = m_blocks[*block_id];
		block.contains_return_statement() = true;
		block_id = block.parent();
	}

	return {};
}

Result<Types::Id, Error> Typechecker::check_expression(std::shared_ptr<AST::Expression const> expression) {
	if (expression->is_parenthesized_expression()) {
		return check_expression(std::static_pointer_cast<AST::ParenthesizedExpression const>(expression)->expression());
	}

	if (expression->is_integer_literal()) {
		return check_integer_literal(std::static_pointer_cast<AST::IntegerLiteral const>(expression));
	}

	if (expression->is_identifier()) {
		return check_identifier(std::static_pointer_cast<AST::Identifier const>(expression));
	}

	if (expression->is_binary_expression()) {
		return check_binary_expression(std::static_pointer_cast<AST::BinaryExpression const>(expression));
	}

	if (expression->is_unary_expression()) {
		return check_unary_expression(std::static_pointer_cast<AST::UnaryExpression const>(expression));
	}

	if (expression->is_assignment_expression()) {
		return check_assignment_expression(std::static_pointer_cast<AST::AssignmentExpression const>(expression));
	}

	if (expression->is_update_expression()) {
		return check_update_expression(std::static_pointer_cast<AST::UpdateExpression const>(expression));
	}

	if (expression->is_pointer_dereference_expression()) {
		return check_pointer_dereference_expression(std::static_pointer_cast<AST::PointerDereferenceExpression const>(expression));
	}

	if (expression->is_address_of_expression()) {
		return check_address_of_expression(std::static_pointer_cast<AST::AddressOfExpression const>(expression));
	}

	if (expression->is_range_expression()) {
		return Error { "Range expressions can be used only in a for-in loop", expression->span() };
	}

	if (expression->is_block_expression()) {
		assert(m_current_block);
		auto old_current_block = *m_current_block;
		m_current_block = create_block(old_current_block);
		TRY(check_block_expression(std::static_pointer_cast<AST::BlockExpression const>(expression)));
		auto block_type = m_blocks[*m_current_block].return_type_id();
		m_current_block = old_current_block;
		return block_type;
	}

	if (expression->is_if_expression()) {
		return check_if_expression(std::static_pointer_cast<AST::IfExpression const>(expression));
	}

	if (expression->is_function_call_expression()) {
		return check_function_call_expression(std::static_pointer_cast<AST::FunctionCallExpression const>(expression));
	}

	if (expression->is_array_expression()) {
		return check_array_expression(std::static_pointer_cast<AST::ArrayExpression const>(expression));
	}

	if (expression->is_array_subscription_expression()) {
		return check_array_subscript_expression(std::static_pointer_cast<AST::ArraySubscriptExpression const>(expression));
	}

	assert(false && "Expression not handled!");
}

Result<Types::Id, Error> Typechecker::check_integer_literal(std::shared_ptr<AST::IntegerLiteral const> integer_literal) {
	if (integer_literal->suffix().empty()) {
		return Types::builtin_i32_id;
	}

	if (integer_literal->suffix() == "u8") {
		return Types::builtin_u8_id;
	}

	if (integer_literal->suffix() == "u16") {
		return Types::builtin_u16_id;
	}

	if (integer_literal->suffix() == "u32") {
		return Types::builtin_u32_id;
	}

	if (integer_literal->suffix() == "u64") {
		return Types::builtin_u64_id;
	}

	if (integer_literal->suffix() == "usize") {
		return Types::builtin_usize_id;
	}

	if (integer_literal->suffix() == "i8") {
		return Types::builtin_i8_id;
	}

	if (integer_literal->suffix() == "i16") {
		return Types::builtin_i16_id;
	}

	if (integer_literal->suffix() == "i32") {
		return Types::builtin_i32_id;
	}

	if (integer_literal->suffix() == "i64") {
		return Types::builtin_i64_id;
	}

	if (integer_literal->suffix() == "isize") {
		return Types::builtin_isize_id;
	}

	return Error { "Invalid suffix for integer literal", integer_literal->span() };
}

Result<Types::Id, Error> Typechecker::check_identifier(std::shared_ptr<AST::Identifier const> identifier) {
	assert(m_current_function && m_current_block);

	if (auto variable = find_variable(identifier->id())) {
		auto type_id = variable->type_id;
		return type_id;
	}

	return Error { "Unknown identifier", identifier->span() };
}

Result<Types::Id, Error> Typechecker::check_binary_expression(std::shared_ptr<AST::BinaryExpression const> binary_expression) {
	assert(m_current_function && m_current_block);

	auto lhs_type_id = TRY(check_expression(binary_expression->lhs()));
	auto rhs_type_id = TRY(check_expression(binary_expression->rhs()));

	switch (binary_expression->op()) {
	case AST::BinaryOperator::LogicalAnd:
	case AST::BinaryOperator::LogicalOr:
		{
			if (!m_types[lhs_type_id].is<Types::Bool>()) {
				return Error { "Logical operator requires boolean type", binary_expression->lhs()->span() };
			}

			if (!m_types[rhs_type_id].is<Types::Bool>()) {
				return Error { "Logical operator requires boolean type", binary_expression->rhs()->span() };
			}

			return 11;
		}
	case AST::BinaryOperator::BitwiseLeftShift:
	case AST::BinaryOperator::BitwiseRightShift:
		{
			if (!m_types[lhs_type_id].is_integer() || !m_types[rhs_type_id].is_integer()) {
				return Error { "Incompatible types for binary operation", binary_expression->span() };
			}

			return lhs_type_id;
		}
	case AST::BinaryOperator::Addition:
	case AST::BinaryOperator::Subtraction:
	case AST::BinaryOperator::Multiplication:
	case AST::BinaryOperator::Division:
	case AST::BinaryOperator::Modulo:
	case AST::BinaryOperator::BitwiseAnd:
	case AST::BinaryOperator::BitwiseXor:
	case AST::BinaryOperator::BitwiseOr:
		return find_common_type_for_integers(lhs_type_id, rhs_type_id);
	case AST::BinaryOperator::LessThan:
	case AST::BinaryOperator::GreaterThan:
	case AST::BinaryOperator::LessThanOrEqualTo:
	case AST::BinaryOperator::GreaterThanOrEqualTo:
	case AST::BinaryOperator::EqualTo:
	case AST::BinaryOperator::NotEqualTo:
		{
			if (m_types[lhs_type_id].is_integer() && m_types[rhs_type_id].is_integer()) {
				if ((m_types[lhs_type_id].is_signed() && m_types[rhs_type_id].is_signed()) || (!m_types[lhs_type_id].is_signed() && !m_types[rhs_type_id].is_signed())) {
					return Types::builtin_bool_id;
				}

				return Error { "Comparison between types of different signedness", binary_expression->span() };
			}

			if (m_types[lhs_type_id].is<Types::Char>() && m_types[rhs_type_id].is<Types::Char>()) {
				return Types::builtin_bool_id;
			}

			return Error { "Incompatible types for binary operation", binary_expression->span() };
		}
	}

	assert(false && "Binary expression not handled");
}

Result<Types::Id, Error> Typechecker::check_unary_expression(std::shared_ptr<AST::UnaryExpression const> unary_expression) {
	assert(m_current_function && m_current_block);

	auto operand_type_id = TRY(check_expression(unary_expression->operand()));

	switch (unary_expression->op()) {
	case AST::UnaryOperator::Positive:
	case AST::UnaryOperator::Negative:
	case AST::UnaryOperator::BitwiseNot:
		{
			if (!m_types[operand_type_id].is_integer()) {
				return Error { "Unary operator requires integer type", unary_expression->operand()->span() };
			}

			return operand_type_id;
		}
	case AST::UnaryOperator::LogicalNot:
		{
			if (!m_types[operand_type_id].is<Types::Bool>()) {
				return Error { "Unary operator requires boolean type", unary_expression->operand()->span() };
			}

			return Types::builtin_bool_id;
		}
	}

	assert(false && "Unary expression not handled");
}

Result<Types::Id, Error> Typechecker::check_assignment_expression(std::shared_ptr<AST::AssignmentExpression const> assignment_expression) {
	assert(m_current_function && m_current_block);

	auto lhs_type_id = TRY(check_expression(assignment_expression->lhs()));
	auto rhs_type_id = TRY(check_expression(assignment_expression->rhs()));

	if (!m_types[lhs_type_id].is_mutable()) {
		return Error { "Cannot assign to immutable value", assignment_expression->lhs()->span() };
	}

	switch (assignment_expression->op()) {
	case AST::AssignmentOperator::Assignment:
		{
			if (!are_types_compatible_for_assignment(lhs_type_id, rhs_type_id)) {
				return Error { "Incompatible types for assignment", assignment_expression->span() };
			}

			return lhs_type_id;
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
			if (!m_types[lhs_type_id].is_integer() || !m_types[rhs_type_id].is_integer()) {
				return Error { "Incompatible types for assignment", assignment_expression->span() };
			}

			return lhs_type_id;
		}
	case AST::AssignmentOperator::BitwiseLeftShiftAssignment:
	case AST::AssignmentOperator::BitwiseRightShiftAssignment:
		{
			if (!m_types[lhs_type_id].is_integer() || !m_types[rhs_type_id].is_integer()) {
				return Error { "Incompatible types for binary operation", assignment_expression->span() };
			}

			return lhs_type_id;
		}
	case AST::AssignmentOperator::LogicalAndAssignment:
	case AST::AssignmentOperator::LogicalOrAssignment:
		{
			if (!m_types[lhs_type_id].is<Types::Bool>() || !m_types[rhs_type_id].is<Types::Bool>()) {
				return Error { "Incompatible types for binary operation", assignment_expression->span() };
			}

			return lhs_type_id;
		}
	}

	assert(false && "Assignment expression not handled");
}

Result<Types::Id, Error> Typechecker::check_update_expression(std::shared_ptr<AST::UpdateExpression const> update_expression) {
	assert(m_current_function && m_current_block);

	auto operand_type_id = TRY(check_expression(update_expression->operand()));

	if (!m_types[operand_type_id].is_integer()) {
		return Error { "Update operator requires integer type", update_expression->operand()->span() };
	}

	if (!m_types[operand_type_id].is_mutable()) {
		return Error { "Update operator requires mutable type", update_expression->operand()->span() };
	}

	return Types::builtin_void_id;
}

Result<Types::Id, Error> Typechecker::check_pointer_dereference_expression(std::shared_ptr<AST::PointerDereferenceExpression const> pointer_dereference_expression) {
	assert(m_current_function && m_current_block);

	auto operand_type_id = TRY(check_expression(pointer_dereference_expression->operand()));

	if (!m_types[operand_type_id].is<Types::Pointer>()) {
		return Error { "Pointer dereference requires pointer type", pointer_dereference_expression->operand()->span() };
	}

	return m_types[operand_type_id].as<Types::Pointer>().inner_type_id();
}

Result<Types::Id, Error> Typechecker::check_address_of_expression(std::shared_ptr<AST::AddressOfExpression const> address_of_expression) {
	auto operand_type_id = TRY(check_expression(address_of_expression->operand()));
	return find_or_add_type(Types::Type::pointer(Types::Pointer::Kind::Strong, operand_type_id, false));
}

Result<void, Error> Typechecker::check_block_expression(std::shared_ptr<AST::BlockExpression const> block_expression) {
	assert(m_current_function && m_current_block);

	if (block_expression->statements().empty()) {
		return {};
	}

	for (std::size_t i = 0; i < block_expression->statements().size() - 1; ++i) {
		TRY(check_statement(block_expression->statements()[i]));
	}

	auto last_statement = block_expression->statements().back();
	m_blocks[*m_current_block].return_type_id() = TRY(check_statement(last_statement));
	return {};
}

Result<Types::Id, Error> Typechecker::check_if_expression(std::shared_ptr<AST::IfExpression const> if_expression) {
	assert(m_current_function && m_current_block);

	auto condition_type_id = TRY(check_expression(if_expression->condition()));
	if (!m_types[condition_type_id].is<Types::Bool>()) {
		return Error { "If condition must be a boolean expression", if_expression->condition()->span() };
	}

	auto old_current_block = *m_current_block;
	m_current_block = create_block(old_current_block);
	TRY(check_block_expression(if_expression->then()));
	auto then_type_id = m_blocks[*m_current_block].return_type_id();

	if (if_expression->else_()) {
		auto else_type_id = TRY(check_expression(if_expression->else_()));

		if (then_type_id != else_type_id) {
			return Error { "If branches must have the same type", if_expression->span() };
		}
	}

	return then_type_id;
}

Result<Types::Id, Error> Typechecker::check_function_call_expression(std::shared_ptr<AST::FunctionCallExpression const> function_call_expression) {
	auto function_name = function_call_expression->name()->id();
	auto function_it = std::find_if(m_functions.begin(), m_functions.end(), [function_name](auto const& f) { return f.name() == function_name; });
	if (function_it == m_functions.end()) {
		return Error { "Unknown function", function_call_expression->name()->span() };
	}

	auto const& parameters = function_it->parameters();
	auto const& arguments = function_call_expression->arguments();

	if (parameters.size() != arguments.size()) {
		return Error { "Function call has wrong number of parameters", function_call_expression->span() };
	}

	for (std::size_t i = 0; i < arguments.size(); ++i) {
		auto parameter_type_id = parameters[i].type_id;
		auto argument_type_id = TRY(check_expression(arguments[i].value));
		if (!are_types_compatible_for_assignment(parameter_type_id, argument_type_id)) {
			return Error { "Function call has wrong parameter type", arguments[i].value->span() };
		}

		if (!parameters[i].is_anonymous && arguments[i].name->id() != parameters[i].name) {
			return Error { "Function call has wrong parameter name", arguments[i].name->span() };
		}
	}

	return function_it->return_type_id();
}

Result<Types::Id, Error> Typechecker::check_array_expression(std::shared_ptr<AST::ArrayExpression const> array_expression) {
	assert(m_current_function && m_current_block);

	// FIXME: Handle empty array expressions

	auto array_elements = array_expression->elements();
	Types::Id first_element_type_id = Types::builtin_void_id;
	if (!array_elements.empty()) {
		first_element_type_id = TRY(check_expression(array_elements[0]));
		for (std::size_t i = 1; i < array_elements.size(); ++i) {
			auto element_type_id = TRY(check_expression(array_elements[i]));
			if (first_element_type_id != element_type_id) {
				return Error { "Array elements must have the same type", array_expression->span() };
			}
		}
	}

	return find_or_add_type(Types::Type::array(array_elements.size(), first_element_type_id, false));
}

Result<Types::Id, Error> Typechecker::check_array_subscript_expression(std::shared_ptr<AST::ArraySubscriptExpression const> array_subscript_expression) {
	assert(m_current_function && m_current_block);

	auto array_type_id = TRY(check_expression(array_subscript_expression->array()));
	auto index_type_id = TRY(check_expression(array_subscript_expression->index()));

	if (!m_types[array_type_id].is<Types::Array>() && !m_types[array_type_id].is<Types::Slice>()) {
		return Error { "Array subscript requires array or slice type", array_subscript_expression->array()->span() };
	}

	if (!m_types[index_type_id].is_integer()) {
		return Error { "Array subscript requires integer type", array_subscript_expression->index()->span() };
	}

	return m_types[array_type_id].is<Types::Array>() ? m_types[array_type_id].as<Types::Array>().inner_type_id() : m_types[array_type_id].as<Types::Slice>().inner_type_id();
}

bool Typechecker::are_types_compatible_for_assignment(Types::Id lhs, Types::Id rhs) const {
	if (m_types[lhs].is<Types::Void>() && m_types[rhs].is<Types::Void>()) {
		return true;
	}

	if (m_types[lhs].is_integer() && m_types[lhs].is_integer()) {
		return true;
	}

	if (m_types[lhs].is<Types::Char>() && m_types[rhs].is<Types::Char>()) {
		return true;
	}

	if (m_types[lhs].is<Types::Bool>() && m_types[rhs].is<Types::Bool>()) {
		return true;
	}

	if (m_types[lhs].is<Types::Pointer>() && m_types[rhs].is<Types::Pointer>()) {
		auto const& lhs_pointer = m_types[lhs].as<Types::Pointer>();
		auto const& rhs_pointer = m_types[rhs].as<Types::Pointer>();

		if (lhs_pointer.kind() == Types::Pointer::Kind::Strong && rhs_pointer.kind() != Types::Pointer::Kind::Strong) {
			return false;
		}

		auto lhs_inner_type_id = lhs_pointer.inner_type_id();
		auto rhs_inner_type_id = rhs_pointer.inner_type_id();
		return are_types_compatible_for_assignment(lhs_inner_type_id, rhs_inner_type_id);
	}

	if (m_types[lhs].is<Types::Array>() && m_types[rhs].is<Types::Array>()) {
		auto const& lhs_array = m_types[lhs].as<Types::Array>();
		auto const& rhs_array = m_types[rhs].as<Types::Array>();
		if (lhs_array.size() != rhs_array.size()) {
			return false;
		}

		return lhs_array.inner_type_id() == rhs_array.inner_type_id();
	}

	if (m_types[lhs].is<Types::Slice>()) {
		if (m_types[rhs].is<Types::Array>()) {
			auto const& lhs_slice = m_types[lhs].as<Types::Slice>();
			auto const& rhs_array = m_types[rhs].as<Types::Array>();

			return lhs_slice.inner_type_id() == rhs_array.inner_type_id();
		} else if (m_types[rhs].is<Types::Slice>()) {
			auto const& lhs_slice = m_types[lhs].as<Types::Slice>();
			auto const& rhs_slice = m_types[rhs].as<Types::Slice>();

			return lhs_slice.inner_type_id() == rhs_slice.inner_type_id();
		}
	}

	return false;
}

Types::Id Typechecker::find_common_type_for_integers(Types::Id lhs, Types::Id rhs) const {
	assert(m_types[lhs].is_integer() && m_types[rhs].is_integer());

	if (lhs == rhs) {
		return lhs;
	}

	if ((m_types[lhs].is_signed() && m_types[rhs].is_signed()) || (!m_types[lhs].is_signed() && !m_types[rhs].is_signed())) {
		if (m_types[rhs].size() < m_types[lhs].size()) {
			return lhs;
		}

		return rhs;
	}

	if (m_types[lhs].is_signed()) {
		std::swap(lhs, rhs);
	}

	if (m_types[rhs].size() <= m_types[lhs].size()) {
		return lhs;
	}

	return rhs;
}

std::size_t Typechecker::create_block(std::optional<size_t> parent) {
	m_blocks.emplace_back(parent);
	return m_blocks.size() - 1;
}

std::size_t Typechecker::define_function(Function function) {
	assert(std::find_if(m_functions.begin(), m_functions.end(), [&](auto const& f) { return f.name() == function.name(); }) == m_functions.end());
	m_functions.push_back(function);
	return m_functions.size() - 1;
}

std::optional<Variable> Typechecker::find_variable(std::string_view name) {
	assert(m_current_block);
	auto block = m_current_block;

	do {
		auto variable = m_blocks[*block].find_variable(name);
		if (variable) {
			return *variable;
		}

		block = m_blocks[*block].parent();
	} while (block);

	return {};
}

Result<void, Error> Typechecker::define_variable(Variable variable) {
	assert(m_current_block);
	auto block = m_current_block;
	do {
		auto other_variable = m_blocks[*block].find_variable(variable.name);
		if (other_variable) {
			return Error { "Variable shadows already declared variable", other_variable->declaration_span };
		}

		block = m_blocks[*block].parent();
	} while (block);

	TRY(m_blocks[*m_current_block].define_variable(variable));
	return {};
}

}
