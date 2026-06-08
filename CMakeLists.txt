cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(KayteIDE
        VERSION 1.0.0
        DESCRIPTION "KayteIDE – The Kayte Language IDE"
        LANGUAGES CXX
)

# ─── C++17 ────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Qt automation ────────────────────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ─── Architecture (macOS) ─────────────────────────────────────────────────────
if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
            set(CMAKE_OSX_ARCHITECTURES "arm64"
                    CACHE STRING "Build architectures for macOS")
        else()
            set(CMAKE_OSX_ARCHITECTURES "x86_64"
                    CACHE STRING "Build architectures for macOS")
        endif()
    endif()
    message(STATUS "macOS – processor: ${CMAKE_SYSTEM_PROCESSOR}  architectures: ${CMAKE_OSX_ARCHITECTURES}")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Linux – processor: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# ─── Custom CMake module path ─────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ─── Updater subdirectory path (used by both KayteIDE and updater targets) ────
# All updater source/resource paths go through this variable.
set(UPDATER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/updater")

# ─── Qt 6 ─────────────────────────────────────────────────────────────────────
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Concurrent Network)

# ─── libgit2 ──────────────────────────────────────────────────────────────────
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBGIT2 QUIET libgit2)
endif()

include(FetchContent)

if(NOT LIBGIT2_FOUND)
    message(STATUS "libgit2 not found via pkg-config – fetching via FetchContent…")
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
    set(LIBGIT2_TARGET git2)
else()
    set(LIBGIT2_TARGET ${LIBGIT2_LINK_LIBRARIES})
endif()

# ─── IconFontCppHeaders (Font Awesome 6 glyph → Unicode map) ──────────────────
FetchContent_Declare(
        IconFontCppHeaders
        GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(IconFontCppHeaders)

# ─── KayteGitClient static library ───────────────────────────────────────────
set(GIT_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gitclient")

set(GIT_CLIENT_SOURCES
        ${GIT_CLIENT_DIR}/src/GitRepository.cpp
        ${GIT_CLIENT_DIR}/src/GitCredentialDialog.cpp
        ${GIT_CLIENT_DIR}/src/GitClientPanel.cpp
        ${GIT_CLIENT_DIR}/src/GitLogModel.cpp
        ${GIT_CLIENT_DIR}/src/GitBranchManager.cpp
        ${GIT_CLIENT_DIR}/src/GitDiffView.cpp
)

set(GIT_CLIENT_HEADERS
        ${GIT_CLIENT_DIR}/include/GitRepository.hpp
        ${GIT_CLIENT_DIR}/include/GitCredentialDialog.hpp
        ${GIT_CLIENT_DIR}/include/GitClientPanel.hpp
        ${GIT_CLIENT_DIR}/include/GitLogModel.hpp
        ${GIT_CLIENT_DIR}/include/GitBranchManager.hpp
        ${GIT_CLIENT_DIR}/include/GitDiffView.hpp
)

add_library(KayteGitClient STATIC
        ${GIT_CLIENT_SOURCES}
        ${GIT_CLIENT_HEADERS}
)

qt_wrap_cpp(GIT_CLIENT_MOC_SOURCES
        ${GIT_CLIENT_HEADERS}
        TARGET KayteGitClient
)
target_sources(KayteGitClient PRIVATE ${GIT_CLIENT_MOC_SOURCES})
set_target_properties(KayteGitClient PROPERTIES AUTOMOC OFF)

target_include_directories(KayteGitClient
        PUBLIC  ${GIT_CLIENT_DIR}/include
        PRIVATE ${GIT_CLIENT_DIR}/src
                ${GIT_CLIENT_DIR}/include
)
target_link_libraries(KayteGitClient
        PUBLIC  Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent
        PRIVATE ${LIBGIT2_TARGET}
)

# ─── KayteSvnClient static library ───────────────────────────────────────────
set(SVN_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/svn")

find_package(Subversion QUIET)
if(Subversion_FOUND)
    message(STATUS "SVN – native libsvn ${Subversion_VERSION} found (full API mode)")
    set(KAYTE_SVN_NATIVE TRUE)
else()
    message(STATUS "SVN – libsvn not found; using QProcess/CLI fallback (requires `svn` in PATH)")
    set(KAYTE_SVN_NATIVE FALSE)
endif()

set(SVN_CLIENT_SOURCES
        ${SVN_CLIENT_DIR}/SvnClient.cpp
        ${SVN_CLIENT_DIR}/SvnModel.cpp
        ${SVN_CLIENT_DIR}/SvnPanel.cpp
        ${SVN_CLIENT_DIR}/SvnLogDialog.cpp
        ${SVN_CLIENT_DIR}/SvnCommitDialog.cpp
        ${SVN_CLIENT_DIR}/SvnStatusDelegate.cpp
)
if(KAYTE_SVN_NATIVE)
    list(APPEND SVN_CLIENT_SOURCES ${SVN_CLIENT_DIR}/SvnNativeBinding.cpp)
endif()

add_library(KayteSvnClient STATIC ${SVN_CLIENT_SOURCES})
add_library(KayteIDE::SvnClient ALIAS KayteSvnClient)

target_compile_features(KayteSvnClient PUBLIC cxx_std_17)
target_compile_definitions(KayteSvnClient
        PRIVATE
        $<$<BOOL:${KAYTE_SVN_NATIVE}>:KAYTE_SVN_NATIVE>
        QT_NO_KEYWORDS
)
target_include_directories(KayteSvnClient
        PUBLIC  ${SVN_CLIENT_DIR}
        PRIVATE $<$<BOOL:${KAYTE_SVN_NATIVE}>:${Subversion_INCLUDE_DIRS}>
)
target_link_libraries(KayteSvnClient
        PUBLIC  Qt6::Core Qt6::Widgets
        PRIVATE Qt6::Concurrent Qt6::Network
                $<$<BOOL:${KAYTE_SVN_NATIVE}>:${Subversion_LIBRARIES}>
)

# ─── Unit-test plugin ─────────────────────────────────────────────────────────
set(UNIT_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/plugins/unit_test")

set(UNIT_CLIENT_SOURCES
        ${UNIT_CLIENT_DIR}/src/LanguageRunner.cpp
        ${UNIT_CLIENT_DIR}/src/main.cpp
        ${UNIT_CLIENT_DIR}/src/MultiTestProcess.cpp
        ${UNIT_CLIENT_DIR}/src/SyntaxHighlighter.cpp
        ${UNIT_CLIENT_DIR}/src/TestProcess.cpp
        ${UNIT_CLIENT_DIR}/src/TestRunnerWindow.cpp
)

set(UNIT_CLIENT_HEADERS
        ${UNIT_CLIENT_DIR}/src/LanguageRunner.h
        ${UNIT_CLIENT_DIR}/src/MultiTestProcess.h
        ${UNIT_CLIENT_DIR}/src/SyntaxHighlighter.h
        ${UNIT_CLIENT_DIR}/src/TestProcess.h
        ${UNIT_CLIENT_DIR}/src/TestRunnerWindow.h
)

# ─── Main application sources ─────────────────────────────────────────────────
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
        src/editortabwidget.cpp
        plugins/project/newprojectdialog.cpp
        src/widgetpalettedock.cpp
        src/uicanvaswidget.cpp
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
        src/editortabwidget.h
        src/threads/threadmanager.h
        src/threads/posixthread.h
        src/keyboard.h
        plugins/project/newprojectdialog.h
        src/widgetpalettedock.h
        src/uicanvaswidget.h
)

set(PROJECT_FORMS
        src/mainwindow.ui
        src/downloadprogressdialog.ui
        src/choicemode.ui
        plugins/project/newprojectdialog.ui
)

set(PROJECT_RESOURCES
        resources/i18n.qrc
        resources/main_resources.qrc
)

# ─── Font Awesome resource file ───────────────────────────────────────────────
# Shared between KayteIDE and the updater – lives in resources/ at repo root.
set(FA_QRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc")
set(FA_TTF "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/fa-solid-900.ttf")

if(NOT EXISTS "${FA_QRC}")
    message(STATUS "Creating stub resources/resources.qrc (Font Awesome TTF not bundled yet)")
    file(WRITE "${FA_QRC}"
        "<!DOCTYPE RCC>\n"
        "<RCC version=\"1.0\">\n"
        "  <!-- Place fa-solid-900.ttf in resources/fonts/ to enable Font Awesome icons. -->\n"
        "  <!-- Download: https://use.fontawesome.com/releases/v6.5.1/fontawesome-free-6.5.1-desktop.zip -->\n"
        "  <qresource/>\n"
        "</RCC>\n"
    )
endif()

if(EXISTS "${FA_TTF}")
    message(STATUS "Font Awesome TTF found – enabling FA icon glyphs")
    file(WRITE "${FA_QRC}"
        "<!DOCTYPE RCC>\n"
        "<RCC version=\"1.0\">\n"
        "  <qresource>\n"
        "    <file alias=\"fa-solid-900.ttf\">fonts/fa-solid-900.ttf</file>\n"
        "  </qresource>\n"
        "</RCC>\n"
    )
else()
    message(STATUS "Font Awesome TTF not found – icons use text fallback. "
        "Re-run cmake after placing fa-solid-900.ttf in resources/fonts/")
endif()

list(APPEND PROJECT_RESOURCES "${FA_QRC}")

# ─── Platform-specific source additions ───────────────────────────────────────
if(ANDROID)
    message(STATUS "Configuring for Android API ${ANDROID_API_LEVEL}")
    set(ANDROID_API_LEVEL         21           CACHE STRING "Minimum Android API Level")
    set(ANDROID_NATIVE_API_LEVEL  ${ANDROID_API_LEVEL})
    set(ANDROID_STL               "c++_shared"  CACHE STRING "Android C++ STL")
    set(ANDROID_ABI               "arm64-v8a;x86_64" CACHE STRING "Android ABIs")
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
else()
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin|Linux")
        list(APPEND PROJECT_SOURCES
                src/threads/threadmanager.cpp
                src/threads/posixthread.cpp
                src/keyboard.cpp
        )
    endif()
endif()

# ─── Doxygen ──────────────────────────────────────────────────────────────────
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYFILE_IN  "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
    set(DOXYFILE_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
    if(EXISTS "${DOXYFILE_IN}")
        configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)
        add_custom_target(docs ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM
        )
    else()
        message(WARNING "Doxyfile.in not found – skipping doc target.")
    endif()
else()
    message(STATUS "Doxygen not found – documentation target disabled.")
endif()

# ─── KayteIDE executable ──────────────────────────────────────────────────────
qt_add_executable(${PROJECT_NAME}
        ${PROJECT_SOURCES}
        ${PROJECT_HEADERS}
        ${PROJECT_FORMS}
        ${PROJECT_RESOURCES}
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins/project
        ${GIT_CLIENT_DIR}/include
        ${SVN_CLIENT_DIR}
        ${iconfontcppheaders_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Concurrent
        Qt6::Network
        KayteGitClient
        KayteIDE::SvnClient
)

target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        $<$<CONFIG:Debug>:QT_QML_DEBUG>
        KAYTEIDE_VERSION="${PROJECT_VERSION}"
)

# ─── Android-specific properties ─────────────────────────────────────────────
if(ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            QT_ANDROID_PACKAGE_SOURCE_DIR    "${ANDROID_PACKAGE_SOURCE_DIR}"
            QT_ANDROID_PACKAGE_APPLICATION_LABEL "KayteIDE"
            MACOSX_BUNDLE FALSE
    )
endif()

# ─── POSIX threads ────────────────────────────────────────────────────────────
if(NOT ANDROID AND CMAKE_SYSTEM_NAME MATCHES "Darwin|Linux")
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
endif()

# ─── macOS bundle (KayteIDE) ──────────────────────────────────────────────────
if(APPLE AND NOT ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE                TRUE
            MACOSX_BUNDLE_ICON_FILE      "kayte.icns"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.ringsce.kayteide"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            MACOSX_BUNDLE_INFO_PLIST     "${CMAKE_SOURCE_DIR}/resources/Info.plist"
    )
    set_source_files_properties(resources/kayte.icns
            PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/tools/sdk/usr/bin"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/tools/sdk/usr/bin/kayte"
            COMMAND chmod +x
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/tools/sdk/usr/bin/kayte"
            COMMENT "Installing kayte tool inside app bundle"
            VERBATIM
    )

    set(DMG_SCRIPT "${CMAKE_SOURCE_DIR}/create_dmg.sh")
    if(EXISTS "${DMG_SCRIPT}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${DMG_SCRIPT}
                "${CMAKE_BINARY_DIR}/bin"
                "${CMAKE_SOURCE_DIR}/resources/dmg_background.png"
                COMMENT "Creating macOS DMG installer"
                VERBATIM
        )
    endif()
endif()

# ─── Output directory ─────────────────────────────────────────────────────────
if(NOT ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# Auto-Updater (KayteIDEUpdater) – built as part of the same cmake invocation
# Sources live in updater/src/; resources in updater/resources/.
# The updater shares libgit2, IconFontCppHeaders, KayteGitClient, and
# KayteSvnClient that were already fetched/built above – no duplication.
# ═══════════════════════════════════════════════════════════════════════════════

# ─── Updater Font Awesome resource stub ───────────────────────────────────────
# The updater has its own .qrc so it can be distributed independently.
set(UPDATER_FA_QRC "${UPDATER_DIR}/resources/resources.qrc")
set(UPDATER_FA_TTF "${UPDATER_DIR}/resources/fonts/fa-solid-900.ttf")

if(NOT EXISTS "${UPDATER_FA_QRC}")
    message(STATUS "Creating stub updater/resources/resources.qrc")
    file(MAKE_DIRECTORY "${UPDATER_DIR}/resources")
    file(WRITE "${UPDATER_FA_QRC}"
        "<!DOCTYPE RCC>\n"
        "<RCC version=\"1.0\">\n"
        "  <!-- Place fa-solid-900.ttf in updater/resources/fonts/ to enable FA icons. -->\n"
        "  <qresource/>\n"
        "</RCC>\n"
    )
endif()

if(EXISTS "${UPDATER_FA_TTF}")
    file(WRITE "${UPDATER_FA_QRC}"
        "<!DOCTYPE RCC>\n"
        "<RCC version=\"1.0\">\n"
        "  <qresource>\n"
        "    <file alias=\"fa-solid-900.ttf\">fonts/fa-solid-900.ttf</file>\n"
        "  </qresource>\n"
        "</RCC>\n"
    )
endif()

# ─── Updater sources ──────────────────────────────────────────────────────────
set(UPDATER_SOURCES
        ${UPDATER_DIR}/src/main.cpp
        ${UPDATER_DIR}/src/MainWindow.cpp
        ${UPDATER_DIR}/src/UpdateChecker.cpp
        ${UPDATER_DIR}/src/BuildManager.cpp
        ${UPDATER_DIR}/src/LogWidget.cpp
)

set(UPDATER_HEADERS
        ${UPDATER_DIR}/src/MainWindow.h
        ${UPDATER_DIR}/src/UpdateChecker.h
        ${UPDATER_DIR}/src/BuildManager.h
        ${UPDATER_DIR}/src/LogWidget.h
)

# ─── Updater executable ───────────────────────────────────────────────────────
qt_add_executable(KayteIDEUpdater
        ${UPDATER_SOURCES}
        ${UPDATER_HEADERS}
        ${UPDATER_FA_QRC}
)

target_include_directories(KayteIDEUpdater
        PRIVATE
        ${UPDATER_DIR}/src
        ${iconfontcppheaders_SOURCE_DIR}
        ${GIT_CLIENT_DIR}/include
        ${SVN_CLIENT_DIR}
)

target_link_libraries(KayteIDEUpdater
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Concurrent
        Qt6::Network
        KayteGitClient
        KayteIDE::SvnClient
)

target_compile_definitions(KayteIDEUpdater
        PRIVATE
        KAYTEIDE_REPO_URL="https://github.com/ringsce/kayteide.git"
        KAYTEIDE_API_URL="https://api.github.com/repos/ringsce/kayteide"
        KAYTEIDE_VERSION="${PROJECT_VERSION}"
        $<$<CONFIG:Debug>:QT_QML_DEBUG>
)

# POSIX threads for updater
if(NOT ANDROID AND CMAKE_SYSTEM_NAME MATCHES "Darwin|Linux")
    target_link_libraries(KayteIDEUpdater PRIVATE Threads::Threads)
endif()

# macOS bundle for updater
if(APPLE AND NOT ANDROID)
    set_target_properties(KayteIDEUpdater PROPERTIES
            MACOSX_BUNDLE                TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.ringsce.kayteide.updater"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
    # After KayteIDE is bundled, embed the updater inside it so users can
    # launch it from KayteIDE's Help menu or run it standalone.
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/MacOS"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:KayteIDEUpdater>"
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/MacOS/KayteIDEUpdater"
            COMMENT "Embedding KayteIDEUpdater inside KayteIDE.app bundle"
            VERBATIM
    )
endif()

# Output directory for updater binary
if(NOT ANDROID)
    set_target_properties(KayteIDEUpdater PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# ─── Sanitizers (both targets) ────────────────────────────────────────────────
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN AND NOT ANDROID)
    foreach(_t ${PROJECT_NAME} KayteIDEUpdater KayteGitClient KayteSvnClient)
        target_compile_options(${_t} PRIVATE -fsanitize=address -g)
        target_link_options(${_t}    PRIVATE -fsanitize=address)
    endforeach()
endif()

# ─── CPack – DEB + RPM (Linux desktop only) ───────────────────────────────────
if(NOT ANDROID AND UNIX AND NOT APPLE)
    include(GNUInstallDirs)

    install(TARGETS ${PROJECT_NAME} KayteIDEUpdater
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    install(FILES
            "${CMAKE_SOURCE_DIR}/resources/kayteide.desktop"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    install(FILES
            "${CMAKE_SOURCE_DIR}/resources/kayte.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps")

    # Optional updater .desktop entry
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/kayteide-updater.desktop")
        install(FILES "${CMAKE_SOURCE_DIR}/resources/kayteide-updater.desktop"
                DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    endif()

    set(CPACK_PACKAGE_NAME              "kayteide")
    set(CPACK_PACKAGE_VERSION           "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "KayteIDE – The Kayte Language IDE")
    set(CPACK_PACKAGE_VENDOR            "ringsce")
    set(CPACK_PACKAGE_CONTACT           "contact@ringsce.com")
    set(CPACK_PACKAGE_HOMEPAGE_URL      "https://ringsce.com")
    set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/LICENSE")

    # Debian
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
            "libqt6core6, libqt6gui6, libqt6widgets6, libqt6network6, libgit2-1.5 | libgit2-dev, subversion, git, cmake")
    set(CPACK_DEBIAN_PACKAGE_SECTION    "devel")

    # RPM
    list(APPEND CPACK_GENERATOR "RPM")
    set(CPACK_RPM_PACKAGE_LICENSE       "MIT")
    set(CPACK_RPM_PACKAGE_GROUP         "Development/Tools")
    set(CPACK_RPM_PACKAGE_REQUIRES      "qt6-qtbase, libgit2, subversion, git, cmake")
    set(CPACK_RPM_PACKAGE_URL           "https://ringscejs.gleentech.com")

    include(CPack)
endif()