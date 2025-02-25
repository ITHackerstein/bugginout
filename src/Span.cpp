#include "Span.hpp"

#include <fmt/core.h>

namespace boc {

auto format_as(Span span) -> std::string {
	return fmt::format("Span {{ start: {}, end: {} }}", span.start, span.end);
}

}
