cmake_minimum_required(VERSION 3.21)

project(1-1-OpenClose-C LANGUAGES CXX)

# Check for MSFS 2024 SDK environment variable
if(NOT DEFINED ENV{MSFS2024_SDK})
    message(FATAL_ERROR "MSFS2024_SDK environment variable not set")
endif()

set(SOURCE_FILE "$ENV{MSFS2024_SDK}/Samples/VisualStudio/SimConnectSamples/OpenAndClose/OpenClose.cpp")

if(NOT EXISTS "${SOURCE_FILE}")
    message(FATAL_ERROR "Source file not found at: ${SOURCE_FILE}")
endif()

# Create executable from SDK sample
add_executable(1-1-OpenClose-C "${SOURCE_FILE}")

# Link to project options (includes SimConnect SDK paths and libraries)
target_link_libraries(1-1-OpenClose-C
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
)

# Set output directory and disable clang-tidy for SDK sample code
set_target_properties(1-1-OpenClose-C PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    CXX_CLANG_TIDY ""
)
