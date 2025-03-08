#include "Error.hpp"
#include "Parser.hpp"
#include "utils/Result.hpp"

#include <fmt/core.h>

Result<void, bo::Error> my_main() {
	// FIXME: For now I have to specify stupid types because keywords are not recognized as identifiers
	auto source = "/* simple add function */ fn add(anon a: asdf, anon b: asdf): asdf {\n\ta + b;\n\tc\n}"sv;
	auto parser = TRY(bo::Parser::create(source));
	auto program = TRY(parser.parse_program());
	program->dump(0);
	return {};
}

int main() {
	auto result = my_main();
	if (result.is_error()) {
		// FIXME: Use custom formatter for Error
		auto error = result.release_error();
		fmt::println("Error: {}", error.message());
		fmt::println("Span: {}", error.span());
		return 1;
	}

	return 0;
}
