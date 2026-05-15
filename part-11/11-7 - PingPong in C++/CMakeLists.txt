cmake_minimum_required(VERSION 3.21)

project(11-7-PingPong-CPP LANGUAGES CXX)

add_executable(11-7-PingPong-Ping-CPP ping.cpp)
target_link_libraries(11-7-PingPong-Ping-CPP
    PRIVATE cmake_cpp_simconnect::cmake_cpp_simconnect_options)
set_target_properties(11-7-PingPong-Ping-CPP PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(11-7-PingPong-Pong-CPP pong.cpp)
target_link_libraries(11-7-PingPong-Pong-CPP
    PRIVATE cmake_cpp_simconnect::cmake_cpp_simconnect_options)
set_target_properties(11-7-PingPong-Pong-CPP PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
