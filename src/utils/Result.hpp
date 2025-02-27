#pragma once

#include <cassert>
#include <optional>
#include <variant>

// NOTE: Marking these classes as [[nodiscard]] so that whenever we use them we
//       are warned if we don't call them with TRY or MUST.

template<typename ValueType, typename ErrorType>
class [[nodiscard]] Result {
public:
	Result(ValueType&& value)
	  : m_impl(std::forward<ValueType>(value)) {}

	Result(ErrorType&& error)
	  : m_impl(std::forward<ErrorType>(error)) {}

	Result(Result const&) = default;
	Result(Result&&) = default;
	~Result() = default;

	bool is_value() const { return std::holds_alternative<ValueType>(m_impl); }
	bool is_error() const { return std::holds_alternative<ErrorType>(m_impl); }

	ValueType& value() {
		assert(is_value());
		return std::get<ValueType>(m_impl);
	}

	ValueType&& release_value() {
		assert(is_value());
		return std::get<ValueType>(std::move(m_impl));
	}

	ErrorType& error() {
		assert(is_error());
		return std::get<ErrorType>(m_impl);
	}

	ErrorType&& release_error() {
		assert(is_error());
		return std::get<ErrorType>(std::move(m_impl));
	}

private:
	std::variant<ValueType, ErrorType> m_impl;
};

template<typename ErrorType>
class Result<void, ErrorType> {
public:
	Result() = default;

	Result(ErrorType&& error)
	  : m_impl(std::forward<ErrorType>(error)) {}

	Result(Result const&) = default;
	Result(Result&&) = default;
	~Result() = default;

	bool is_value() const { return !is_error(); }
	bool is_error() const { return m_impl.has_value(); }

	void value() { assert(is_value()); }
	void release_value() { assert(is_value()); }

	ErrorType& error() {
		assert(is_error());
		return *m_impl;
	}

	ErrorType&& release_error() {
		assert(is_error());
		return std::move(*m_impl);
	}

private:
	std::optional<ErrorType> m_impl;
};

#define TRY(expr)                  \
	({                               \
		auto _tmp = (expr);            \
		if (_tmp.is_error())           \
			return _tmp.release_error(); \
		_tmp.release_value();          \
	})

#define MUST(expr)           \
	({                         \
		auto _tmp = (expr);      \
		assert(_tmp.is_value()); \
		_tmp.release_value();    \
	})
