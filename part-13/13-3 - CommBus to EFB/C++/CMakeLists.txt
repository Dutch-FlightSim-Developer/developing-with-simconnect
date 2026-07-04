cmake_minimum_required(VERSION 3.21)

project(13-3-CommBusToEFB-Cpp LANGUAGES CXX)

# Create executable from local source
add_executable(13-3-CommBusToEFB-Sender
    sender.cpp
)

# Link to project options (includes SimConnect SDK paths and libraries)
target_link_libraries(13-3-CommBusToEFB-Sender
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
        cmake_cpp_simconnect::cmake_cpp_simconnect_warnings
)

# Set output directory
set_target_properties(13-3-CommBusToEFB-Sender PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
