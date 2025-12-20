cmake_minimum_required(VERSION 3.21)

project(2-10-SubscribeToSystemEvent-in-C LANGUAGES CXX)

# Check for MSFS 2024 SDK environment variable
if(NOT DEFINED ENV{MSFS2024_SDK})
    message(FATAL_ERROR "MSFS2024_SDK environment variable not set")
endif()

# Create executable from local source
add_executable(2-10-SubscribeToSystemEvent-in-C main.cpp)

target_compile_options(2-10-SubscribeToSystemEvent-in-C PRIVATE
    -Wno-missing-declarations
    -Wno-error  # Disable warnings-as-errors
)

# Link to project options (includes SimConnect SDK paths and libraries)
target_link_libraries(2-10-SubscribeToSystemEvent-in-C
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
        cmake_cpp_simconnect::cmake_cpp_simconnect_warnings
)

# Set output directory
set_target_properties(2-10-SubscribeToSystemEvent-in-C PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
