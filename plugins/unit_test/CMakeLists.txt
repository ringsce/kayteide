cmake_minimum_required(VERSION 3.21)
project(Qt6TestRunner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Test
)

# ── GUI Test Runner app ──────────────────────────────────────────────────────
add_subdirectory(src)

# ── Unit tests ───────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
