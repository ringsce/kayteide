# Qt6 Multi-Language Test Runner

A polished Qt6 desktop GUI for running and visualising unit tests across **multiple languages**.

## Supported file types

| Extension | Language | Runner |
|-----------|----------|--------|
| `.kayte`  | Kayte (BASIC-style) | `kayte <file>` interpreter |
| `.c`      | C        | `gcc` → compile + run |
| `.cpp` `.cxx` `.cc` | C++ | `g++` → compile + run |
| `.h` `.hpp` `.hxx`  | C/C++ Header | `gcc`/`g++` `-fsyntax-only` |
| `.php`    | PHP      | `php -f <file>` |
| `.pl`     | Perl     | `perl -w <file>` |
| `.pas`    | Pascal   | `fpc` → compile + run |
| *(no ext / other)* | QTest binary | run directly with `-xml` |

## Features

- **Multi-file queue** — add as many files as you like; they run sequentially
- **Drag-to-reorder** file list
- **Results tab** — tree grouped by file; colour-coded PASS/FAIL/SKIP
- **Source tab** — syntax-highlighted viewer (per-language) that auto-loads on selection
- **Log tab** — live streaming raw output with colour highlights
- **Filter bar** — narrow results by test name or message
- **Summary bar** — total / passed / failed / skipped across all files
- **Dark Catppuccin-Mocha theme** built-in

## Build requirements

| Tool  | Version |
|-------|---------|
| CMake | ≥ 3.21  |
| Qt    | 6.x (Core, Widgets, Test) |
| C++   | 17      |

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Launch the GUI
./build/src/TestRunnerApp

# Pre-load a file on startup
./build/src/TestRunnerApp ./build/tests/SampleMathTests
```

## Running QTest suites headlessly (CTest)

```bash
cd build && ctest --output-on-failure
```

## Project layout

```
qt6-test-gui/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── LanguageRunner.h/.cpp       ← per-extension command builder
│   ├── MultiTestProcess.h/.cpp     ← multi-language process runner + parser
│   ├── SyntaxHighlighter.h/.cpp    ← C/C++, PHP, Perl, Pascal, Kayte highlighter
│   ├── TestProcess.h/.cpp          ← QTest XML runner (existing)
│   └── TestRunnerWindow.h/.cpp     ← main GUI window
└── tests/
    ├── SampleMathTests.cpp         ← Qt unit tests
    ├── SampleStringTests.cpp
    ├── test_sample.c
    ├── test_sample.kayte
    ├── test_sample.pas
    ├── test_sample.php
    └── test_sample.pl
```

## Kayte language support

Kayte is a BASIC-style language. The runner:
1. Executes `kayte <file>` via the system PATH
2. Treats each line of `PRINT` output as a passing assertion
3. Detects `PASS:` / `FAIL:` prefixes in output for structured results
4. Reports non-zero exit codes as a failing `exit` test case

Make sure the `kayte` interpreter is on your `PATH` before running `.kayte` files.

## Adding custom interpreters

Edit `src/LanguageRunner.cpp` and add a new `else if (ext == "xyz")` branch.
