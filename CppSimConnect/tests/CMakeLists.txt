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
  DataBlockTests.cpp
  DataBlockReaderTests.cpp
  DataBlockBuilderTests.cpp
  TestDataDefinition.cpp
  TestDataDefinition_Float32.cpp
  TestDataDefinition_Float64.cpp
  TestDataDefinition_Int32.cpp
  TestDataDefinition_Int64.cpp
  TestDataDefinition_TestString.cpp
  TestDataFrequency.cpp
  TestLogger.cpp
  TestPeriodLimits.cpp
  TestSimObjectType.cpp
  TestSimpleHandler.cpp
)

# Create test executable
add_executable(unit_tests ${TEST_SOURCES})

target_link_libraries(
  unit_tests
  PRIVATE 
    cmake_cpp_simconnect::cmake_cpp_simconnect_warnings
    cmake_cpp_simconnect::cmake_cpp_simconnect_options
    GTest::gtest
    GTest::gtest_main
)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(
    TARGET unit_tests
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:unit_tests> $<TARGET_FILE_DIR:unit_tests>
    COMMAND_EXPAND_LISTS)
endif()

# Automatically discover tests
gtest_discover_tests(
  unit_tests
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTIES 
    LABELS "unit"
)
