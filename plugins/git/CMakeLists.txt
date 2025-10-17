# This assumes you have already found the Git library with a find_package call
# For example:
# find_package(libgit2 REQUIRED)

add_library(gitclient_plugin MODULE src/GitClientPlugin.cpp src/GitClientPlugin.h)

# Make sure Qt finds the plugin interfaces
target_include_directories(gitclient_plugin
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR} # Include this plugin's headers
        # Add the path to the main project's source directory where IGitClient.h is located.
        ${CMAKE_SOURCE_DIR}/src     # <-- THIS IS THE MISSING LINE
)

# Link to the necessary libraries
target_link_libraries(gitclient_plugin
        PRIVATE
        Qt6::Core
        Qt6::Concurrent
        libgit2::libgit2 # Link to your Git library here
)

set_target_properties(gitclient_plugin PROPERTIES
        QT_PLUGIN_TYPE "gitclient"
        # This is a specific property for Qt plugins.
        # Set the plugin destination folder. This will be where your main app looks for plugins.
        BUNDLE_DESTINATION "plugins"
        LIBRARY_DESTINATION "plugins"
)

# This is important to ensure the plugin is copied to the correct location
install(TARGETS gitclient_plugin
        BUNDLE DESTINATION "${CMAKE_INSTALL_PREFIX}/plugins"
        LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/plugins"
)