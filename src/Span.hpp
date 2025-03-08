#pragma once

#include <cstddef>
#include <string>

namespace bo {

struct Span {
	size_t start;
	size_t end;

	static Span merge(Span a, Span b) {
		if (a.start > b.start) {
			std::swap(a, b);
		}

		return Span { a.start, b.end };
	}
};

auto format_as(Span) -> std::string;

}
