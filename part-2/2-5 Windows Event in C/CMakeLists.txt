cmake_minimum_required(VERSION 3.21)

project(2-5-Windows-Event-in-C LANGUAGES CXX)

# Check for MSFS 2024 SDK environment variable
if(NOT DEFINED ENV{MSFS2024_SDK})
    message(FATAL_ERROR "MSFS2024_SDK environment variable not set")
endif()

# Create executable from local source
add_executable(2-5-Windows-Event-in-C main.cpp)

# Link to project options (includes SimConnect SDK paths and libraries)
# Don't link warnings target for external SDK code
target_link_libraries(2-5-Windows-Event-in-C
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
        cmake_cpp_simconnect::cmake_cpp_simconnect_warnings
)

# Set output directory
set_target_properties(2-5-Windows-Event-in-C PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
