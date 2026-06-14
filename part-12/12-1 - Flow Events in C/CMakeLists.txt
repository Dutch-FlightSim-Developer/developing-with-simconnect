cmake_minimum_required(VERSION 3.21)

project(12-1-Flow-Events-in-C LANGUAGES CXX)

if(NOT DEFINED ENV{MSFS2024_SDK})
    message(FATAL_ERROR "MSFS2024_SDK environment variable not set. Flow Events require the MSFS 2024 SDK.")
endif()

add_executable(12-1-Flow-Events-in-C main.cpp)
target_link_libraries(12-1-Flow-Events-in-C
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
)
set_target_properties(12-1-Flow-Events-in-C PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    CXX_CLANG_TIDY ""
)
