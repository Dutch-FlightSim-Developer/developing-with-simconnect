cmake_minimum_required(VERSION 3.21)

project(11-5-PMDG-Electrical-Power-Up-CPP LANGUAGES CXX)

# Requires PMDG_NG3_SDK_PATH to be set in the top-level CMakeLists.txt.
# If not set this target is silently skipped.
if(NOT PMDG_NG3_SDK_PATH)
    message(STATUS "PMDG_NG3_SDK_PATH not set — skipping '${PROJECT_NAME}'")
    return()
endif()

message(STATUS "Using PMDG NG3 SDK from: ${PMDG_NG3_SDK_PATH}")

add_executable(11-5-PMDG-Electrical-Power-Up-CPP electrical_power_up.cpp)

target_include_directories(11-5-PMDG-Electrical-Power-Up-CPP
    PRIVATE
        "${PMDG_NG3_SDK_PATH}"
)

target_link_libraries(11-5-PMDG-Electrical-Power-Up-CPP
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
)

set_target_properties(11-5-PMDG-Electrical-Power-Up-CPP PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    CXX_CLANG_TIDY ""
)
