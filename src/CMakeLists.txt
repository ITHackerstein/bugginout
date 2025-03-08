include(FetchContent)

FetchContent_Declare(
	fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt
	GIT_TAG        11.1.3
)
FetchContent_MakeAvailable(fmt)

add_executable(boc
	AST.cpp
	Lexer.cpp
	Parser.cpp
	Span.cpp
	Token.cpp
	boc.cpp
)

target_compile_features(boc PRIVATE cxx_std_23)
target_compile_options(boc PRIVATE -Wall -Wextra -Werror -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wunused -Wformat=2)
target_link_libraries(boc PRIVATE fmt::fmt)
