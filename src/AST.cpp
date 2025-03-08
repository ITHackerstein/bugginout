#include "AST.hpp"
#include "fmt/base.h"

#include <fmt/core.h>

namespace bo {

namespace AST {

static void print_indent(unsigned indent) {
	for (unsigned i = 0; i < indent; ++i) {
		fmt::print("  ");
	}
}

void ExpressionStatement::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("ExpressionStatement");
	m_expression->dump(indent + 1);
}

void IntegerLiteral::dump(unsigned indent) const {
	print_indent(indent);
	fmt::print("IntegerLiteral {{ {:?} }}\n", m_value);
}

void Identifier::dump(unsigned indent) const {
	print_indent(indent);
	fmt::print("Identifier {{ {:?} }}\n", m_id);
}

void BinaryExpression::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("BinaryExpression");
	print_indent(indent + 1);
	fmt::print("Operator: ");
	switch (m_op) {
#define BO_ENUMERATE_BINARY_OPERATOR(x) \
	case BinaryOperator::x:               \
		fmt::println(#x);                   \
		break;
		_BO_ENUMERATE_BINARY_OPERATORS
#undef BO_ENUMERATE_BINARY_OPERATOR
	}
	print_indent(indent + 1);
	fmt::println("LHS:");
	m_lhs->dump(indent + 2);

	print_indent(indent + 1);
	fmt::println("RHS:");
	m_rhs->dump(indent + 2);
}

void VariableDeclarationStatement::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("VariableDeclarationStatement");
	m_identifier->dump(indent + 1);
	m_expression->dump(indent + 1);
}

void BlockExpression::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("BlockExpression");
	for (auto const& statement : m_statements) {
		statement->dump(indent + 1);
	}

	std::visit([indent](auto const& value) { value->dump(indent + 1); }, m_last_expression_or_statement);
}

void FunctionDeclarationStatement::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("FunctionDeclarationStatement");

	print_indent(indent + 1);
	fmt::println("Name");
	m_name->dump(indent + 2);

	print_indent(indent + 1);
	fmt::println("Parameters");
	for (auto const& parameter : m_parameters) {
		print_indent(indent + 2);
		fmt::println("Anonymous: {}", parameter.is_anonymous);

		print_indent(indent + 2);
		fmt::println("Name");
		parameter.type->dump(indent + 3);

		print_indent(indent + 2);
		fmt::println("Name");
		parameter.name->dump(indent + 3);

		fmt::println("");
	}

	print_indent(indent + 1);
	fmt::println("Body");
	m_body->dump(indent + 2);
}

void IfExpression::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("IfExpression");

	print_indent(indent + 1);
	fmt::println("Condition");
	m_condition->dump(indent + 2);

	print_indent(indent + 1);
	fmt::println("Then");
	m_then_block->dump(indent + 2);

	if (m_else_block) {
		print_indent(indent + 1);
		fmt::println("Else");
		(*m_else_block)->dump(indent + 2);
	}
}

void InfiniteForExpression::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("InfiniteForExpression");

	print_indent(indent + 1);
	fmt::println("Body");
	m_body->dump(indent + 2);
}

void ForWithConditionExpression::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("ForWithConditionExpression");

	print_indent(indent + 1);
	fmt::println("Condition");
	m_condition->dump(indent + 2);

	print_indent(indent + 1);
	fmt::println("Body");
	m_body->dump(indent + 2);
}

void ForWithRangeExpression::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("ForWithRangeExpression");

	print_indent(indent + 1);
	fmt::println("Range variable");
	m_range_variable->dump(indent + 2);

	print_indent(indent + 1);
	fmt::println("Range expression");
	m_range_expression->dump(indent + 2);

	print_indent(indent + 1);
	fmt::println("Body");
	m_body->dump(indent + 2);
}

void FunctionCallExpression::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("FunctionCallExpression");

	print_indent(indent + 1);
	fmt::println("Name");
	m_name->dump(indent + 2);

	print_indent(indent + 1);
	fmt::println("Arguments");
	for (auto const& argument : m_arguments) {
		if (argument.name) {
			print_indent(indent + 2);
			fmt::println("Name");
			(*argument.name)->dump(indent + 3);
		}

		print_indent(indent + 2);
		fmt::println("Value");
		argument.value->dump(indent + 3);
	}

	m_name->dump(indent + 2);
}

void Program::dump(unsigned indent) const {
	print_indent(indent);
	fmt::println("Program");

	for (auto const& function : m_functions) {
		function->dump(indent + 1);
	}
}

}

}
