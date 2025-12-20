cmake_minimum_required(VERSION 3.21)

project(7-1-Flight-recorder LANGUAGES CXX)

# Check for MSFS 2024 SDK environment variable
if(NOT DEFINED ENV{MSFS2024_SDK})
    message(FATAL_ERROR "MSFS2024_SDK environment variable not set")
endif()

# Create executable from SDK sample
add_executable(7-1-Flight-recorder main.cpp)

# Link to project options (includes SimConnect SDK paths and libraries)
target_link_libraries(7-1-Flight-recorder
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
)

# Set output directory and disable clang-tidy for SDK sample code
set_target_properties(7-1-Flight-recorder PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    CXX_CLANG_TIDY ""
)
