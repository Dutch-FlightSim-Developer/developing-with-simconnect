cmake_minimum_required(VERSION 3.21)

project(11-6-PMDG-ELEC-Dump-C LANGUAGES CXX)

if(NOT PMDG_NG3_SDK_PATH)
    message(STATUS "PMDG_NG3_SDK_PATH not set — skipping '${PROJECT_NAME}'")
    return()
endif()

add_executable(11-6-PMDG-ELEC-Dump-C elec_dump.cpp)

target_include_directories(11-6-PMDG-ELEC-Dump-C
    PRIVATE "${PMDG_NG3_SDK_PATH}"
)

target_link_libraries(11-6-PMDG-ELEC-Dump-C
    PRIVATE cmake_cpp_simconnect::cmake_cpp_simconnect_options
)

set_target_properties(11-6-PMDG-ELEC-Dump-C PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    CXX_CLANG_TIDY ""
)
