cmake_minimum_required(VERSION 3.21)

project(1-2-OpenClose-Cpp LANGUAGES CXX)

# Create executable from local source
add_executable(1-2-OpenClose-Cpp
    OpenClose.cpp
)

# Link to project options (includes SimConnect SDK paths and libraries)
target_link_libraries(1-2-OpenClose-Cpp
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
        cmake_cpp_simconnect::cmake_cpp_simconnect_warnings
)

# Set output directory
set_target_properties(1-2-OpenClose-Cpp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
