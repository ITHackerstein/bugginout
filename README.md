# Buggin Out

## Types

```
TYPE:
	IS_MUTABLE? IS_POINTER? INNER_TYPE

IS_MUTABLE: "mut"
IS_POINTER: "^" | "*"
INNER_TYPE: TYPE | IDENTIFIER
```

### Integer types

| Type  | Description             |
|-------|-------------------------|
| `u8`  | Unsigned 8-bit integer  |
| `i8`  | Signed 8-bit integer    |
| `u16` | Unsigned 16-bit integer |
| `i16` | Signed 16-bit integer   |
| `u32` | Unsigned 32-bit integer |
| `i32` | Signed 32-bit integer   |
| `u64` | Unsigned 32-bit integer |
| `i64` | Signed 32-bit integer   |

In addition to these, there are two machine-dependent integer: `usize` and `isize` which will have the same size as the platform's pointer type (usually 64 bits).

### Textual types

For now the only textual type is `char` which represent an ASCII character, in the future it will be ported to Unicode and a type for strings will be added.

### Boolean type

The language supports the `bool` type which is a boolean type with only two values `true` and `false`

### Weak and strong pointer types

Pointers in this language can be weak, meaning they can be _null_ (point to nothing), or strong, meaning they can'be null.

Weak pointers are denoted with a `*` (example: `int*`) and strong pointers are denoted with a `^` (example: `int^`).

### Other types

> **TODO**: arrays, user-defined types, standard-types

## Identifiers

An identifier will have the following grammar:
```
IDENTIFIER:
  IDENTIFIER_START IDENTIFIER_MIDDLE*

IDENTIFIER_START: ["A"-"Z", "a"-"z", "_", "$"]

IDENTIFIER_MIDDLE: IDENTIFIER_START | ["0"-"9"]
```

## Comments

There are two types of comments:
* `// single line comment`
* `/* multi line comment */`

## Expressions

An expression produces a value, may have side effects and may contain other sub-expressions.

### Literal expressions

```
LITERAL_EXPRESSION: INTEGER_LITERAL | CHAR_LITERAL | BOOLEAN_LITERAL | NULL

INTEGER_LITERAL:
  (DECIMAL_LITERAL | BINARY_LITERAL | OCTAL_LITERAL | HEXADECIMAL_LITERAL) SUFFIX?

DECIMAL_LITERAL:
  DECIMAL_DIGIT (DECIMAL_DIGIT | "'")*

BINARY_LITERAL:
  "0b" BINARY_DIGIT (BINARY_DIGIT | "'")*

OCTAL_LITERAL:
  "0o" OCTAL_DIGIT (OCTAL_DIGIT | "'")*

HEXADECIMAL_LITERAL:
  "0x" HEXADECIMAL_DIGIT (HEXADECIMAL_DIGIT | "'")*

CHAR_LITERAL:
  "'" (!["'", "\", NEW_LINE, CARRIAGE_RETURN, HORIZONTAL_TAB] | QUOTE_ESCAPE | ASCII_ESCAPE) "'"

QUOTE_ESCAPE: "\'"

ASCII_ESCAPE:
  "\x" OCTAL_DIGIT HEXADECIMAL_DIGIT |
  "\n" | "\r" | "\t" | "\\" | "\0"

BOOLEAN_LITERAL: "true" | "false"

DECIMAL_DIGIT: ["0"-"9"]
BINARY_DIGIT: "0" | "1"
OCTAL_DIGIT: ["0"-"7"]
HEXADECIMAL_DIGIT: ["0"-"9", "a"-"f", "A"-"F"]

SUFFIX: "_" ("u8" | "i8" | "u16" | "i16" | "u32" | "i32" | "u64" | "i64" | "usize" | "isize")
```

### Operators

The following operators can be used in expressions (listed in order of precedence):

| Operator                                                                                                                            | Associativity | Description                         |
|-------------------------------------------------------------------------------------------------------------------------------------|---------------|-------------------------------------|
| `a++`,`a--`                                                                                                                         | Left          | Postfix increment/decrement         |
| `++a`,`--a`                                                                                                                         | Right         | Prefix increment/decrement          |
| `+a`,`-a`                                                                                                                           | Right         | Unary plus/minus                    |
| `!a`                                                                                                                                | Right         | Logical NOT                         |
| `~a`                                                                                                                                | Right         | Bitwise NOT                         |
| `@a`                                                                                                                                | Right         | Pointer dereference                 |
| `&a`                                                                                                                                | Right         | Address of                          |
| `a * b`,`a / b`,`a % b`                                                                                                             | Left          | Multiplication, division and modulo |
| `a + b`,`a - b`                                                                                                                     | Left          | Addition, subtraction               |
| `a << b`,`a >> b`                                                                                                                   | Left          | Bitwise left/right shift            |
| `a < b`,`a > b`,`a <= b`, `a >= b`                                                                                                  | Left          | Relational operators                |
| `a == b`,`a != b`                                                                                                                   | Left          | Equality operators                  |
| `a & b`                                                                                                                             | Left          | Bitwise AND                         |
| `a ^ b`                                                                                                                             | Left          | Bitwise XOR                         |
| `a \| b`                                                                                                                            | Left          | Bitwise OR                          |
| `a && b`                                                                                                                            | Left          | Logical AND                         |
| `a \|\| b`                                                                                                                          | Left          | Logical OR                          |
| `a ..< b`,`a ..= b`                                                                                                                 | Right         | Range expression                    |
| `a = b`,`a += b`,`a -= b`,`a *= b`,`a /= b`,`a %= b`,`a <<= b`,`a >>= b`,`a &= b`,`a ^= b`,`a \|= b`,`a &&= b`,`a \|\|= b`          | Right         | Assignment/update                   |

The order of evaluation of an expression can be changed using parenthesis `()`.

### Block expressions

```
BLOCK_EXPRESSION:
	"{" STATEMENTS? "}"

STATEMENTS:
	STATEMENTS_END |
	STATEMENTS STATEMENTS_END

STATEMENTS_END:
	STATEMENT | EXPRESSION_WITHOUT_BLOCK
```

### Call expressions

> **TODO**: We could have something more generic than a IDENTIFIER for the callee

```
CALL_EXPRESSION:
  IDENTIFIER "(" CALL_PARAMETERS? ")"

CALL_PARAMETERS:
  CALL_PARAMETER ("," CALL_PARAMETER)*

CALL_PARAMETER: ANONYMOUS_CALL_PARAMETER | NAMED_CALL_PARAMETER
ANONYMOUS_CALL_PARAMETER: EXPRESSION
NAMED_CALL_PARAMETER: IDENTIFIER ":" EXPRESSION
```

### If expressions

```
IF_EXPRESSION:
	"if" "(" EXPRESSION ")" BLOCK_EXPRESSION ("else" (IF_EXPRESSION | BLOCK_EXPRESSION))?
```

### Range expressions

```
RANGE_EXPRESSION:
	EXPRESSION ("..<" | "..=") EXPRESSION
```

## Statements

A statement evaluates an expression and discards its value. They are ended with `;`.

### Variable declaration statement

```
VARIABLE_DECLARATION_STATEMENT:
  (
    "var" IDENTIFIER ":" TYPE |
    "var" IDENTIFIER "=" EXPRESSION |
    "var" IDENTIFIER ":" TYPE "=" EXPRESSION |
  ) ";"
```

Variables are declared with the following syntax:
```
var variable_name [: type] [= initial_value];
```
If the type is not specified it will be infered from the initial value. Also, every variable is immutable by default.

### For statements

```
FOR_STATEMENT:
	"for" BLOCK_EXPRESSION |
	"for" "(" EXPRESSION ")" BLOCK_EXPRESSION |
	"for" "(" IDENTIFIER "in" EXPRESSION ")" BLOCK_EXPRESSION
```

## Functions

```
FUNCTION_DECLARATION:
  "fn" IDENTIFIER "(" FUNCTION_PARAMETERS? ")" ":" TYPE BLOCK_EXPRESSION

FUNCTION_PARAMETERS:
  FUNCTION_PARAMETER ("," FUNCTION_PARAMETER)*

FUNCTION_PARAMETER:
  PARAMETER_ANONYMITY? IDENTIFIER ":" TYPE

PARAMETER_ANONYMITY: "anon"
```

Function will be defined with the following syntax:
```
fn function_name(function_parameter_1: a_type, anon b: b_type): return_type {
	// implementation
}
```
and will be called using this one
```
function_name(function_parameter_1: a_value, b_value)
```

> **TODO**: Better phrasing

Every argument needs to be named in the call to avoid amibiguity, `anon` argument do not need to be named and, when defining the function, need to be specified at the end of the function.

## Memory management

The memory management in this language will be manual, meaning you have to keep track of the memory you allocate and free it after using it.

> **TODO**: memory management functions
