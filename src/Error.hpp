#pragma once

#include <string>

#include "Span.hpp"

namespace bo {

class Error {
public:
	explicit Error(std::string&& message, Span span)
	  : m_message(std::move(message)), m_span(span) {}

	std::string_view message() const { return m_message; }
	Span span() const { return m_span; }

private:
	std::string m_message;
	Span m_span;
};

}
