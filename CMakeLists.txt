cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(KayteIDE LANGUAGES CXX)

# --- Architecture Configuration ---
if (APPLE)
    if (NOT CMAKE_OSX_ARCHITECTURES)
        if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
            # Build for both arm64 (Apple Silicon) and x86_64 (Intel) by default
            set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
        else()
            # Default to x86_64 for Intel Macs
            set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures for macOS")
        endif()
    endif()

elseif (UNIX AND NOT APPLE)
    message(STATUS "Building for Linux on ${CMAKE_SYSTEM_PROCESSOR} architecture.")
endif()
# ----------------------------------

# C++17 everywhere
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Use strict C++17

# ---- Qt 6 ---------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Enable automatic processing for Qt's meta-object system, UI files, and resource files
set(CMAKE_AUTOMOC ON) # Processes Q_OBJECT macros in headers
set(CMAKE_AUTOUIC ON) # Processes .ui files to generate ui_*.h
set(CMAKE_AUTORCC ON) # Processes .qrc files to embed resources

# ------------------------------------------------------------------------
# List every source/header/ui file once
# CMake's AUTOUIC/AUTOMOC will handle the dependencies
# You only need to list the .cpp files, .ui files, and .qrc files (if any)
# in the add_executable command. Headers are found via includes in .cpp files.

set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/linenumberarea.cpp
        src/vbsyntaxhighlighter.cpp
        src/cppsyntaxhighlighter.cpp
        src/kaytesyntaxhighlighter.cpp
        src/pascalsyntaxhighlighter.cpp
        src/delphisyntaxhighlighter.cpp
        src/choicemode.cpp
        src/myjsontranslator.cpp
        src/downloadprogressdialog.cpp
        # ADD editortabwidget.cpp HERE - It's a source file
        src/editortabwidget.cpp
)

set(PROJECT_HEADERS
        src/mainwindow.h
        src/linenumberarea.h
        src/vbsyntaxhighlighter.h
        src/cppsyntaxhighlighter.h
        src/kaytesyntaxhighlighter.h
        src/pascalsyntaxhighlighter.h
        src/delphisyntaxhighlighter.h
        src/choicemode.h
        src/myjsontranslator.h
        src/downloadprogressdialog.h
        # ADD editortabwidget.h HERE - It's a header file
        src/editortabwidget.h
)

set(PROJECT_FORMS
        src/mainwindow.ui # List .ui files here. AUTOUIC will process them.
        src/downloadprogressdialog.ui
        src/choicemode.ui
)

set(PROJECT_RESOURCES
        resources/i18n.qrc
        resources/main_resources.qrc # <--- These are the actual QRC files
        # REMOVED: editortabwidget.h and editortabwidget.cpp from here
)


# Create the executable target.
# Only provide source files (.cpp) and UI files (.ui) directly.
# The headers will be included by the source files and AUTOMOC/AUTOUIC will process them.
qt_add_executable(${PROJECT_NAME}
        ${PROJECT_SOURCES}
        ${PROJECT_FORMS}
        # REMOVED: ${PROJECT_HEADERS} - Headers are implicitly handled by includes in .cpp files
        # If you include a header in PROJECT_HEADERS and it contains Q_OBJECT,
        # AUTOMOC will pick it up when processing the .cpp files that include it.
        # Explicitly listing them here is generally not needed and can sometimes confuse the build system.
        ${PROJECT_RESOURCES}
)

if (APPLE)
    set(MACOS_BUNDLE_ICON_FILE "${CMAKE_SOURCE_DIR}/resources/kayte.icns")
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_ICON_FILE "${MACOS_BUNDLE_ICON_FILE}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.kd.kayteide"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist"
    )
    set_source_files_properties(resources/kayte.icns PROPERTIES XCODE_EXPLICIT_FILE_TYPE .icns)
endif()

# Link the necessary Qt libraries to your executable
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

# Add compile definitions, typically for debug configurations
target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        $<$<CONFIG:Debug>:QT_QML_DEBUG> # Adds QT_QML_DEBUG for Debug builds
)

# Set the output directory for the executable
set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)