cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(libreparser LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

# ─── GTK4 (optional, alongside Qt6) ────────────────────────────────────────────
# Qt6 remains the primary toolkit everywhere (KDE on Linux, and macOS). GTK4 is
# additionally offered, behind KAYTE_GTK4_ENABLED, on:
#   • Linux (any architecture)
#   • macOS (any architecture)
# It is never pulled in on other platforms (e.g. Windows, Android), and even on
# Linux/macOS it only actually enables once pkg-config locates a real gtk4
# installation below — this flag just decides whether we look.
set(KAYTE_GTK4_TARGET_PLATFORM FALSE)
if(UNIX AND NOT APPLE AND NOT ANDROID)
    set(KAYTE_GTK4_TARGET_PLATFORM TRUE)
elseif(APPLE)
    set(KAYTE_GTK4_TARGET_PLATFORM TRUE)
endif()

set(KAYTE_GTK4_AVAILABLE FALSE)
if(KAYTE_GTK4_TARGET_PLATFORM)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GTK4 QUIET IMPORTED_TARGET gtk4)
        if(GTK4_FOUND)
            set(KAYTE_GTK4_AVAILABLE TRUE)
            message(STATUS "GTK4 ${GTK4_VERSION} found (${CMAKE_SYSTEM_PROCESSOR}) – enabling KAYTE_GTK4_ENABLED")
        else()
            message(STATUS "GTK4 not found via pkg-config – GTK4 integration disabled")
        endif()
    else()
        message(STATUS "pkg-config not found – GTK4 integration disabled")
    endif()
else()
    message(STATUS "GTK4 integration not applicable on this platform/architecture")
endif()

qt_add_executable(libreparser
        src/main.cpp
        src/UiXmlDocument.h
        src/UiXmlDocument.cpp
        src/XmlEditorWindow.h
        src/XmlEditorWindow.cpp
        src/Gtk4Editor.h
        src/Gtk4Editor.cpp
)

target_include_directories(libreparser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(libreparser PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_compile_definitions(libreparser PRIVATE
        $<$<BOOL:${KAYTE_GTK4_AVAILABLE}>:KAYTE_GTK4_ENABLED>
)

if(KAYTE_GTK4_AVAILABLE)
    target_link_libraries(libreparser PRIVATE PkgConfig::GTK4)
endif()

set_target_properties(libreparser PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
