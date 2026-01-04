cmake_minimum_required(VERSION 3.21)

project(8-1-Receiving-events-C LANGUAGES CXX)

# Check for MSFS 2024 SDK environment variable
if(NOT DEFINED ENV{MSFS2024_SDK})
    message(FATAL_ERROR "MSFS2024_SDK environment variable not set")
endif()

# Create executable from SDK sample
add_executable(8-1-Receiving-events-C main.cpp)

# Link to project options (includes SimConnect SDK paths and libraries)
target_link_libraries(8-1-Receiving-events-C
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
)

# Set output directory and disable clang-tidy for SDK sample code
set_target_properties(8-1-Receiving-events-C PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    CXX_CLANG_TIDY ""
)
