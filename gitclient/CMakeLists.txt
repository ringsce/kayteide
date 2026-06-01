cmake_minimum_required(VERSION 3.21)
project(KayteIDEGitClient VERSION 1.0.0 LANGUAGES CXX)

# ─── C++17 ───────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Qt 6 ────────────────────────────────────────────────────────────────────
# Keep AUTORCC/AUTOUIC on, but we handle MOC manually below (see note).
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Tell AUTOUIC where the .ui files live
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

# ─── libgit2 ─────────────────────────────────────────────────────────────────
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBGIT2 QUIET libgit2)
endif()

if(NOT LIBGIT2_FOUND)
    message(STATUS "libgit2 not found via pkg-config – using FetchContent")
    include(FetchContent)
    FetchContent_Declare(
            libgit2
            GIT_REPOSITORY https://github.com/libgit2/libgit2.git
            GIT_TAG        v1.7.2
            GIT_SHALLOW    TRUE
    )
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS       OFF CACHE BOOL "" FORCE)
    set(BUILD_CLI         OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(libgit2)
    set(LIBGIT2_TARGET libgit2package)
else()
    set(LIBGIT2_TARGET ${LIBGIT2_LINK_LIBRARIES})
endif()

# ─── Sources ─────────────────────────────────────────────────────────────────
set(GIT_CLIENT_SOURCES
        src/GitRepository.cpp
        src/GitCredentialDialog.cpp
        src/GitClientPanel.cpp
        src/GitLogModel.cpp
        src/GitBranchManager.cpp
        src/GitDiffView.cpp
)

set(GIT_CLIENT_HEADERS
        include/GitRepository.hpp
        include/GitCredentialDialog.hpp
        include/GitClientPanel.hpp
        include/GitLogModel.hpp
        include/GitBranchManager.hpp
        include/GitDiffView.hpp
)

set(GIT_CLIENT_UI
        ui/GitClientPanel.ui
        ui/GitCredentialDialog.ui
)

# ─── Explicit MOC pass ───────────────────────────────────────────────────────
# AUTOMOC can miss headers that live in a directory separate from the .cpp
# files (include/ vs src/), leaving all signal bodies and vtables undefined
# at link time.  qt_wrap_cpp runs moc on every listed header unconditionally,
# producing moc_*.cpp files that are compiled directly into the library.
qt_wrap_cpp(GIT_CLIENT_MOC_SOURCES
        ${GIT_CLIENT_HEADERS}
        TARGET KayteGitClient          # gives moc the right compile definitions
)

# ─── Library target ───────────────────────────────────────────────────────────
add_library(KayteGitClient STATIC
        ${GIT_CLIENT_SOURCES}
        ${GIT_CLIENT_HEADERS}
        ${GIT_CLIENT_MOC_SOURCES}      # <-- the generated moc_*.cpp files
        ${GIT_CLIENT_UI}
)

# Disable AUTOMOC on this target – we already ran moc manually above.
set_target_properties(KayteGitClient PROPERTIES AUTOMOC OFF)

target_include_directories(KayteGitClient
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        # Needed so moc-generated files can #include their own header
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(KayteGitClient
        PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Concurrent
        PRIVATE
        ${LIBGIT2_TARGET}
)

# ─── Standalone demo ─────────────────────────────────────────────────────────
option(BUILD_GIT_CLIENT_DEMO "Build standalone demo application" ON)
if(BUILD_GIT_CLIENT_DEMO)
    add_executable(KayteGitDemo demo/main.cpp)
    target_link_libraries(KayteGitDemo PRIVATE KayteGitClient)
endif()

# ─── Installation ─────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS KayteGitClient
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/KayteGitClient)