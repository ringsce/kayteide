##############################################################################
# kayteide – SVN Client Module
# Integrates Apache Subversion into the KaytEIDE application.
#
# Requires: Qt 6, C++17, and either libsvn or the svn command-line tool.
# Usage:    add_subdirectory(src/svn) from the root CMakeLists.txt, then
#           target_link_libraries(kayteide PRIVATE KayteIDE::SvnClient)
##############################################################################

cmake_minimum_required(VERSION 3.21)

project(KayteSvnClient
    VERSION 1.0.0
    DESCRIPTION "SVN client plugin for KaytEIDE"
    LANGUAGES CXX
)

# ── C++17 ──────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Qt 6 ───────────────────────────────────────────────────────────────────
find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core
    Widgets
    Concurrent
    Network      # used for HTTP-based SVN repos in future
)

qt_standard_project_setup()

# ── Optional: libsvn native binding ────────────────────────────────────────
# If the native SVN libraries are found we compile the thin C wrapper
# (SvnNativeBinding) and expose extra detail. When not found we fall back
# gracefully to driving the `svn` CLI through QProcess.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
find_package(Subversion QUIET)

if(Subversion_FOUND)
    message(STATUS "[SVN] Native libsvn found (${Subversion_VERSION}). "
                   "Building with native API support.")
    set(KAYTE_SVN_NATIVE TRUE)
else()
    message(STATUS "[SVN] libsvn not found. "
                   "Using QProcess/CLI fallback (requires `svn` in PATH).")
    set(KAYTE_SVN_NATIVE FALSE)
endif()

# ── Source files ────────────────────────────────────────────────────────────
set(SVN_SOURCES
    SvnClient.cpp
    SvnModel.cpp
    SvnPanel.cpp
    SvnLogDialog.cpp
    SvnCommitDialog.cpp
    SvnStatusDelegate.cpp
)

set(SVN_HEADERS
    SvnClient.h
    SvnModel.h
    SvnPanel.h
    SvnLogDialog.h
    SvnCommitDialog.h
    SvnStatusDelegate.h
    SvnTypes.h
)

if(KAYTE_SVN_NATIVE)
    list(APPEND SVN_SOURCES SvnNativeBinding.cpp)
    list(APPEND SVN_HEADERS SvnNativeBinding.h)
endif()

# ── Library target ──────────────────────────────────────────────────────────
qt_add_library(KayteSvnClient STATIC
    ${SVN_SOURCES}
    ${SVN_HEADERS}
)

add_library(KayteIDE::SvnClient ALIAS KayteSvnClient)

target_compile_features(KayteSvnClient PUBLIC cxx_std_17)

target_compile_definitions(KayteSvnClient
    PRIVATE
        $<$<BOOL:${KAYTE_SVN_NATIVE}>:KAYTE_SVN_NATIVE>
        QT_NO_KEYWORDS   # avoid conflicts with svn headers that use 'signals'
)

target_include_directories(KayteSvnClient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        $<$<BOOL:${KAYTE_SVN_NATIVE}>:${Subversion_INCLUDE_DIRS}>
)

target_link_libraries(KayteSvnClient
    PUBLIC
        Qt6::Core
        Qt6::Widgets
    PRIVATE
        Qt6::Concurrent
        Qt6::Network
        $<$<BOOL:${KAYTE_SVN_NATIVE}>:Subversion::Client>
        $<$<BOOL:${KAYTE_SVN_NATIVE}>:${Subversion_LIBRARIES}>
)

# ── Install rules (integrated into parent install) ──────────────────────────
include(GNUInstallDirs)
install(TARGETS KayteSvnClient
    EXPORT KayteIDE_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES ${SVN_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kayteide/svn
)
