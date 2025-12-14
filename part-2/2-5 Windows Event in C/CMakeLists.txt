cmake_minimum_required(VERSION 3.21)

project(2-5-Windows-Event-in-C LANGUAGES CXX)

set(SOURCE_FILE "$ENV{MSFS2024_SDK}/Samples/VisualStudio/SimConnectSamples/WindowsEvent/WindowsEvent.cpp")

if(NOT EXISTS "${SOURCE_FILE}")
    message(FATAL_ERROR "Source file not found at: ${SOURCE_FILE}")
endif()

# Create executable from local source
add_executable(2-5-Windows-Event-in-C "${SOURCE_FILE}")

# Link to project options (includes SimConnect SDK paths and libraries)
# Don't link warnings target for external SDK code
target_link_libraries(2-5-Windows-Event-in-C
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
)

# Set output directory
set_target_properties(2-5-Windows-Event-in-C PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    CXX_CLANG_TIDY ""
)
