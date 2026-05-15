cmake_minimum_required(VERSION 3.21)

project(11-6-Custom-ClientData-CPP LANGUAGES CXX)

add_executable(11-6-CustomClientData-Sender-CPP sender.cpp)
target_link_libraries(11-6-CustomClientData-Sender-CPP
    PRIVATE cmake_cpp_simconnect::cmake_cpp_simconnect_options)
set_target_properties(11-6-CustomClientData-Sender-CPP PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(11-6-CustomClientData-Receiver-CPP receiver.cpp)
target_link_libraries(11-6-CustomClientData-Receiver-CPP
    PRIVATE cmake_cpp_simconnect::cmake_cpp_simconnect_options)
set_target_properties(11-6-CustomClientData-Receiver-CPP PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
