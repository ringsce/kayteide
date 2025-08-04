# plugins/pas2dox/CMakeLists.txt
# This CMakeLists.txt builds the pas2dox library

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(pas2dox_library LANGUAGES CXX) # Change project name to reflect library

# Find the Flex tool
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

# Create the library target.
# Compile both your main C++ file and the Flex-generated lexer source.
# This MUST come BEFORE target_include_directories or target_link_libraries for this target.
add_library(pas2dox_lib STATIC # <--- CHANGE: Create a STATIC library
        ${PAS2DOX_MAIN_CPP_FILE}
        ${PAS2DOX_GENERATED_SOURCE}
)

# Add include directories for headers.
target_include_directories(pas2dox_lib PRIVATE # <--- CHANGE: Target is now pas2dox_lib
        ${CMAKE_CURRENT_SOURCE_DIR} # To find pasdoxflexlexer.h and globals.h
)

# Add include directory for FlexLexer.h.
if (FLEX_INCLUDE_DIRS)
    target_include_directories(pas2dox_lib PRIVATE ${FLEX_INCLUDE_DIRS}) # <--- CHANGE: Target is now pas2dox_lib
else()
    # Fallback if FindFlex doesn't set FLEX_INCLUDE_DIRS
    set(FLEX_SPECIFIC_INCLUDE_DIR "/opt/homebrew/Cellar/flex/2.6.4_2/include")
    target_include_directories(pas2dox_lib PRIVATE ${FLEX_SPECIFIC_INCLUDE_DIR}) # <--- CHANGE: Target is now pas2dox_lib
    find_path(FLEX_SYSTEM_INCLUDE_DIR FlexLexer.h
            PATHS
            ${FLEX_SPECIFIC_INCLUDE_DIR}
            /usr/local/include/FlexLexer
            /usr/local/include
            /usr/include
            /opt/homebrew/include
            ${CMAKE_INSTALL_PREFIX}/include
            NO_CMAKE_FIND_ROOT_PATH)
    if (FLEX_SYSTEM_INCLUDE_DIR)
        target_include_directories(pas2dox_lib PRIVATE ${FLEX_SYSTEM_INCLUDE_DIR}) # <--- CHANGE: Target is now pas2dox_lib
    else()
        message(FATAL_ERROR "Could not find FlexLexer.h include directory. Please ensure Flex is installed or set FLEX_SPECIFIC_INCLUDE_DIR.")
    endif()
endif()

# Link libraries if needed. Flex-generated code often requires the Flex runtime library.
if (FLEX_LIBRARIES)
    target_link_libraries(pas2dox_lib PRIVATE ${FLEX_LIBRARIES}) # <--- CHANGE: Target is now pas2dox_lib
elseif (UNIX)
    target_link_libraries(pas2dox_lib PRIVATE fl) # <--- CHANGE: Target is now pas2dox_lib
endif()

# Optional install rule (for the library)
install(TARGETS pas2dox_lib DESTINATION lib) # <--- CHANGE: Install as a library