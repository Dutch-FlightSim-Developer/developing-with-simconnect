cmake_minimum_required(VERSION 3.21)

project(14-3-GetAndSetInputEventValue-CXX LANGUAGES CXX)

add_executable(14-3-GetAndSetInputEventValue-CXX main.cpp)
target_link_libraries(14-3-GetAndSetInputEventValue-CXX
    PRIVATE cmake_cpp_simconnect::cmake_cpp_simconnect_options)
set_target_properties(14-3-GetAndSetInputEventValue-CXX PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
