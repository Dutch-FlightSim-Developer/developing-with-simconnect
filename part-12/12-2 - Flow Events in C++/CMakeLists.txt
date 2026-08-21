cmake_minimum_required(VERSION 3.21)

project(12-2-Flow-Events-in-Cpp LANGUAGES CXX)

add_executable(12-2-Flow-Events-in-Cpp main.cpp)
target_link_libraries(12-2-Flow-Events-in-Cpp
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
)
set_target_properties(12-2-Flow-Events-in-Cpp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
