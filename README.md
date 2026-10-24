# C++ Lexical Analyzer (DFA-based)

A small, hand-written **lexical analyzer (lexer / tokenizer)** written in C++, implemented as a hand-rolled **deterministic finite automaton (DFA)**. It reads a source file and prints a stream of classified tokens to the console.

Originally written as a compilers-course assignment; this repository contains a cleaned-up version with the bugs found during review fixed, and a few small features added.

## Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [How It Works (DFA design)](#how-it-works-dfa-design)
- [Requirements](#requirements)
- [Build & Run](#build--run)
- [Example](#example)
- [Supported Tokens](#supported-tokens)
- [Remaining Limitations](#remaining-limitations)

## Features

The lexer recognizes:

- Keywords: `int`, `double`, `float`, `String`, `char`, `bool`, `if`, `else`, `switch`, `case`, `break`, `for`, `while`
- Identifiers
- Integer literals (`10`, `0`) and real literals (`3.14`), including scientific notation (`1e10`, `2.5e-3`)
- String literals (`"like this"`, with `\"` escaping)
- `//` line comments and `/* ... */` block comments (stripped, not emitted as tokens)
- Arithmetic operators: `+ - * / %`, plus `++` and `--`
- Comparison / assignment: `= == != < <= > >=`
- Logical operators: `&& ||`; bitwise: `& |`; unary not: `!`
- Punctuation: `: ; { } [ ] ( ) ,`
- Invalid tokens — any run of characters that doesn't match a known pattern is reported instead of being silently dropped or crashing the program

## Project Structure

```
.
├── main.cpp          # Lexer source code
├── README.md          # This file
├── LICENSE            # MIT license
├── .gitignore          # Build artifacts to exclude from the repo
└── examples/
    └── source.txt      # Sample input file
```

## How It Works (DFA design)

Instead of one monolithic DFA, the scanner tries a **chain of independent start states** for each new token: identifier → number → operator → punctuation → invalid. If the current attempt doesn't match, the file position is rewound with `fseek()` and the next state in the chain is tried. The `fail()` function defines that chain:

| Current start state | On mismatch → next start | Target |
|---|---|---|
| 1 | 3 | number |
| 3 | 7 | operator |
| 7 | 8 | punctuation |
| 8 | 9 | invalid token |

States 2, 4, 5, 6, 12 are "continuation" states that keep consuming characters once a token has started (identifier, integer, real, exponent). State 10 is the dispatcher: it skips whitespace, detects the start of string literals and comments, and exits cleanly at `EOF`.

## Requirements

- A C++ compiler with C++17 support (`g++`, `clang++`, or MSVC)
- No external libraries — only the standard library

## Build & Run

```bash
g++ -std=c++17 -Wall -Wextra -O2 -o lexer main.cpp
```

The lexer now takes the input file as a command-line argument (it falls back to `source.txt` in the current directory if none is given):

```bash
./lexer examples/source.txt
# or, with the default filename:
cp examples/source.txt source.txt
./lexer
```

## Example

Input (`examples/source.txt`):

```c
int x = 10;
float y = 3.14;
if (x == 10) {
    y = y + 1;
}
```

Output:

```
KEYWORD: int
IDENTIFIER: x
ASSIGN: =
INTEGER: 10
SIGN: ;
KEYWORD: float
IDENTIFIER: y
ASSIGN: =
REAL: 3.14
SIGN: ;
KEYWORD: if
SIGN: (
IDENTIFIER: x
OPERATOR: ==
INTEGER: 10
SIGN: )
SIGN: {
IDENTIFIER: y
ASSIGN: =
IDENTIFIER: y
OPERATOR: +
INTEGER: 1
SIGN: ;
SIGN: }
```

## Supported Tokens

| Category | Example | Output label |
|---|---|---|
| Keyword | `int`, `if`, `while` | `KEYWORD: ...` |
| Identifier | `x`, `total1` | `IDENTIFIER: ...` |
| Integer | `10`, `0` | `INTEGER: ...` |
| Real | `3.14`, `1e10`, `2.5e-3` | `REAL: ...` |
| String | `"hello"` | `STRING: "..."` |
| Arithmetic operator | `+ - * / %` | `OPERATOR: ...` |
| Increment / decrement | `++ --` | `OPERATOR: ++` / `OPERATOR: --` |
| Equality / relational | `== != < <= > >=` | `OPERATOR: ...` |
| Assignment | `=` | `ASSIGN: =` |
| Logical not | `!` | `OPERATOR: !` |
| Logical AND / OR | `&& / \|\|` | `OPERATOR: &&` / `OPERATOR: \|\|` |
| Bitwise AND / OR | `& / \|` | `BITWISE AND: &` / `BITWISE OR: \|` |
| Punctuation | `; { } ( ) [ ] : ,` | `SIGN: ...` |
| Line / block comment | `// ...`, `/* ... */` | *(silently skipped)* |
| Invalid | `@`, `#` | `INVALID TOKEN: ...` |


## Remaining Limitations

A few things are intentionally out of scope for this simple assignment-level lexer and have not been changed:

- No line/column tracking in the output (useful for a later parser stage, but not implemented here).
- Numbers with a leading `0` followed directly by another digit (e.g. `007`) are tokenized as separate integers (`0`, then `7`), matching the original design's behavior rather than C-style octal literals.
- No support for character literals (`'a'`), hexadecimal/binary literals (`0x1F`, `0b101`), or multi-byte/Unicode identifiers.
