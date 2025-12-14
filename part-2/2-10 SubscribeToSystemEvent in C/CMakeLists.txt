cmake_minimum_required(VERSION 3.21)

project(2-10-SubscribeToSystemEvent-in-C LANGUAGES CXX)

set(SOURCE_FILE "$ENV{MSFS2024_SDK}/Samples/VisualStudio/SimConnectSamples/SystemEvent/SystemEvent.cpp")

if(NOT EXISTS "${SOURCE_FILE}")
    message(FATAL_ERROR "Source file not found at: ${SOURCE_FILE}")
endif()

# Create executable from local source
add_executable(2-10-SubscribeToSystemEvent-in-C "${SOURCE_FILE}")

target_compile_options(2-10-SubscribeToSystemEvent-in-C PRIVATE
    -Wno-missing-declarations
    -Wno-error  # Disable warnings-as-errors
)

# Link to project options (includes SimConnect SDK paths and libraries)
target_link_libraries(2-10-SubscribeToSystemEvent-in-C
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
)

# Set output directory and disable clang-tidy for SDK sample code
set_target_properties(2-10-SubscribeToSystemEvent-in-C PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    CXX_CLANG_TIDY ""
)
