# BASIC-boost.spirit

BASIC programming language interpreter based on `Boost.Spirit X3`

## About The Project

The project was born as a result of a time-limited engineering challenge of implementing a BASIC interpreter that is good enough for running 2 text-based games:

* [**WUMPUS 2**](https://www.atariarchives.org/morebasicgames/showpage.php?page=181) by Gregory Yob from ["More BASIC Computer Games"](https://www.atariarchives.org/morebasicgames/index.php) by David H. Ahl, published 1979 ([listing](https://www.roug.org/retrocomputing/languages/basic/morebasicgames/wumpus2.bas))
* [**THE ANCIENT CHATEAU**](https://www.atariarchives.org/adventure/chapter21.php) from ["Creating Adventure Games On Your Computer"](https://www.atariarchives.org/adventure/index.php) by Tim Hartnell, published 1983 ([listing](https://www.atariarchives.org/adventure/chapter22.php))

## Features

* Fully functional, but limited to only essential (used in samples) commands.
* Parsing based on `Boost.Spirit X3`. The whole language [grammar](grammar.cpp) takes less than 250 lines.
* Comprehensive unit test suite: [`unit_tests.bas`](programs/unit_tests.bas) (based on the test suite of [Applesoft BASIC in Javascript](https://www.calormen.com/jsbasic/) by Joshua Bell) + separate [`tests.cpp`](tests.cpp)
* Input automation: User input is being logged and can be reused through `.input` files
* Walkthrough `.input` files for both `wumpus2.bas` and `chateau.bas`
* Interactive mode
* Additional programs included

## Executables

Download [Windows executable archive](https://github.com/black-square/BASIC-boost.spirit/releases/latest/download/BASIC-boost.spirit.zip), unarchive, and execute `run.bat` to run most programs at once using the input automation.

## BASIC Program Examples

Maze generation, Prime numbers, etc.
See a [separate page](programs/README.md) for more info

## Design

The goal was to write an interpreter with a minimum amount of code yet capable of running the sample programs as is and without cheating. It means that many classical concepts of compiler construction theory were omitted for the sake of simplicity. Here are some key decisions and related consequences:
* Preparse step ([`bool Preparse()`](basic_int.cpp)), which copies the whole program into memory. Doing so, it indexes lines for fast `GOTO` and separately stores `DATA` section. Also, it combines multiline statement sequences together.
* No tokenization / lexical analysis step. The parser works with characters directly. It increases parser complexity and likely slows it down. Additionally, some "nospace inputs" aren't supported, e.g., in `IFK9>T9THENT9=K9` the substring `T9THENT9` will be recognized as an identifier instead of 2 identifiers and the `then` keyword. (It could be supported using lookahead syntax in `identifier_def` rule). The "right" approach could leverage **`Boost.Spirit.Lex`** or old trusty [**Flex**](https://en.wikipedia.org/wiki/Flex_(lexical_analyser_generator)) to generate the lexical analyzer.
* Absence of Abstract Syntax Tree (AST) and bytecode generations as well as no separate execution step. I.e. parsing and execution happen simultaneously without intermediate representation. That's the biggest hack. The obvious issue is that each iteration of the loop involves parsing which isn't optimal. Also, there is some increased complexity of the grammar to support proper backtracking. The hardest thing though was the `ESLE` part of the condition statement. To solve that [`class SkipStatementRuntime`](runtime.h) was created and [`bool ParseSequence()`](parse_utils.hpp) complexity came from that. The "right" approach could involve the generation of AST, bytecode with following separate execution, or even generation of LLVM IR and making the real compiler.

## Useful Links

See a [separate page](NOTES.md) for more info
