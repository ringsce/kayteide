# ─────────────────────────────────────────────────────────────────────────────
# QTest suites (compiled Qt unit tests — run via the GUI or CTest)
# ─────────────────────────────────────────────────────────────────────────────
macro(add_qt_test TARGET)
    qt6_add_executable(${TARGET} ${ARGN})
    target_link_libraries(${TARGET} PRIVATE Qt6::Test Qt6::Core)
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endmacro()

add_qt_test(SampleMathTests   SampleMathTests.cpp)
add_qt_test(SampleStringTests SampleStringTests.cpp)

# ─────────────────────────────────────────────────────────────────────────────
# Copy interpreted/source test files to the build tree so they're easy to find
# ─────────────────────────────────────────────────────────────────────────────
set(SCRIPT_TESTS
    test_sample.php
    test_sample.pl
    test_sample.pas
    test_sample.kayte
    test_sample.c
)

foreach(f IN LISTS SCRIPT_TESTS)
    configure_file(${f} ${CMAKE_CURRENT_BINARY_DIR}/${f} COPYONLY)
endforeach()
