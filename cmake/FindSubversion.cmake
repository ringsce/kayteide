# FindSubversion.cmake
# Finds the Apache Subversion client libraries (libsvn_client, libsvn_wc, etc.)
#
# This module defines:
#   Subversion_FOUND        - True if SVN libraries were found
#   Subversion_INCLUDE_DIRS - Include directories
#   Subversion_LIBRARIES    - Libraries to link against
#   Subversion_VERSION      - Version string (if detectable)
#   Subversion_SVN_EXECUTABLE - Path to the svn command-line tool
#
# Imported targets:
#   Subversion::Client      - svn_client library
#   Subversion::Wc          - svn_wc library

cmake_minimum_required(VERSION 3.16)

# --- Find the command-line SVN executable (optional, used as fallback) ---
find_program(Subversion_SVN_EXECUTABLE
    NAMES svn svn.exe
    DOC "Subversion command-line client"
)

# --- Find include directory ---
find_path(Subversion_INCLUDE_DIR
    NAMES svn_client.h
    PATHS
        /usr/include/subversion-1
        /usr/local/include/subversion-1
        /opt/homebrew/include/subversion-1
        /usr/include
        /usr/local/include
        "C:/Program Files/CollabNet/Subversion Client/include"
        "C:/svn/include"
    PATH_SUFFIXES subversion-1
    DOC "Subversion include directory"
)

# --- Find individual libraries ---
foreach(_lib svn_client svn_wc svn_ra svn_delta svn_subr apr-1 aprutil-1)
    string(TOUPPER ${_lib} _lib_upper)
    string(REPLACE "-" "_" _lib_upper "${_lib_upper}")
    find_library(Subversion_${_lib_upper}_LIBRARY
        NAMES ${_lib} lib${_lib}
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/homebrew/lib
            /usr/lib/x86_64-linux-gnu
            "C:/svn/lib"
        DOC "Subversion ${_lib} library"
    )
    list(APPEND _svn_libs Subversion_${_lib_upper}_LIBRARY)
endforeach()

# Collect found libraries
set(Subversion_LIBRARIES)
foreach(_var ${_svn_libs})
    if(${_var})
        list(APPEND Subversion_LIBRARIES ${${_var}})
    endif()
endforeach()

set(Subversion_INCLUDE_DIRS ${Subversion_INCLUDE_DIR})

# --- Version detection via svn --version ---
if(Subversion_SVN_EXECUTABLE)
    execute_process(
        COMMAND ${Subversion_SVN_EXECUTABLE} --version --quiet
        OUTPUT_VARIABLE _svn_ver_out
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(_svn_ver_out MATCHES "^([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(Subversion_VERSION "${CMAKE_MATCH_1}")
    endif()
endif()

# --- Handle REQUIRED / QUIET ---
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Subversion
    REQUIRED_VARS Subversion_INCLUDE_DIR Subversion_SVN_CLIENT_LIBRARY
    VERSION_VAR Subversion_VERSION
)

# --- Create imported targets ---
if(Subversion_FOUND)
    if(NOT TARGET Subversion::Client)
        add_library(Subversion::Client UNKNOWN IMPORTED)
        set_target_properties(Subversion::Client PROPERTIES
            IMPORTED_LOCATION             "${Subversion_SVN_CLIENT_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Subversion_INCLUDE_DIRS}"
        )
        if(Subversion_SVN_WC_LIBRARY)
            set_target_properties(Subversion::Client PROPERTIES
                INTERFACE_LINK_LIBRARIES  "${Subversion_SVN_WC_LIBRARY}"
            )
        endif()
    endif()
endif()

mark_as_advanced(
    Subversion_INCLUDE_DIR
    Subversion_SVN_CLIENT_LIBRARY
    Subversion_SVN_WC_LIBRARY
    Subversion_SVN_RA_LIBRARY
    Subversion_SVN_DELTA_LIBRARY
    Subversion_SVN_SUBR_LIBRARY
    Subversion_APR_1_LIBRARY
    Subversion_APRUTIL_1_LIBRARY
)
