cmake_minimum_required(VERSION 3.21)

# This template attempts to be "fetch_content"-able
# so that it works well with tools like CPM or other
# manual dependency management

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the project name and language
project(
  cmake_cpp_simconnect
  VERSION 0.0.1
  DESCRIPTION ""
  HOMEPAGE_URL "https://github.com/Dutch-FlightSim-Developer/developing-with-simconnect"
  LANGUAGES CXX C)

include(cmake/PreventInSourceBuilds.cmake)
include(ProjectOptions.cmake)


# ---- SimConnect SDK Selection ----
# Option to choose between MSFS 2020 and MSFS 2024 SDK
set(MSFS_SDK_VERSION "2024" CACHE STRING "Microsoft Flight Simulator SDK version (2020 or 2024)")
set_property(CACHE MSFS_SDK_VERSION PROPERTY STRINGS "2020" "2024")

# ---- SimConnect SDK Path ----
if(MSFS_SDK_VERSION STREQUAL "2020")
  # Check for MSFS 2020 SDK environment variable
  if(DEFINED ENV{MSFS2020_SDK})
    set(SIMCONNECT_SDK_PATH "$ENV{MSFS2020_SDK}/SimConnect SDK")
    message(STATUS "Using MSFS 2020 SDK from: ${SIMCONNECT_SDK_PATH}")
  elseif(DEFINED ENV{MSFS_SDK})
    set(SIMCONNECT_SDK_PATH "$ENV{MSFS_SDK}/SimConnect SDK")
    message(STATUS "Using MSFS 2020 SDK from: ${SIMCONNECT_SDK_PATH}")
  else()
    message(FATAL_ERROR "MSFS2020_SDK or MSFS_SDK environment variable not set. Please set it to your MSFS 2020 SDK installation path.")
  endif()
elseif(MSFS_SDK_VERSION STREQUAL "2024")
  # Check for MSFS 2024 SDK environment variable
  if(DEFINED ENV{MSFS2024_SDK})
    set(SIMCONNECT_SDK_PATH "$ENV{MSFS2024_SDK}/SimConnect SDK")
    message(STATUS "Using MSFS 2024 SDK from: ${SIMCONNECT_SDK_PATH}")
  else()
    message(FATAL_ERROR "MSFS2024_SDK environment variable not set. Please set it to your MSFS 2024 SDK installation path.")
  endif()
else()
  message(FATAL_ERROR "Invalid MSFS_SDK_VERSION: ${MSFS_SDK_VERSION}. Must be either '2020' or '2024'.")
endif()

# Verify the path exists
if(NOT EXISTS "${SIMCONNECT_SDK_PATH}/include")
  message(FATAL_ERROR "SimConnect SDK include directory not found at: ${SIMCONNECT_SDK_PATH}/include")
endif()

# Find SimConnect library
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SIMCONNECT_LIB_PATH "${SIMCONNECT_SDK_PATH}/lib/static/SimConnect_debug.lib")
  else()
    set(SIMCONNECT_LIB_PATH "${SIMCONNECT_SDK_PATH}/lib/static/SimConnect.lib")
  endif()
else()
  message(FATAL_ERROR "32-bit builds are not supported")
endif()

if(NOT EXISTS "${SIMCONNECT_LIB_PATH}")
  message(FATAL_ERROR "SimConnect.lib not found at: ${SIMCONNECT_LIB_PATH}")
endif()

message(STATUS "Using SimConnect library: ${SIMCONNECT_LIB_PATH}")

cmake_cpp_simconnect_setup_options()

cmake_cpp_simconnect_global_options()
include(Dependencies.cmake)
cmake_cpp_simconnect_setup_dependencies()

cmake_cpp_simconnect_local_options()

# don't know if this should be set globally from here or not...
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(GIT_SHA
    "Unknown"
    CACHE STRING "SHA this build was generated from")
string(
  SUBSTRING "${GIT_SHA}"
            0
            8
            GIT_SHORT_SHA)

target_compile_features(cmake_cpp_simconnect_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

# Suppress Microsoft CRT security warnings (clang-tidy will catch genuinely unsafe code)
if(MSVC OR MSVC_IDE)
  target_compile_definitions(cmake_cpp_simconnect_options INTERFACE _CRT_SECURE_NO_WARNINGS)
endif()

add_library(cmake_cpp_simconnect::cmake_cpp_simconnect_options ALIAS cmake_cpp_simconnect_options)
add_library(cmake_cpp_simconnect::cmake_cpp_simconnect_warnings ALIAS cmake_cpp_simconnect_warnings)

# Add global include directories for all targets
target_include_directories(cmake_cpp_simconnect_options INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${SIMCONNECT_SDK_PATH}/include>
  $<INSTALL_INTERFACE:include>
)

# Link SimConnect library for all targets
target_link_libraries(cmake_cpp_simconnect_options INTERFACE
  "${SIMCONNECT_LIB_PATH}" "shlwapi.lib" "user32.lib" "Ws2_32.lib"
)

# Adding the src:
add_subdirectory(CppSimConnect)
add_subdirectory(part-1)
add_subdirectory(part-2)
add_subdirectory(part-3)
add_subdirectory(part-4)
add_subdirectory(part-5)
add_subdirectory(part-6)
add_subdirectory(part-7)
add_subdirectory(part-8)
add_subdirectory(part-9)


# Don't even look at tests if we're not top level
if(NOT PROJECT_IS_TOP_LEVEL)
  return()
endif()


# If MSVC is being used, and ASAN is enabled, we need to set the debugger environment
# so that it behaves well with MSVC's debugger, and we can run the target from visual studio
if(MSVC)
  get_all_installable_targets(all_targets)
  message("all_targets=${all_targets}")
  set_target_properties(${all_targets} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=$(VC_ExecutablePath_x64);%PATH%")
endif()

# set the startup project for the "play" button in MSVC
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT intro)

if(CMAKE_SKIP_INSTALL_RULES)
  return()
endif()

include(cmake/PackageProject.cmake)

# Add other targets that you want installed here, by default we just package the one executable
# we know we want to ship
cmake_cpp_simconnect_package_project(
  TARGETS
  cmake_cpp_simconnect_options
  cmake_cpp_simconnect_warnings
  # FIXME: this does not work! CK
  # PRIVATE_DEPENDENCIES_CONFIGURED project_options project_warnings
)

# Experience shows that explicit package naming can help make it easier to sort
# out potential ABI related issues before they start, while helping you
# track a build to a specific GIT SHA
set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)
