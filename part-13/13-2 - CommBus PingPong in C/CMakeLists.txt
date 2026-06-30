cmake_minimum_required(VERSION 3.21)

project(13-2-CommBusPingPong-C LANGUAGES CXX)

add_executable(13-2-CommBusPingPong-Ping-C ping.cpp)
add_executable(13-2-CommBusPingPong-Pong-C pong.cpp)

foreach(target IN ITEMS 13-2-CommBusPingPong-Ping-C 13-2-CommBusPingPong-Pong-C)
    target_link_libraries(${target}
        PRIVATE
            cmake_cpp_simconnect::cmake_cpp_simconnect_options
    )
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        CXX_CLANG_TIDY ""
    )
endforeach()
