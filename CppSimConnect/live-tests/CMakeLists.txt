cmake_minimum_required(VERSION 3.15...3.23)

project(CmakeConfigPackageTests LANGUAGES CXX)

# ---- Test as standalone project the exported config package ----

if(PROJECT_IS_TOP_LEVEL OR TEST_INSTALLED_VERSION)
  enable_testing()

  find_package(cmake_cpp_simconnect CONFIG REQUIRED) # for intro, project_options, ...

  if(NOT TARGET cmake_cpp_simconnect_options)
    message(FATAL_ERROR "Required config package not found!")
    return() # be strictly paranoid for Template Janitor github action! CK
  endif()
endif()

# GTest integration
include(GoogleTest)

# Collect all test files
file(GLOB TEST_SOURCES 
  TestBackgroundHandler.cpp
  TestConnection.cpp
  TestEventHandler.cpp
  TestEvents.cpp
  TestMultiClient.cpp
  TestNotificationGroups.cpp
  TestSystemEvents.cpp
  TestSystemState.cpp
)

# Create test executable
add_executable(live_tests ${TEST_SOURCES})

target_link_libraries(
  live_tests
  PRIVATE 
    cmake_cpp_simconnect::cmake_cpp_simconnect_warnings
    cmake_cpp_simconnect::cmake_cpp_simconnect_options
    GTest::gtest
    GTest::gtest_main
)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(
    TARGET live_tests
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:live_tests> $<TARGET_FILE_DIR:live_tests>
    COMMAND_EXPAND_LISTS)
endif()

# Automatically discover tests
gtest_discover_tests(
  live_tests
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTIES 
    LABELS "unit"
)
