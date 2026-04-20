cmake_minimum_required(VERSION 3.21)

project(11-1-SimpleClientData-C LANGUAGES CXX)

add_executable(11-1-SimpleClientData-Sender-C   sender.cpp)
add_executable(11-1-SimpleClientData-Receiver-C receiver.cpp)

foreach(target IN ITEMS 11-1-SimpleClientData-Sender-C 11-1-SimpleClientData-Receiver-C)
    target_link_libraries(${target}
        PRIVATE
            cmake_cpp_simconnect::cmake_cpp_simconnect_options
    )
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        CXX_CLANG_TIDY ""
    )
endforeach()
