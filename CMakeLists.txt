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
        src/editortabwidget.h
        # REMOVED: src/threads/threadmanager.cpp (This is a source file)
        src/threads/threadmanager.h
        # REMOVED: src/threads/posixthread.cpp (This is a source file)
        src/threads/posixthread.h
        # REMOVED: src/keyboard.cpp (This is a source file)
        src/keyboard.h # This should be a header
)

set(PROJECT_FORMS
        src/mainwindow.ui
        src/downloadprogressdialog.ui
        src/choicemode.ui
)

set(PROJECT_RESOURCES
        resources/i18n.qrc
        resources/main_resources.qrc

)

# --- Android Configuration (NEW) ---
if (ANDROID)
    message(STATUS "Configuring for Android build.")

    # Find Android SDK and NDK (these typically rely on environment variables
    # like ANDROID_SDK_ROOT and ANDROID_NDK_ROOT, or are passed via toolchain file)
    find_package(Android) # This module finds SDK/NDK and sets up paths if variables are defined

    # Required Android properties for the target APK
    # Adjust ANDROID_API_LEVEL as needed (e.g., 28, 30, 33, 34)
    set(ANDROID_API_LEVEL 21 CACHE STRING "Minimum Android API Level")
    set(ANDROID_NATIVE_API_LEVEL ${ANDROID_API_LEVEL}) # NDK's minimum API level

    # Define the C++ Standard Library. c++_shared is usually preferred for Qt apps.
    set(ANDROID_STL "c++_shared" CACHE STRING "Android C++ Standard Library (c++_shared or c++_static)")

    # Define ABIs. For tablets, arm64-v8a and x86_64 are most common.
    set(ANDROID_ABI "arm64-v8a;x86_64" CACHE STRING "Android ABIs to build for (e.g., armeabi-v7a;arm64-v8a;x86;x86_64)")

    # Ensure AndroidExtras module is found for Qt on Android
    find_package(Qt6 REQUIRED COMPONENTS AndroidExtras)

    # Define the source directory for Android-specific files (AndroidManifest.xml, res/, assets/)
    # This directory MUST exist in your source tree.
    # Example: KayteIDE/android/
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")

    # Ensure pthread and macOS specific features are *not* used for Android builds
    # The threading and DMG blocks will be wrapped in NOT ANDROID checks below.

else() # Not Android (i.e., Linux or macOS)

    # --- Add Threading Files Conditionally (for non-Android Linux/macOS) ---
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin" OR CMAKE_SYSTEM_NAME MATCHES "Linux")
        list(APPEND PROJECT_SOURCES
                src/threads/threadmanager.cpp
                src/threads/posixthread.cpp
                src/keyboard.cpp # This should be a source file
        )
    endif()
endif()
# -----------------------------------------


# Add Doxygen integration
find_package(Doxygen)

if(DOXYGEN_FOUND)
    message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")

    # Define the Doxyfile template and output location
    set(DOXYFILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
    set(DOXYFILE_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

    # Configure the Doxyfile (replace @PROJECT_SOURCE_DIR@ etc. with actual paths)
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

    # Create a custom target to run Doxygen
    add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
    )
    # This line ensures Doxygen runs only after pas2dox is built
    #add_dependencies(docs pas2dox_executable) # Removed this line as pas2dox_executable is now a library


    # Optionally, make documentation a dependency of your main target
    # add_dependencies(KayteIDE docs)

else()
    message(WARNING "Doxygen not found. Documentation will not be generated.")
endif()

# Create the executable target.
# For Android, `qt_add_executable` understands `QT_ANDROID_PACKAGE_SOURCE_DIR`.
qt_add_executable(${PROJECT_NAME}
        ${PROJECT_SOURCES}
        ${PROJECT_FORMS}
        ${PROJECT_RESOURCES}
)

# --- Android specific target properties ---
if (ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_PACKAGE_SOURCE_DIR}"
            # Optional Android-specific properties for release builds (requires keystore setup)
            # QT_ANDROID_PACKAGE_SIGNING_KEYSTORE "/path/to/your/debug.keystore"
            # QT_ANDROID_PACKAGE_SIGNING_KEYSTORE_PASSWORD "android"
            # QT_ANDROID_PACKAGE_SIGNING_KEY_ALIAS "androiddebugkey"
            # QT_ANDROID_PACKAGE_SIGNING_KEY_PASSWORD "android"
            QT_ANDROID_PACKAGE_APPLICATION_LABEL "KayteIDE for Android"
            # You might need to specify exact build tools/cmake/ndk versions used by your Android SDK/NDK setup
            # QT_ANDROID_PACKAGE_SDK_BUILD_TOOLS_VERSION "33.0.0"
            # QT_ANDROID_PACKAGE_SDK_CMAKE_VERSION "3.22.1"
            # QT_ANDROID_PACKAGE_SDK_NDK_GRADLE_VERSION "23.1.7779620"
    )
    # Link AndroidExtras if using Android-specific Qt APIs
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::AndroidExtras)

    # Disable macOS-specific bundle properties for Android builds
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE FALSE)
endif()
# ------------------------------------------

# --- Link POSIX threads library conditionally (for non-Android Linux/macOS) ---
if (NOT ANDROID AND (CMAKE_SYSTEM_NAME MATCHES "Darwin" OR CMAKE_SYSTEM_NAME MATCHES "Linux"))
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()
# ---------------------------------------------------------------------------------

# --- macOS Specific Properties (ensure not applied for Android) ---
if (APPLE AND NOT ANDROID)
    set(MACOS_BUNDLE_ICON_FILE "${CMAKE_SOURCE_DIR}/resources/kayte.icns")
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_ICON_FILE "${MACOS_BUNDLE_ICON_FILE}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.kd.kayteide"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist"
    )
    set_source_files_properties(resources/kayte.icns PROPERTIES XCODE_EXPLICIT_FILE_TYPE .icns)
endif()

# Link the necessary Qt libraries and pas2dox_lib to your executable
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

# Ensure KayteIDE can find the headers from pas2dox plugin
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/plugins/pas2dox # To find pasdoxflexlexer.h and globals.h
)


# Add compile definitions, typically for debug configurations
target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        $<$<CONFIG:Debug>:QT_QML_DEBUG> # Adds QT_QML_DEBUG for Debug builds
)

# Set the output directory for the executable (for desktop builds)
# For Android, output is handled by Android APK generation.
if (NOT ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# Sanitizers and debug flags (typically for desktop development)
# You might want to conditionalize these more strictly for release builds or specific platforms
if (NOT ANDROID) # Sanitizers are typically not used for Android release builds
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
endif()

# --- Custom Target for DMG Creation (macOS only, and not Android) ---
if (APPLE AND NOT ANDROID)
    # Define the path to your DMG creation script
    set(DMG_SCRIPT "${CMAKE_SOURCE_DIR}/create_dmg.sh")
    set(DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/resources/dmg_background.png") # Optional background image

    # Get the full path to the *built application bundle's containing directory*
    get_target_property(APP_BUNDLE_OUTPUT_DIR ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)

    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Running DMG creation script..."
            # Pass the directory containing the .app bundle as the first argument to the script
            COMMAND ${DMG_SCRIPT} "${APP_BUNDLE_OUTPUT_DIR}" "${DMG_BACKGROUND_IMAGE}"
            COMMENT "Creating macOS DMG installer"
            VERBATIM
    )
endif()

# --- CPack Configuration for Debian and RPM Packages ---
# Only configure for non-Android and Linux builds
if (NOT ANDROID AND UNIX AND NOT APPLE)

    # We will install the main executable to /usr/bin
    install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION bin)

    # Add an install command for a desktop file and icon
    install(FILES "${CMAKE_SOURCE_DIR}/resources/kayteide.desktop" DESTINATION "share/applications")
    install(FILES "${CMAKE_SOURCE_DIR}/resources/kayte.png" DESTINATION "share/icons")

    # --- Debian Package Configuration ---
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "kayteide")
    set(CPACK_PACKAGE_VERSION "0.1.0")
    set(CPACK_PACKAGE_DESCRIPTION "KayteIDE, a simple IDE for Pascal and C++")
    set(CPACK_PACKAGE_VENDOR "Pedro Dias Vicente")
    set(CPACK_PACKAGE_CONTACT "pdvicente@gleentech.com")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://www.ringscejs.gleentech.com")

    # --- RPM Package Configuration ---
    # Set CPack generator to include RPM
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_RPM_PACKAGE_NAME "kayteide")
    set(CPACK_RPM_PACKAGE_VERSION "0.1.0")
    set(CPACK_RPM_PACKAGE_SUMMARY "A simple IDE for Pascal and C++")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Tools") # Common RPM group for development tools
    set(CPACK_RPM_PACKAGE_VENDOR "Kreatyve Designs")
    set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
    set(CPACK_RPM_PACKAGE_URL "https://www.ringscejs.gleentech.com")

    # Include CPack
    include(CPack)
endif()