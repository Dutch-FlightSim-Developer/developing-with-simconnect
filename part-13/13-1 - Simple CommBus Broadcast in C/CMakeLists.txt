cmake_minimum_required(VERSION 3.21)

project(13-1-SimpleCommBusBroadcast-C LANGUAGES CXX)

add_executable(13-1-SimpleCommBusBroadcast-Caller-C   caller.cpp)
add_executable(13-1-SimpleCommBusBroadcast-Listener-C listener.cpp)

foreach(target IN ITEMS 13-1-SimpleCommBusBroadcast-Caller-C 13-1-SimpleCommBusBroadcast-Listener-C)
    target_link_libraries(${target}
        PRIVATE
            cmake_cpp_simconnect::cmake_cpp_simconnect_options
    )
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        CXX_CLANG_TIDY ""
    )
endforeach()
