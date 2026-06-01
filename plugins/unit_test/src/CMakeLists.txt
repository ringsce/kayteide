set(SOURCES
    main.cpp
    TestRunnerWindow.cpp
    TestRunnerWindow.h
    TestProcess.cpp
    TestProcess.h
    MultiTestProcess.cpp
    MultiTestProcess.h
    LanguageRunner.cpp
    LanguageRunner.h
    SyntaxHighlighter.cpp
    SyntaxHighlighter.h
)

qt6_add_executable(TestRunnerApp ${SOURCES})

target_link_libraries(TestRunnerApp PRIVATE
    Qt6::Core
    Qt6::Widgets
)

set_target_properties(TestRunnerApp PROPERTIES
    WIN32_EXECUTABLE TRUE
)
