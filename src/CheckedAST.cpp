#include "CheckedAST.hpp"

#include <fmt/core.h>

// FIXME: Bring back dumping here too

namespace bo {

namespace CheckedAST {

void ParenthesizedExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"ParenthesizedExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"expression\":");
	m_expression->dump(program);
	fmt::print("}}");
}

void IntegerLiteral::dump(Program const& program) const {
	fmt::print("{{\"node\":\"IntegerLiteral\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"value\":{:?}", m_value);
	fmt::print(",\"suffix\":{:?}", m_suffix);
	fmt::print("}}");
}

void Identifier::dump(Program const& program) const {
	fmt::print("{{\"node\":\"Identifier\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"variable\":");
	program.dump_variable(m_variable_id);
	fmt::print("}}");
}

void BinaryExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"BinaryExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	switch (m_op) {
#define BO_ENUMERATE_BINARY_OPERATOR(x)     \
	case AST::BinaryOperator::x:              \
		fmt::print(",\"operator\":\"{}\"", #x); \
		break;
		_BO_ENUMERATE_BINARY_OPERATORS
#undef BO_ENUMERATE_BINARY_OPERATOR
	}
	fmt::print(",\"lhs\":");
	m_lhs->dump(program);
	fmt::print(",\"rhs\":");
	m_rhs->dump(program);
	fmt::print("}}");
}

void UnaryExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"UnaryExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	switch (m_op) {
#define BO_ENUMERATE_UNARY_OPERATOR(x)      \
	case AST::UnaryOperator::x:               \
		fmt::print(",\"operator\":\"{}\"", #x); \
		break;
		_BO_ENUMERATE_UNARY_OPERATORS
#undef BO_ENUMERATE_UNARY_OPERATOR
	}
	fmt::print(",\"operand\":");
	m_operand->dump(program);
	fmt::print("}}");
}

void AssignmentExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"AssignmentExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	switch (m_op) {
#define BO_ENUMERATE_ASSIGNMENT_OPERATOR(x) \
	case AST::AssignmentOperator::x:          \
		fmt::print(",\"operator\":\"{}\"", #x); \
		break;
		_BO_ENUMERATE_ASSIGNMENT_OPERATORS
#undef BO_ENUMERATE_ASSIGNMENT_OPERATOR
	}
	fmt::print(",\"lhs\":");
	m_lhs->dump(program);
	fmt::print(",\"rhs\":");
	m_rhs->dump(program);
	fmt::print("}}");
}

void UpdateExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"UpdateExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	switch (m_op) {
#define BO_ENUMERATE_UPDATE_OPERATOR(x)     \
	case AST::UpdateOperator::x:              \
		fmt::print(",\"operator\":\"{}\"", #x); \
		break;
		_BO_ENUMERATE_UPDATE_OPERATORS
#undef BO_ENUMERATE_UPDATE_OPERATOR
	}
	fmt::print(",\"is_prefixed\":{}", m_is_prefixed);
	fmt::print(",\"operand\":");
	m_operand->dump(program);
	fmt::print("}}");
}

void PointerDereferenceExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"PointerDereferenceExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
}

void AddressOfExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"AddressOfExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"operand\":");
	m_operand->dump(program);
	fmt::print("}}");
}

void RangeExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"RangeExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"is_inclusive\":{}", m_is_inclusive);
	fmt::print(",\"start\":");
	m_start->dump(program);
	fmt::print(",\"end\":");
	m_end->dump(program);
	fmt::print("}}");
}

void BlockExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"BlockExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"contains_return_statement\":{}", m_contains_return_statement);
	fmt::print(",\"statements\":[");
	for (std::size_t i = 0; i < m_statements.size(); ++i) {
		m_statements[i]->dump(program);
		if (i != m_statements.size() - 1) {
			fmt::print(",");
		}
	}
	fmt::print("]}}");
}

void IfExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"IfExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"condition\":");
	m_condition->dump(program);
	fmt::print(",\"then_block\":");
	m_then->dump(program);
	if (m_else) {
		fmt::print(",\"else_block\":");
		m_else->dump(program);
	}
	fmt::print("}}");
}

void FunctionCallExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"FunctionCallExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"function\":");
	m_function->dump(program);
	fmt::print(",\"arguments\":[");
	for (std::size_t i = 0; i < m_arguments.size(); ++i) {
		auto const& argument = m_arguments[i];

		fmt::print("{{");
		fmt::print("\"name\":{:?}", argument.name);
		fmt::print(",");
		fmt::print("\"value\":");
		argument.value->dump(program);
		fmt::print("}}");

		if (i != m_arguments.size() - 1) {
			fmt::print(",");
		}
	}
	fmt::print("]}}");
}

void ArrayExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"ArrayExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"elements\":[");
	for (std::size_t i = 0; i < m_elements.size(); ++i) {
		m_elements[i]->dump(program);
		if (i != m_elements.size() - 1) {
			fmt::print(",");
		}
	}
	fmt::print("]}}");
}

void ArraySubscriptExpression::dump(Program const& program) const {
	fmt::print("{{\"node\":\"ArraySubscriptExpression\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"array\":");
	m_array->dump(program);
	fmt::print(",\"index\":");
	m_index->dump(program);
	fmt::print("}}");
}

void ExpressionStatement::dump(Program const& program) const {
	fmt::print("{{\"node\":\"ExpressionStatement\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"ends_with_semicolon\":{}", m_ends_with_semicolon);
	fmt::print(",\"expression\":");
	m_expression->dump(program);
	fmt::print("}}");
}

void VariableDeclarationStatement::dump(Program const& program) const {
	fmt::print("{{\"node\":\"VariableDeclarationStatement\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"variable\":");
	program.dump_variable(m_variable_id);
	if (m_initializer) {
		fmt::print(",\"initializer\":");
		m_initializer->dump(program);
	}
	fmt::print("}}");
}

void Function::dump(Program const& program) const {
	fmt::print("{{\"node\":\"Function\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"name\":{:?}", m_name);
	fmt::print(",\"return_type\":");
	program.dump_type(m_return_type_id);
	fmt::print(",\"parameters\":[");
	for (std::size_t i = 0; i < m_parameters.size(); ++i) {
		auto const& parameter = m_parameters[i];
		fmt::print("{{");
		fmt::print("\"is_anonymous\":{}", parameter.is_anonymous);
		fmt::print(",\"variable\":");
		program.dump_variable(parameter.variable_id);
		fmt::print("}}");

		if (i != m_parameters.size() - 1) {
			fmt::print(",");
		}
	}
	fmt::print("]");
	fmt::print(",\"body\":");
	m_body->dump(program);
	fmt::print("}}");
}

void InfiniteForStatement::dump(Program const& program) const {
	fmt::print("{{\"node\":\"InfiniteForStatement\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"body\":");
	m_body->dump(program);
	fmt::print("}}");
}

void ForWithConditionStatement::dump(Program const& program) const {
	fmt::print("{{\"node\":\"ForWithConditionStatement\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"condition\":");
	m_condition->dump(program);
	fmt::print(",\"body\":");
	m_body->dump(program);
	fmt::print("}}");
}

void ForWithRangeStatement::dump(Program const& program) const {
	fmt::print("{{\"node\":\"ForWithRangeStatement\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"range_variable\":");
	program.dump_variable(m_range_variable_id);
	fmt::print(",\"range_expression\":");
	m_range_expression->dump(program);
	fmt::print(",\"body\":");
	m_body->dump(program);
	fmt::print("}}");
}

void ReturnStatement::dump(Program const& program) const {
	fmt::print("{{\"node\":\"ReturnStatement\"");
	fmt::print(",\"type\":");
	program.dump_type(type_id());
	fmt::print(",\"span\":[{},{}]", span().start, span().end);
	if (m_expression) {
		fmt::print(",\"expression\":");
		m_expression->dump(program);
	}
	fmt::print("}}");
}

Program::Program() {
#define BO_ENUMERATE_BUILTIN_TYPE(klass_name, type_name) m_types.push_back(Types::Type::builtin_##type_name());
	_BO_ENUMERATE_BUILTIN_TYPES
#undef BO_ENUMERATE_BUILTIN_TYPE
}

Types::Id Program::find_or_add_type(Types::Type type) {
	auto it = std::find(m_types.begin(), m_types.end(), type);

	if (it != m_types.end()) {
		return static_cast<Types::Id>(std::distance(m_types.begin(), it));
	}

	m_types.push_back(type);
	return static_cast<Types::Id>(m_types.size() - 1);
}

Types::Id Program::apply_mutability(Types::Id type_id, bool is_mutable) {
	return find_or_add_type(Types::Type::apply_mutability(m_types[type_id], is_mutable));
}

Types::Type const& Program::get_type(Types::Id type_id) const {
	return m_types[type_id];
}

std::optional<std::size_t> Program::find_variable(std::string_view name, std::size_t scope_id) const {
	for (std::size_t variable_id = 0; variable_id < m_variables.size(); ++variable_id) {
		if (m_variables[variable_id].name != name) {
			continue;
		}

		std::size_t current_scope_id = scope_id;
		while (true) {
			if (current_scope_id == m_variables[variable_id].owner_scope_id) {
				return variable_id;
			}

			if (!m_scopes[current_scope_id].parent()) {
				break;
			}

			current_scope_id = *m_scopes[current_scope_id].parent();
		}
	}

	return std::nullopt;
}

Variable const& Program::get_variable(std::size_t id) const {
	return m_variables[id];
}

std::size_t Program::define_variable(Variable&& variable) {
	assert(!find_variable(variable.name, variable.owner_scope_id));
	m_variables.push_back(std::move(variable));
	return m_variables.size() - 1;
}

std::size_t Program::create_scope(std::optional<std::size_t> parent) {
	m_scopes.emplace_back(parent);
	return m_scopes.size() - 1;
}

std::shared_ptr<Function const> Program::find_function(std::string_view name) const {
	auto it = std::find_if(m_functions.begin(), m_functions.end(), [&](auto const& f) { return f->name() == name; });
	return it != m_functions.end() ? *it : nullptr;
}

std::size_t Program::add_function(std::shared_ptr<Function const> function) {
	auto it = std::find_if(m_functions.begin(), m_functions.end(), [&](auto const& f) { return f->name() == function->name(); });
	assert(it == m_functions.end());
	m_functions.push_back(std::move(function));
	m_span = Span::merge(m_span, m_functions.back()->span());
	return m_functions.size() - 1;
}

void Program::dump_type(Types::Id id) const {
	auto const& type = m_types[id];

	if (type.is_builtin()) {
#define BO_ENUMERATE_BUILTIN_TYPE(klass_name, type_name) \
	if (type.is<Types::klass_name>()) {                    \
		fmt::print("\"{}\"", #type_name);                    \
	}
		_BO_ENUMERATE_BUILTIN_TYPES

		return;
	}

	fmt::print("{{");
	if (type.is<Types::Pointer>()) {
		auto const& pointer = type.as<Types::Pointer>();
		fmt::print("\"name\":\"pointer\"");
		fmt::print(",\"inner_type\":");
		dump_type(pointer.inner_type_id());
	} else if (type.is<Types::Array>()) {
		auto const& array = type.as<Types::Array>();
		fmt::print("\"name\":\"array\"");
		fmt::print(",\"size\":{}", array.size());
		fmt::print(",\"inner_type\":");
		dump_type(array.inner_type_id());
	} else if (type.is<Types::Slice>()) {
		auto const& slice = type.as<Types::Slice>();
		fmt::print("\"name\":\"slice\"");
		fmt::print(",\"inner_type\":");
		dump_type(slice.inner_type_id());
	} else if (type.is<Types::Range>()) {
		auto const& range = type.as<Types::Range>();
		fmt::print("\"name\":\"range\"");
		fmt::print(",\"is_inclusive\":{}", range.is_inclusive());
		fmt::print(",\"element_type\":");
		dump_type(range.element_type_id());
	} else {
		assert(false && "Unknown type");
	}
	fmt::print("}}");
}

void Program::dump_variable(std::size_t id) const {
	auto const& variable = m_variables[id];
	fmt::print("{{");
	fmt::print("\"name\":{:?}", variable.name);
	fmt::print(",\"type\":");
	dump_type(variable.type_id);
	fmt::print(",\"declaration_span\":[{},{}]", variable.declaration_span.start, variable.declaration_span.end);
	fmt::print("}}");
}

void Program::dump() const {
	fmt::print("{{\"node\":\"Program\"");
	fmt::print(",\"span\":[{},{}]", m_span.start, m_span.end);
	fmt::print(",\"functions\":[");
	for (std::size_t i = 0; i < m_functions.size(); ++i) {
		m_functions[i]->dump(*this);

		if (i != m_functions.size() - 1) {
			fmt::print(",");
		}
	}
	fmt::print("]}}");
}

}

}
