cmake_minimum_required(VERSION 3.21)

project(14-1-EnumerateInputEventsAndParameters-C LANGUAGES CXX)

add_executable(14-1-EnumerateInputEventsAndParameters-C main.cpp)
target_link_libraries(14-1-EnumerateInputEventsAndParameters-C
    PRIVATE cmake_cpp_simconnect::cmake_cpp_simconnect_options)
set_target_properties(14-1-EnumerateInputEventsAndParameters-C PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    CXX_CLANG_TIDY "")
