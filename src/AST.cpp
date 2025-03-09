#include "AST.hpp"
#include "fmt/base.h"

#include <fmt/core.h>

namespace bo {

namespace AST {

void Type::dump() const {
	fmt::print("{{\"node\":\"Type\",\"span\":[{},{}],\"type\":{:?},\"is_mutable\":{}}}", span().start, span().end, m_type, m_is_mutable);
}

void ExpressionStatement::dump() const {
	fmt::print("{{\"node\":\"ExpressionStatement\",\"span\":[{},{}],\"expression\":", span().start, span().end);
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
#define BO_ENUMERATE_BINARY_OPERATOR(x)     \
	case BinaryOperator::x:                   \
		fmt::print("\"operator\":\"{}\",", #x); \
		break;
		_BO_ENUMERATE_BINARY_OPERATORS
#undef BO_ENUMERATE_BINARY_OPERATOR
	}
	fmt::print("\"lhs\":");
	m_lhs->dump();
	fmt::print(",\"rhs\":");
	m_rhs->dump();
	fmt::print("}}");
}

void UnaryExpression::dump() const {
	fmt::print("{{\"node\":\"UnaryExpression\",\"span\":[{},{}],", span().start, span().end);
	switch (m_op) {
#define BO_ENUMERATE_UNARY_OPERATOR(x)      \
	case UnaryOperator::x:                    \
		fmt::print("\"operator\":\"{}\",", #x); \
		break;
		_BO_ENUMERATE_UNARY_OPERATORS
#undef BO_ENUMERATE_UNARY_OPERATOR
	}
	fmt::print("\"operand\":");
	m_operand->dump();
	fmt::print("}}");
}

void AssignmentExpression::dump() const {
	fmt::print("{{\"node\":\"AssignmentExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::println("\"lhs\":");
	m_lhs->dump();
	fmt::print(",\"rhs\":");
	m_rhs->dump();
	fmt::println("}}");
}

void VariableDeclarationStatement::dump() const {
	fmt::print("{{\"node\":\"VariableDeclarationStatement\",\"span\":[{},{}],", span().start, span().end);
	fmt::println("\"identifier\":");
	m_identifier->dump();
	if (*m_type) {
		fmt::print(",\"type\":");
		(*m_type)->dump();
	}

	if (*m_expression) {
		fmt::print(",\"expression\":");
		(*m_expression)->dump();
	}
	fmt::println("}}");
}

void BlockExpression::dump() const {
	fmt::print("{{\"node\":\"BlockExpression\",\"span\":[{},{}],\"statements\":[", span().start, span().end);
	for (auto const& statement : m_statements) {
		statement->dump();
		fmt::print(",");
	}
	std::visit([](auto const& node) { node->dump(); }, m_last_expression_or_statement);
	fmt::print("]}}");
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

void IfExpression::dump() const {
	fmt::print("{{\"node\":\"IfExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::println("\"condition\":");
	m_condition->dump();
	fmt::println(",\"then_block\":");
	m_then_block->dump();
	if (m_else_block) {
		fmt::println(",\"else_block\":");
		(*m_else_block)->dump();
	}
	fmt::print("}}");
}

void InfiniteForExpression::dump() const {
	fmt::print("{{\"node\":\"InfiniteForExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::println("\"body\":");
	m_body->dump();
	fmt::print("}}");
}

void ForWithConditionExpression::dump() const {
	fmt::print("{{\"node\":\"ForWithConditionExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::println("\",condition\":");
	m_condition->dump();
	fmt::println("\",body\":");
	m_body->dump();
	fmt::print("}}");
}

void ForWithRangeExpression::dump() const {
	fmt::print("{{\"node\":\"ForWithRangeExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::println("\",range_variable\":");
	m_range_variable->dump();
	fmt::println("\",range_expression\":");
	m_range_expression->dump();
	fmt::print("}}");
}

void FunctionCallExpression::dump() const {
	fmt::print("{{\"node\":\"FunctionCallExpression\",\"span\":[{},{}],", span().start, span().end);
	fmt::println("\"name\":");
	m_name->dump();
	fmt::println(",\"arguments\":[");
	for (std::size_t i = 0; i < m_arguments.size(); ++i) {
		auto const& argument = m_arguments[i];

		fmt::print("{{");
		if (argument.name) {
			fmt::print("\"name\":");
			(*argument.name)->dump();
			fmt::print(",");
		}
		fmt::print("\"value\":");
		argument.value->dump();

		if (i != m_arguments.size() - 1) {
			fmt::print(",");
		}
	}
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
