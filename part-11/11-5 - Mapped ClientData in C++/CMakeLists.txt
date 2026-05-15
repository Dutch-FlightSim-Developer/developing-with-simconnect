cmake_minimum_required(VERSION 3.21)

project(11-5-Mapped-ClientData-CPP LANGUAGES CXX)

add_executable(11-5-MappedClientData-Sender-CPP sender.cpp)
target_link_libraries(11-5-MappedClientData-Sender-CPP
    PRIVATE cmake_cpp_simconnect::cmake_cpp_simconnect_options)
set_target_properties(11-5-MappedClientData-Sender-CPP PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(11-5-MappedClientData-Receiver-CPP receiver.cpp)
target_link_libraries(11-5-MappedClientData-Receiver-CPP
    PRIVATE cmake_cpp_simconnect::cmake_cpp_simconnect_options)
set_target_properties(11-5-MappedClientData-Receiver-CPP PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
