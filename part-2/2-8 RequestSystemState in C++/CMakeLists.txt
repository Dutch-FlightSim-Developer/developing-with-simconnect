cmake_minimum_required(VERSION 3.21)

project(2-8-RequestSystemState-in-C++ LANGUAGES CXX)

# Create executable from local source
add_executable(2-8-RequestSystemState-in-C++
    SystemState.cpp
)

# Link to project options (includes SimConnect SDK paths and libraries)
target_link_libraries(2-8-RequestSystemState-in-C++
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
        cmake_cpp_simconnect::cmake_cpp_simconnect_warnings
)

# Set output directory
set_target_properties(2-8-RequestSystemState-in-C++ PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
