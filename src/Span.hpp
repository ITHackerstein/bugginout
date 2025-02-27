#pragma once

#include <cstddef>
#include <string>

namespace bo {

struct Span {
	size_t start;
	size_t end;
};

auto format_as(Span) -> std::string;

}
