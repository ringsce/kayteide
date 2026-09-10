# plugins/pas2dox/CMakeLists.txt
#
# Builds the `pas2dox` executable: a Pascal/Delphi -> pseudo-C++ filter
# meant to be used as a Doxygen INPUT_FILTER / FILTER_PATTERNS program (see
# ../../Doxyfile.in, where the top-level CMakeLists.txt wires this in
# automatically when Flex is available).

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(pas2dox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Flex tool.
#
# On macOS, prefer Homebrew's genuine GNU flex over Apple's bundled
# /usr/bin/flex (an older, modified fork: `flex --version` reports "2.6.4
# Apple(flex-35)") *before* find_package(Flex) runs its own search - Apple's
# flex generates C++ scanner code (LexerInput/LexerOutput taking `size_t`)
# that doesn't match the `int`-based FlexLexer.h signatures shipped by
# Homebrew's flex, which is otherwise the only FlexLexer.h available on a
# stock macOS system. Mixing the two fails to compile. If FLEX_EXECUTABLE is
# already cached (e.g. set explicitly by the caller), this is a no-op.
if(APPLE)
    find_program(FLEX_EXECUTABLE
            NAMES flex
            PATHS /opt/homebrew/opt/flex/bin /usr/local/opt/flex/bin
            NO_DEFAULT_PATH)
endif()
find_package(Flex REQUIRED)

# Define the Flex source file (the .l file that Flex reads)
set(PAS2DOX_LEX_FILE "${CMAKE_CURRENT_SOURCE_DIR}/pas2dox.l")

# Define the main C++ source file (containing other lexer logic)
set(PAS2DOX_MAIN_CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/pas2dox.cpp")

# Define the name of the C++ source file that Flex will generate.
set(PAS2DOX_GENERATED_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/pas2dox_lexer.cpp")

# Add a custom command to run Flex.
add_custom_command(
        OUTPUT ${PAS2DOX_GENERATED_SOURCE}
        COMMAND ${FLEX_EXECUTABLE} --c++ -o ${PAS2DOX_GENERATED_SOURCE} ${PAS2DOX_LEX_FILE}
        DEPENDS ${PAS2DOX_LEX_FILE}
        COMMENT "Running Flex to generate ${PAS2DOX_GENERATED_SOURCE}"
        VERBATIM
)

# Create the executable target. This MUST come BEFORE
# target_include_directories / target_link_libraries for this target.
add_executable(pas2dox
        ${PAS2DOX_MAIN_CPP_FILE}
        ${PAS2DOX_GENERATED_SOURCE}
)

# Add include directories for headers.
target_include_directories(pas2dox PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR} # To find pasdoxflexlexer.h
)

# Add include directory for FlexLexer.h.
if (FLEX_INCLUDE_DIRS)
    target_include_directories(pas2dox PRIVATE ${FLEX_INCLUDE_DIRS})
else()
    # Fallback if FindFlex doesn't set FLEX_INCLUDE_DIRS
    find_path(FLEX_SYSTEM_INCLUDE_DIR FlexLexer.h
            PATHS
            /opt/homebrew/opt/flex/include
            /opt/homebrew/include
            /usr/local/opt/flex/include
            /usr/local/include
            /usr/include
            ${CMAKE_INSTALL_PREFIX}/include
            NO_CMAKE_FIND_ROOT_PATH)
    if (FLEX_SYSTEM_INCLUDE_DIR)
        target_include_directories(pas2dox PRIVATE ${FLEX_SYSTEM_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "Could not find FlexLexer.h include directory. Please ensure Flex is installed or set FLEX_INCLUDE_DIRS.")
    endif()
endif()

# Link libraries if needed. Flex-generated code often requires the Flex runtime library.
if (FLEX_LIBRARIES)
    target_link_libraries(pas2dox PRIVATE ${FLEX_LIBRARIES})
elseif (UNIX)
    find_library(FLEX_RUNTIME_LIBRARY NAMES fl libfl)
    if (FLEX_RUNTIME_LIBRARY)
        target_link_libraries(pas2dox PRIVATE ${FLEX_RUNTIME_LIBRARY})
    endif()
endif()

set_target_properties(pas2dox PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        # This is a plain Flex/C++ tool with no Qt involvement - when built
        # as part of the main KayteIDE project (which sets these ON
        # globally), explicitly opt back out rather than silently pay for
        # (harmless but pointless) MOC/UIC/RCC steps.
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
)

install(TARGETS pas2dox RUNTIME DESTINATION bin)