cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(KayteIDE LANGUAGES CXX)

# --- Architecture Configuration ---
if (APPLE)
    if (NOT CMAKE_OSX_ARCHITECTURES)
        if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
            set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
        else()
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
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Qt 6 ---------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ------------------------------------------------------------------------
# List every source/header/ui file once
set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        linenumberarea.cpp
        vbsyntaxhighlighter.cpp
        cppsyntaxhighlighter.cpp
        kaytesyntaxhighlighter.cpp
        pascalsyntaxhighlighter.cpp
        delphisyntaxhighlighter.cpp
)

set(PROJECT_HEADERS
        mainwindow.h
        linenumberarea.h
        vbsyntaxhighlighter.h
        cppsyntaxhighlighter.h
        kaytesyntaxhighlighter.h
        pascalsyntaxhighlighter.h
        delphisyntaxhighlighter.h
)
set(PROJECT_FORMS
        mainwindow.ui
)

# FIX IS HERE: Change ${APP_SOURCES} to ${PROJECT_SOURCES}
qt_add_executable(${PROJECT_NAME}
        ${PROJECT_SOURCES} # <--- CORRECTED THIS LINE!
        ${PROJECT_HEADERS}
        ${PROJECT_FORMS}
)

# --- macOS Specific: Add App Icon and Bundle Generation ---
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
# ------------------------------------------------------------------------

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        $<$<CONFIG:Debug>:QT_QML_DEBUG>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)