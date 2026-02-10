cmake_minimum_required(VERSION 3.21)

project(10-1-MSFS2024-RequestJetwayData LANGUAGES CXX)

# Check for MSFS 2024 SDK environment variable
if(NOT DEFINED ENV{MSFS2024_SDK})
    message(FATAL_ERROR "MSFS2024_SDK environment variable not set")
endif()

# Create executable from SDK sample
add_executable(10-1-MSFS2024-RequestJetwayData main.cpp)
# Link to project options (includes SimConnect SDK paths and libraries)
target_link_libraries(10-1-MSFS2024-RequestJetwayData
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
        cmake_cpp_simconnect::cmake_cpp_simconnect_warnings
)

# Set output directory and disable clang-tidy for SDK sample code
set_target_properties(10-1-MSFS2024-RequestJetwayData PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
