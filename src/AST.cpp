#include "AST.hpp"
#include "fmt/base.h"

#include <fmt/core.h>

namespace bo {

namespace AST {

void Type::dump() const {
	fmt::print("{{\"node\":\"Type\",\"span\":[{},{}]", span().start, span().end);
	if (m_inner_type) {
		fmt::print(",\"inner_type\":");
		m_inner_type->dump();
	}

	if (m_array_size) {
		fmt::print(",\"array_size\":");
		m_array_size->dump();
	}

	if (m_name) {
		fmt::print(",\"name\":");
		m_name->dump();
	}

	std::string flags;
#define BO_ENUMERATE_TYPE_FLAG(x, y)     \
	if (m_flags & PF_##x) {                \
		flags += fmt::format("\"{}\",", #x); \
	}
	_BO_ENUMERATE_TYPE_FLAGS
#undef BO_ENUMERATE_TYPE_FLAG
	if (!flags.empty()) {
		flags.pop_back();
	}

	fmt::print(",\"flags\":[{}]", flags);
	fmt::print("}}");
}

void ParenthesizedExpression::dump() const {
	fmt::print("{{\"node\":\"ParenthesizedExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"expression\":");
	m_expression->dump();
	fmt::print("}}");
}

void IntegerLiteral::dump() const {
	fmt::print("{{\"node\":\"IntegerLiteral\",\"span\":[{},{}],\"value\":{:?}}}", span().start, span().end, m_value);
}

void Identifier::dump() const {
	fmt::print("{{\"node\":\"Identifier\",\"span\":[{},{}],\"id\":{:?}}}", span().start, span().end, m_id);
}

void BinaryExpression::dump() const {
	fmt::print("{{\"node\":\"BinaryExpression\",\"span\":[{},{}],", span().start, span().end);
	switch (m_op) {
#define BO_ENUMERATE_BINARY_OPERATOR(x)    \
	case BinaryOperator::x:                  \
		fmt::print("\"operator\":\"{}\"", #x); \
		break;
		_BO_ENUMERATE_BINARY_OPERATORS
#undef BO_ENUMERATE_BINARY_OPERATOR
	}
	fmt::print(",\"lhs\":");
	m_lhs->dump();
	fmt::print(",\"rhs\":");
	m_rhs->dump();
	fmt::print("}}");
}

void UnaryExpression::dump() const {
	fmt::print("{{\"node\":\"UnaryExpression\",\"span\":[{},{}],", span().start, span().end);
	switch (m_op) {
#define BO_ENUMERATE_UNARY_OPERATOR(x)     \
	case UnaryOperator::x:                   \
		fmt::print("\"operator\":\"{}\"", #x); \
		break;
		_BO_ENUMERATE_UNARY_OPERATORS
#undef BO_ENUMERATE_UNARY_OPERATOR
	}
	fmt::print(",\"operand\":");
	m_operand->dump();
	fmt::print("}}");
}

void AssignmentExpression::dump() const {
	fmt::print("{{\"node\":\"AssignmentExpression\",\"span\":[{},{}],", span().start, span().end);
	switch (m_op) {
#define BO_ENUMERATE_ASSIGNMENT_OPERATOR(x) \
	case AssignmentOperator::x:               \
		fmt::print("\"operator\":\"{}\"", #x);  \
		break;
		_BO_ENUMERATE_ASSIGNMENT_OPERATORS
#undef BO_ENUMERATE_ASSIGNMENT_OPERATOR
	}
	fmt::print(",\"lhs\":");
	m_lhs->dump();
	fmt::print(",\"rhs\":");
	m_rhs->dump();
	fmt::print("}}");
}

void UpdateExpression::dump() const {
	fmt::print("{{\"node\":\"UpdateExpression\",\"span\":[{},{}],", span().start, span().end);
	switch (m_op) {
#define BO_ENUMERATE_UPDATE_OPERATOR(x)    \
	case UpdateOperator::x:                  \
		fmt::print("\"operator\":\"{}\"", #x); \
		break;
		_BO_ENUMERATE_UPDATE_OPERATORS
#undef BO_ENUMERATE_UPDATE_OPERATOR
	}
	fmt::print(",\"is_prefixed\":{}", m_is_prefixed);
	fmt::print(",\"operand\":");
	m_operand->dump();
	fmt::print("}}");
}

void PointerDereferenceExpression::dump() const {
	fmt::print("{{\"node\":\"PointerDereferenceExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"operand\":");
	m_operand->dump();
	fmt::print("}}");
}

void AddressOfExpression::dump() const {
	fmt::print("{{\"node\":\"AddressOfExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"operand\":");
	m_operand->dump();
	fmt::print("}}");
}

void RangeExpression::dump() const {
	fmt::print("{{\"node\":\"RangeExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"start\":");
	m_start->dump();
	fmt::print(",\"end\":");
	m_end->dump();
	fmt::print(",\"is_inclusive\":{}", m_is_inclusive);
	fmt::print("}}");
}

void BlockExpression::dump() const {
	fmt::print("{{\"node\":\"BlockExpression\",\"span\":[{},{}],\"statements\":[", span().start, span().end);
	for (std::size_t i = 0; i < m_statements.size(); ++i) {
		m_statements[i]->dump();
		if (i != m_statements.size() - 1) {
			fmt::print(",");
		}
	}
	fmt::print("]}}");
}

void IfExpression::dump() const {
	fmt::print("{{\"node\":\"IfExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"condition\":");
	m_condition->dump();
	fmt::print(",\"then_block\":");
	m_then->dump();
	if (m_else) {
		fmt::print(",\"else_block\":");
		m_else->dump();
	}
	fmt::print("}}");
}

void FunctionCallExpression::dump() const {
	fmt::print("{{\"node\":\"FunctionCallExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"name\":");
	m_name->dump();
	fmt::print(",\"arguments\":[");
	for (std::size_t i = 0; i < m_arguments.size(); ++i) {
		auto const& argument = m_arguments[i];

		fmt::print("{{");
		if (argument.name) {
			fmt::print("\"name\":");
			argument.name->dump();
			fmt::print(",");
		}
		fmt::print("\"value\":");
		argument.value->dump();
		fmt::print("}}");

		if (i != m_arguments.size() - 1) {
			fmt::print(",");
		}
	}
	fmt::print("]}}");
}

void ArrayExpression::dump() const {
	fmt::print("{{\"node\":\"ArrayExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"elements\":[");
	for (std::size_t i = 0; i < m_elements.size(); ++i) {
		m_elements[i]->dump();
		if (i != m_elements.size() - 1) {
			fmt::print(",");
		}
	}
	fmt::print("]}}");
}

void ArraySubscriptExpression::dump() const {
	fmt::print("{{\"node\":\"ArraySubscriptExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"array\":");
	m_array->dump();
	fmt::print(",\"index\":");
	m_index->dump();
	fmt::print("}}");
}

void ExpressionStatement::dump() const {
	fmt::print("{{\"node\":\"ExpressionStatement\",\"span\":[{},{}],\"ends_with_semicolon\":{},\"expression\":", span().start, span().end, m_ends_with_semicolon);
	m_expression->dump();
	fmt::print("}}");
}

void VariableDeclarationStatement::dump() const {
	fmt::print("{{\"node\":\"VariableDeclarationStatement\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"identifier\":");
	m_identifier->dump();
	if (m_type) {
		fmt::print(",\"type\":");
		m_type->dump();
	}

	if (m_expression) {
		fmt::print(",\"expression\":");
		m_expression->dump();
	}
	fmt::print("}}");
}

void FunctionDeclarationStatement::dump() const {
	fmt::print("{{\"node\":\"FunctionDeclarationStatement\",\"span\":[{},{}]", span().start, span().end);
	fmt::print(",\"name\":");
	m_name->dump();
	fmt::print(",\"parameters\":[");
	for (std::size_t i = 0; i < m_parameters.size(); ++i) {
		auto const& parameter = m_parameters[i];
		fmt::print("{{\"name\":");
		parameter.name->dump();
		fmt::print(",\"type\":");
		parameter.type->dump();
		fmt::print(",\"anonymous\":{}}}", parameter.is_anonymous);
		if (i != m_parameters.size() - 1) {
			fmt::print(",");
		}
	}
	fmt::print("]");
	fmt::print(",\"return_type\":");
	m_return_type->dump();
	fmt::print(",\"body\":");
	m_body->dump();
	fmt::print("}}");
}

void InfiniteForStatement::dump() const {
	fmt::print("{{\"node\":\"InfiniteForStatement\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"body\":");
	m_body->dump();
	fmt::print("}}");
}

void ForWithConditionStatement::dump() const {
	fmt::print("{{\"node\":\"ForWithConditionStatement\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"condition\":");
	m_condition->dump();
	fmt::print(",\"body\":");
	m_body->dump();
	fmt::print("}}");
}

void ForWithRangeStatement::dump() const {
	fmt::print("{{\"node\":\"ForWithRangeStatement\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"range_variable\":");
	m_range_variable->dump();
	fmt::print(",\"range_expression\":");
	m_range_expression->dump();
	fmt::print(",\"body\":");
	m_body->dump();
	fmt::print("}}");
}

void ReturnStatement::dump() const {
	fmt::print("{{\"node\":\"ReturnStatement\",\"span\":[{},{}],", span().start, span().end);
	if (m_expression) {
		fmt::print("\"expression\":");
		m_expression->dump();
	}
	fmt::print("}}");
}

void Program::dump() const {
	fmt::print("{{\"node\":\"Program\",\"span\":[{},{}],", span().start, span().end);
	fmt::print("\"functions\":[");
	for (std::size_t i = 0; i < m_functions.size(); ++i) {
		m_functions[i]->dump();
		if (i != m_functions.size() - 1) {
			fmt::print(",");
		}
	}
	fmt::print("]}}");
}

}

}
