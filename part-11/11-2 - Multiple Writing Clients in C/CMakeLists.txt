cmake_minimum_required(VERSION 3.21)

project(11-2-MultipleWritingClients-C LANGUAGES CXX)

add_executable(11-2-PingPong-Ping-C ping.cpp)
add_executable(11-2-PingPong-Pong-C pong.cpp)

foreach(target IN ITEMS 11-2-PingPong-Ping-C 11-2-PingPong-Pong-C)
    target_link_libraries(${target}
        PRIVATE
            cmake_cpp_simconnect::cmake_cpp_simconnect_options
    )
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        CXX_CLANG_TIDY ""
    )
endforeach()
