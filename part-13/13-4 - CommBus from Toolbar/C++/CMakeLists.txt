cmake_minimum_required(VERSION 3.21)

project(13-4-CommBusFromToolbar-Cpp LANGUAGES CXX)

# Create executable from local source
add_executable(13-4-CommBusFromToolbar
    main.cpp
)

# Link to project options (includes SimConnect SDK paths and libraries)
target_link_libraries(13-4-CommBusFromToolbar
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
        cmake_cpp_simconnect::cmake_cpp_simconnect_warnings
)

# Set output directory
set_target_properties(13-4-CommBusFromToolbar PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
