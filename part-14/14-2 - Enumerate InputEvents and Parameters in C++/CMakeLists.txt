cmake_minimum_required(VERSION 3.21)

project(14-2-EnumerateInputEventsAndParameters-CXX LANGUAGES CXX)

add_executable(14-2-EnumerateInputEventsAndParameters-CXX main.cpp)
target_link_libraries(14-2-EnumerateInputEventsAndParameters-CXX
    PRIVATE cmake_cpp_simconnect::cmake_cpp_simconnect_options)
set_target_properties(14-2-EnumerateInputEventsAndParameters-CXX PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
