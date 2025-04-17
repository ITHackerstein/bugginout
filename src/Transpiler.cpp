#include "Transpiler.hpp"

namespace bo {

Result<std::string, Error> Transpiler::transpile() {
	add_prelude();
	for (auto function : m_program.functions()) {
		if (!function->is_builtin()) {
			TRY(transpile_function(function));
		}
	}

	return m_code.str();
}

void Transpiler::add_new_line() {
	m_code << "\n";
	for (int i = 0; i < m_indent_level; ++i) {
		m_code << "    ";
	}
}

void Transpiler::add_prelude() {
	// FIXME: Add suffixes, arguments array to main
	m_code << R"(#include <cstdint>
#include <array>
#include <span>
#include <print>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using isize = std::int64_t;

u8 operator""_u8(unsigned long long value) { return static_cast<u8>(value); }
u16 operator""_u16(unsigned long long value) { return static_cast<u16>(value); }
u32 operator""_u32(unsigned long long value) { return static_cast<u32>(value); }
u64 operator""_u64(unsigned long long value) { return static_cast<u64>(value); }
usize operator""_usize(unsigned long long value) { return static_cast<usize>(value); }
i8 operator""_i8(unsigned long long value) { return static_cast<i8>(value); }
i16 operator""_i16(unsigned long long value) { return static_cast<i16>(value); }
i32 operator""_i32(unsigned long long value) { return static_cast<i32>(value); }
i64 operator""_i64(unsigned long long value) { return static_cast<i64>(value); }
isize operator""_isize(unsigned long long value) { return static_cast<isize>(value); }

template<typename ElementType, bool is_inclusive>
class bo_range {
public:
    struct iterator {
        ElementType value;
        constexpr iterator(ElementType value_):
            value(value_) {}

        constexpr ElementType operator*() { return value; }
        constexpr bool operator==(iterator const& other) { return value == other.value; }
        constexpr bool operator!=(iterator const& other) { return !(*this == other); }
        constexpr void operator++() { ++value; }
    };

    constexpr bo_range(ElementType start, ElementType end):
        m_start(start), m_end(end) {}

    constexpr iterator begin() { return m_start; }
    constexpr iterator end() {
        if constexpr (is_inclusive) {
            return m_end + 1;
        } else {
            return m_end;
        }
    }

private:
    ElementType m_start;
    ElementType m_end;
};

template<typename T>
void print(T value) {
	std::print("{}", value);
}

void bo_main();
int main(int argc, char** argv) {
    (void) argc;
    (void) argv;
    bo_main();
}

)";
}

Result<void, Error> Transpiler::transpile_type(Types::Id id, IgnoreFirstQualifier ignore_first_qualifier) {
	auto const& type = m_program.get_type(id);
	if (type.is<Types::Unknown>()) {
		return Error { "Cannot transpile unknown type", {} };
	}

	if (type.is_builtin()) {
#define BO_ENUMERATE_BUILTIN_TYPE(klass_name, type_name) \
	if (type.is<Types::klass_name>()) {                    \
		m_code << #type_name;                                \
	}
		_BO_ENUMERATE_BUILTIN_TYPES
	} else if (type.is<Types::Pointer>()) {
		// FIXME: Maybe we should add a custom `non-null pointer` type in the C++ code
		auto const& pointer = type.as<Types::Pointer>();
		TRY(transpile_type(pointer.inner_type_id()));
		m_code << "*";
	} else if (type.is<Types::Array>()) {
		auto const& array = type.as<Types::Array>();
		m_code << "std::array<";
		TRY(transpile_type(array.inner_type_id()));
		m_code << ", ";
		m_code << array.size();
		m_code << ">";
	} else if (type.is<Types::Slice>()) {
		auto const& slice = type.as<Types::Slice>();
		m_code << "std::span<";
		TRY(transpile_type(slice.inner_type_id()));
		m_code << ">";
	} else if (type.is<Types::Range>()) {
		auto const& range = type.as<Types::Range>();
		m_code << "bo_range<";
		TRY(transpile_type(range.element_type_id()));
		m_code << ", ";
		m_code << (range.is_inclusive() ? "true" : "false");
		m_code << ">";
	} else {
		assert(false && "Type not handled");
	}

	if (ignore_first_qualifier == IgnoreFirstQualifier::No && !type.is_mutable()) {
		m_code << " const";
	}

	return {};
}

void Transpiler::transpile_binary_operator(AST::BinaryOperator op) {
	switch (op) {
	case AST::BinaryOperator::Addition:
		m_code << "+";
		break;
	case AST::BinaryOperator::Subtraction:
		m_code << "-";
		break;
	case AST::BinaryOperator::Multiplication:
		m_code << "*";
		break;
	case AST::BinaryOperator::Division:
		m_code << "/";
		break;
	case AST::BinaryOperator::Modulo:
		m_code << "%";
		break;
	case AST::BinaryOperator::BitwiseLeftShift:
		m_code << "<<";
		break;
	case AST::BinaryOperator::BitwiseRightShift:
		m_code << ">>";
		break;
	case AST::BinaryOperator::LessThan:
		m_code << "<";
		break;
	case AST::BinaryOperator::GreaterThan:
		m_code << ">";
		break;
	case AST::BinaryOperator::LessThanOrEqualTo:
		m_code << "<=";
		break;
	case AST::BinaryOperator::GreaterThanOrEqualTo:
		m_code << ">=";
		break;
	case AST::BinaryOperator::EqualTo:
		m_code << "==";
		break;
	case AST::BinaryOperator::NotEqualTo:
		m_code << "!=";
		break;
	case AST::BinaryOperator::BitwiseAnd:
		m_code << "&";
		break;
	case AST::BinaryOperator::BitwiseXor:
		m_code << "^";
		break;
	case AST::BinaryOperator::BitwiseOr:
		m_code << "|";
		break;
	case AST::BinaryOperator::LogicalAnd:
		m_code << "&&";
		break;
	case AST::BinaryOperator::LogicalOr:
		m_code << "||";
		break;
	default:
		assert(false && "Binary operator not handled");
	}
}

void Transpiler::transpile_unary_operator(AST::UnaryOperator op) {
	switch (op) {
	case AST::UnaryOperator::Positive:
		m_code << "+";
		break;
	case AST::UnaryOperator::Negative:
		m_code << "-";
		break;
	case AST::UnaryOperator::LogicalNot:
		m_code << "!";
		break;
	case AST::UnaryOperator::BitwiseNot:
		m_code << "~";
		break;
	default:
		assert(false && "Unary operator not handled");
	}
}

void Transpiler::transpile_assignment_operator(AST::AssignmentOperator op) {
	switch (op) {
	case AST::AssignmentOperator::Assignment:
		m_code << "=";
		break;
	case AST::AssignmentOperator::AdditionAssignment:
		m_code << "+=";
		break;
	case AST::AssignmentOperator::SubtractionAssignment:
		m_code << "-=";
		break;
	case AST::AssignmentOperator::MultiplicationAssignment:
		m_code << "*=";
		break;
	case AST::AssignmentOperator::DivisionAssignment:
		m_code << "/=";
		break;
	case AST::AssignmentOperator::ModuloAssignment:
		m_code << "%=";
		break;
	case AST::AssignmentOperator::BitwiseLeftShiftAssignment:
		m_code << "<<=";
		break;
	case AST::AssignmentOperator::BitwiseRightShiftAssignment:
		m_code << ">>=";
		break;
	case AST::AssignmentOperator::BitwiseAndAssignment:
		m_code << "&=";
		break;
	case AST::AssignmentOperator::BitwiseXorAssignment:
		m_code << "^=";
		break;
	case AST::AssignmentOperator::BitwiseOrAssignment:
		m_code << "|=";
		break;
	case AST::AssignmentOperator::LogicalAndAssignment:
	case AST::AssignmentOperator::LogicalOrAssignment:
		assert(false && "Logical assignment operator can't be translated");
	default:
		assert(false && "Assignment operator not handled");
	}
}

void Transpiler::transpile_update_operator(AST::UpdateOperator op) {
	switch (op) {
	case AST::UpdateOperator::Increment:
		m_code << "++";
		break;
	case AST::UpdateOperator::Decrement:
		m_code << "--";
		break;
	default:
		assert(false && "Update operator not handled");
	}
}

Result<void, Error> Transpiler::transpile_statement(std::shared_ptr<CheckedAST::Statement const> statement) {
	if (statement->is_expression_statement()) {
		auto expression_statement = std::static_pointer_cast<CheckedAST::ExpressionStatement const>(statement);
		TRY(transpile_expression(expression_statement->expression()));
		m_code << ";";
		return {};
	}

	if (statement->is_variable_declaration()) {
		auto variable_declaration_statement = std::static_pointer_cast<CheckedAST::VariableDeclarationStatement const>(statement);
		TRY(transpile_variable_declaration_statement(variable_declaration_statement));
		return {};
	}

	if (statement->is_for_statement()) {
		auto for_statement = std::static_pointer_cast<CheckedAST::ForStatement const>(statement);
		TRY(transpile_for_statement(for_statement));
		return {};
	}

	if (statement->is_return_statement()) {
		auto return_statement = std::static_pointer_cast<CheckedAST::ReturnStatement const>(statement);
		TRY(transpile_return_statement(return_statement));
		return {};
	}

	assert(false && "Statement not handled");
}

Result<void, Error> Transpiler::transpile_variable_declaration_statement(std::shared_ptr<CheckedAST::VariableDeclarationStatement const> variable_declaration_statement) {
	auto const& variable = m_program.get_variable(variable_declaration_statement->variable_id());
	TRY(transpile_type(variable.type_id));
	m_code << " ";
	m_code << variable.name;
	if (variable_declaration_statement->initializer()) {
		m_code << " = ";
		TRY(transpile_expression(variable_declaration_statement->initializer()));
	}
	m_code << ";";
	return {};
}

Result<void, Error> Transpiler::transpile_function(std::shared_ptr<CheckedAST::Function const> function) {
	if (function->name() == "main") {
		if (!m_program.get_type(function->return_type_id()).is<Types::Void>() && !function->parameters().empty()) {
			return Error { "Main function must have no parameters and return void", {} };
		}

		m_code << "void bo_main()";
		add_new_line();
		TRY(transpile_block_expression(function->body(), LastBlockStatementTreatment::AsReturnStatement));
		add_new_line();
		return {};
	}

	TRY(transpile_type(function->return_type_id(), IgnoreFirstQualifier::Yes));
	m_code << " ";
	m_code << function->name();
	m_code << "(";
	for (std::size_t i = 0; i < function->parameters().size(); ++i) {
		auto const& parameter = function->parameters()[i];
		TRY(transpile_type(parameter.variable.type_id));
		m_code << " ";
		m_code << parameter.variable.name;

		if (i < function->parameters().size() - 1) {
			m_code << ", ";
		}
	}
	m_code << ")";
	add_new_line();
	TRY(transpile_block_expression(function->body(), LastBlockStatementTreatment::AsReturnStatement));
	add_new_line();
	return {};
}

Result<void, Error> Transpiler::transpile_for_statement(std::shared_ptr<CheckedAST::ForStatement const> for_statement) {
	if (for_statement->is_infinite()) {
		auto infinite_for_statement = std::static_pointer_cast<CheckedAST::InfiniteForStatement const>(for_statement);
		m_code << "for (;;)";
		add_new_line();
		TRY(transpile_block_expression(infinite_for_statement->body(), LastBlockStatementTreatment::Ignore));
	} else if (for_statement->is_with_condition()) {
		auto for_with_condition_statement = std::static_pointer_cast<CheckedAST::ForWithConditionStatement const>(for_statement);
		m_code << "for (;";
		TRY(transpile_expression(for_with_condition_statement->condition()));
		m_code << ";)";
		add_new_line();
		TRY(transpile_block_expression(for_with_condition_statement->body(), LastBlockStatementTreatment::Ignore));
	} else if (for_statement->is_with_range()) {
		auto for_with_range_statement = std::static_pointer_cast<CheckedAST::ForWithRangeStatement const>(for_statement);
		auto const& range_variable = m_program.get_variable(for_with_range_statement->range_variable_id());

		m_code << "for (";
		TRY(transpile_type(range_variable.type_id));
		m_code << " ";
		m_code << range_variable.name;
		m_code << " : ";
		TRY(transpile_expression(for_with_range_statement->range_expression()));
		m_code << ")";
		add_new_line();
		TRY(transpile_block_expression(for_with_range_statement->body(), LastBlockStatementTreatment::Ignore));
	} else {
		assert(false && "Unhandled for statement");
	}

	return {};
}

Result<void, Error> Transpiler::transpile_return_statement(std::shared_ptr<CheckedAST::ReturnStatement const> return_statement) {
	auto const& return_value = return_statement->expression();
	auto const& return_value_type = m_program.get_type(return_value->type_id());

	if (return_value_type.is<Types::Void>()) {
		m_code << "return;";
	} else {
		m_code << "return ";
		TRY(transpile_expression(return_value));
		m_code << ";";
	}

	return {};
}

Result<void, Error> Transpiler::transpile_expression(std::shared_ptr<CheckedAST::Expression const> expression) {
	if (expression->is_parenthesized_expression()) {
		auto parenthesized_expression = std::static_pointer_cast<CheckedAST::ParenthesizedExpression const>(expression);
		m_code << "(";
		TRY(transpile_expression(parenthesized_expression->expression()));
		m_code << ")";
	} else if (expression->is_integer_literal()) {
		TRY(transpile_integer_literal(std::static_pointer_cast<CheckedAST::IntegerLiteral const>(expression)));
	} else if (expression->is_identifier()) {
		TRY(transpile_identifier(std::static_pointer_cast<CheckedAST::Identifier const>(expression)));
	} else if (expression->is_binary_expression()) {
		TRY(transpile_binary_expression(std::static_pointer_cast<CheckedAST::BinaryExpression const>(expression)));
	} else if (expression->is_unary_expression()) {
		TRY(transpile_unary_expression(std::static_pointer_cast<CheckedAST::UnaryExpression const>(expression)));
	} else if (expression->is_assignment_expression()) {
		TRY(transpile_assignment_expression(std::static_pointer_cast<CheckedAST::AssignmentExpression const>(expression)));
	} else if (expression->is_update_expression()) {
		TRY(transpile_update_expression(std::static_pointer_cast<CheckedAST::UpdateExpression const>(expression)));
	} else if (expression->is_pointer_dereference_expression()) {
		TRY(transpile_pointer_dereference_expression(std::static_pointer_cast<CheckedAST::PointerDereferenceExpression const>(expression)));
	} else if (expression->is_address_of_expression()) {
		TRY(transpile_address_of_expression(std::static_pointer_cast<CheckedAST::AddressOfExpression const>(expression)));
	} else if (expression->is_range_expression()) {
		TRY(transpile_range_expression(std::static_pointer_cast<CheckedAST::RangeExpression const>(expression)));
	} else if (expression->is_block_expression()) {
		TRY(transpile_block_expression(std::static_pointer_cast<CheckedAST::BlockExpression const>(expression)));
	} else if (expression->is_if_expression()) {
		TRY(transpile_if_expression(std::static_pointer_cast<CheckedAST::IfExpression const>(expression)));
	} else if (expression->is_function_call_expression()) {
		TRY(transpile_function_call_expression(std::static_pointer_cast<CheckedAST::FunctionCallExpression const>(expression)));
	} else if (expression->is_array_expression()) {
		TRY(transpile_array_expression(std::static_pointer_cast<CheckedAST::ArrayExpression const>(expression)));
	} else if (expression->is_array_subscription_expression()) {
		TRY(transpile_array_subscript_expression(std::static_pointer_cast<CheckedAST::ArraySubscriptExpression const>(expression)));
	} else {
		assert(false && "Expression not handled!");
	}

	return {};
}

Result<void, Error> Transpiler::transpile_integer_literal(std::shared_ptr<CheckedAST::IntegerLiteral const> integer_literal) {
	if (!integer_literal->suffix().empty()) {
		m_code << integer_literal->value() << "_" << integer_literal->suffix();
	} else {
		m_code << "static_cast<";
		TRY(transpile_type(integer_literal->type_id(), IgnoreFirstQualifier::Yes));
		m_code << ">(";
		m_code << integer_literal->value();
		m_code << ")";
	}

	return {};
}

Result<void, Error> Transpiler::transpile_identifier(std::shared_ptr<CheckedAST::Identifier const> identifier) {
	auto const& variable = m_program.get_variable(identifier->variable_id());
	m_code << variable.name;
	return {};
}

Result<void, Error> Transpiler::transpile_binary_expression(std::shared_ptr<CheckedAST::BinaryExpression const> binary_expression) {
	m_code << "static_cast<";
	TRY(transpile_type(binary_expression->type_id(), IgnoreFirstQualifier::Yes));
	m_code << ">(";

	m_code << "(";
	TRY(transpile_expression(binary_expression->lhs()));
	m_code << ")";

	transpile_binary_operator(binary_expression->op());

	m_code << "(";
	TRY(transpile_expression(binary_expression->rhs()));
	m_code << ")";

	m_code << ")";
	return {};
}

Result<void, Error> Transpiler::transpile_unary_expression(std::shared_ptr<CheckedAST::UnaryExpression const> unary_expression) {
	m_code << "static_cast<";
	TRY(transpile_type(unary_expression->type_id(), IgnoreFirstQualifier::Yes));
	m_code << ">(";

	transpile_unary_operator(unary_expression->op());
	m_code << "(";
	TRY(transpile_expression(unary_expression->operand()));
	m_code << ")";

	m_code << ")";

	return {};
}

Result<void, Error> Transpiler::transpile_assignment_expression(std::shared_ptr<CheckedAST::AssignmentExpression const> assignment_expression) {
	m_code << "static_cast<";
	TRY(transpile_type(assignment_expression->type_id(), IgnoreFirstQualifier::Yes));
	m_code << ">(";

	m_code << "(";
	TRY(transpile_expression(assignment_expression->lhs()));
	m_code << ")";

	if (assignment_expression->op() == AST::AssignmentOperator::LogicalAndAssignment) {
		m_code << " = ";
		m_code << "(";
		TRY(transpile_expression(assignment_expression->lhs()));
		m_code << ")";
		transpile_binary_operator(AST::BinaryOperator::LogicalAnd);
	} else if (assignment_expression->op() == AST::AssignmentOperator::LogicalOrAssignment) {
		m_code << " = ";
		m_code << "(";
		TRY(transpile_expression(assignment_expression->lhs()));
		m_code << ")";
		transpile_binary_operator(AST::BinaryOperator::LogicalOr);
	} else {
		transpile_assignment_operator(assignment_expression->op());
	}

	m_code << "(";
	TRY(transpile_expression(assignment_expression->rhs()));
	m_code << ")";

	m_code << ")";

	return {};
}

Result<void, Error> Transpiler::transpile_update_expression(std::shared_ptr<CheckedAST::UpdateExpression const> update_expression) {
	m_code << "static_cast<";
	TRY(transpile_type(update_expression->type_id(), IgnoreFirstQualifier::Yes));
	m_code << ">(";

	if (update_expression->is_prefixed()) {
		transpile_update_operator(update_expression->op());
		m_code << "(";
		TRY(transpile_expression(update_expression->operand()));
		m_code << ")";
	} else {
		m_code << "(";
		TRY(transpile_expression(update_expression->operand()));
		m_code << ")";
		transpile_update_operator(update_expression->op());
	}

	m_code << ")";

	return {};
}

Result<void, Error> Transpiler::transpile_pointer_dereference_expression(std::shared_ptr<CheckedAST::PointerDereferenceExpression const> pointer_dereference_expression) {
	m_code << "*";
	m_code << ")";
	TRY(transpile_expression(pointer_dereference_expression->operand()));
	m_code << ")";
	return {};
}

Result<void, Error> Transpiler::transpile_address_of_expression(std::shared_ptr<CheckedAST::AddressOfExpression const> address_of_expression) {
	m_code << "&";
	m_code << "(";
	TRY(transpile_expression(address_of_expression->operand()));
	m_code << ")";
	return {};
}

Result<void, Error> Transpiler::transpile_range_expression(std::shared_ptr<CheckedAST::RangeExpression const> range_expression) {
	TRY(transpile_type(range_expression->type_id()));
	m_code << "(";
	TRY(transpile_expression(range_expression->start()));
	m_code << ", ";
	TRY(transpile_expression(range_expression->end()));
	m_code << ")";
	return {};
}

Result<void, Error> Transpiler::transpile_block_expression(std::shared_ptr<CheckedAST::BlockExpression const> block_expression, LastBlockStatementTreatment last_statement_treatment) {
	if (m_program.get_type(block_expression->type_id()).is<Types::Void>() || last_statement_treatment == LastBlockStatementTreatment::Ignore) {
		m_code << "{";
		++m_indent_level;
		add_new_line();

		for (std::size_t i = 0; i < block_expression->statements().size(); ++i) {
			TRY(transpile_statement(block_expression->statements()[i]));
			if (i != block_expression->statements().size() - 1) {
				add_new_line();
			}
		}

		--m_indent_level;
		add_new_line();
		m_code << "}";
	} else if (last_statement_treatment == LastBlockStatementTreatment::AsExpression) {
		m_code << "({";
		++m_indent_level;
		add_new_line();

		for (std::size_t i = 0; i < block_expression->statements().size(); ++i) {
			TRY(transpile_statement(block_expression->statements()[i]));
			if (i != block_expression->statements().size() - 1) {
				add_new_line();
			}
		}

		--m_indent_level;
		add_new_line();
		m_code << "})";
	} else if (last_statement_treatment == LastBlockStatementTreatment::AsReturnStatement) {
		m_code << "{";
		++m_indent_level;
		add_new_line();

		for (std::size_t i = 0; i < block_expression->statements().size() - 1; ++i) {
			TRY(transpile_statement(block_expression->statements()[i]));
			add_new_line();
		}

		m_code << "return ";
		TRY(transpile_statement(block_expression->statements().back()));

		--m_indent_level;
		add_new_line();
		m_code << "}";
	} else if (last_statement_treatment == LastBlockStatementTreatment::StoreInVariable) {
		m_code << "{";
		++m_indent_level;
		add_new_line();

		for (std::size_t i = 0; i < block_expression->statements().size() - 1; ++i) {
			TRY(transpile_statement(block_expression->statements()[i]));
			add_new_line();
		}

		m_code << "__block_ret_" << m_temp_variable_iota << " = ";
		TRY(transpile_statement(block_expression->statements().back()));

		--m_indent_level;
		add_new_line();
		m_code << "}";
	} else {
		assert(false && "Unhandled last block statement treatment");
	}

	return {};
}

Result<void, Error> Transpiler::transpile_if_expression(std::shared_ptr<CheckedAST::IfExpression const> if_expression) {
	if (m_program.get_type(if_expression->type_id()).is<Types::Void>()) {
		m_code << "if (";
		TRY(transpile_expression(if_expression->condition()));
		m_code << ")";

		add_new_line();
		TRY(transpile_block_expression(if_expression->then(), LastBlockStatementTreatment::Ignore));
		add_new_line();

		if (if_expression->else_()) {
			auto else_ = if_expression->else_();

			m_code << "else";
			add_new_line();
			if (else_->is_block_expression()) {
				TRY(transpile_block_expression(std::static_pointer_cast<CheckedAST::BlockExpression const>(else_), LastBlockStatementTreatment::Ignore));
			} else {
				TRY(transpile_expression(else_));
			}
		}
	} else {
		assert(if_expression->else_() && "If expression must have an else branch if it has a return type");
		++m_temp_variable_iota;

		m_code << "({";
		++m_indent_level;
		add_new_line();

		TRY(transpile_type(if_expression->type_id()));
		m_code << " __block_ret_" << m_temp_variable_iota << " {};";
		add_new_line();

		m_code << "if (";
		TRY(transpile_expression(if_expression->condition()));
		m_code << ")";

		add_new_line();
		TRY(transpile_block_expression(if_expression->then(), LastBlockStatementTreatment::StoreInVariable));
		add_new_line();

		m_code << "else";
		add_new_line();

		m_code << "{";
		++m_indent_level;
		add_new_line();

		m_code << "__block_ret_" << m_temp_variable_iota << " = ";
		TRY(transpile_expression(if_expression->else_()));
		m_code << ";";

		--m_indent_level;
		add_new_line();
		m_code << "}";

		add_new_line();
		m_code << "__block_ret_" << m_temp_variable_iota << ";";

		--m_indent_level;
		add_new_line();
		m_code << "})";

		--m_temp_variable_iota;
	}

	return {};
}

Result<void, Error> Transpiler::transpile_function_call_expression(std::shared_ptr<CheckedAST::FunctionCallExpression const> function_call_expression) {
	auto const& function = m_program.get_function(function_call_expression->function_id());
	m_code << function->name();
	m_code << "(";
	for (std::size_t i = 0; i < function_call_expression->arguments().size(); ++i) {
		auto const& argument = function_call_expression->arguments()[i];
		TRY(transpile_expression(argument.value));
		if (i != function_call_expression->arguments().size() - 1) {
			m_code << ", ";
		}
	}
	m_code << ")";
	return {};
}

Result<void, Error> Transpiler::transpile_array_expression(std::shared_ptr<CheckedAST::ArrayExpression const> array_expression) {
	m_code << "(";
	TRY(transpile_type(array_expression->type_id(), IgnoreFirstQualifier::Yes));
	m_code << "{";
	for (std::size_t i = 0; i < array_expression->elements().size(); ++i) {
		TRY(transpile_expression(array_expression->elements()[i]));
		if (i != array_expression->elements().size() - 1) {
			m_code << ", ";
		}
	}
	m_code << "})";
	return {};
}

Result<void, Error> Transpiler::transpile_array_subscript_expression(std::shared_ptr<CheckedAST::ArraySubscriptExpression const> array_subscript_expression) {
	m_code << "(";
	TRY(transpile_expression(array_subscript_expression->array()));
	m_code << ")[";
	TRY(transpile_expression(array_subscript_expression->index()));
	m_code << "]";
	return {};
}

}
