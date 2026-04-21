cmake_minimum_required(VERSION 3.21)

project(11-3-PMDG-MCP-Monitor-C LANGUAGES CXX)

# Requires PMDG_NG3_SDK_PATH to be set in the top-level CMakeLists.txt.
# If not set this target is silently skipped.
if(NOT PMDG_NG3_SDK_PATH)
    message(STATUS "PMDG_NG3_SDK_PATH not set — skipping '${PROJECT_NAME}'")
    return()
endif()

message(STATUS "Using PMDG NG3 SDK from: ${PMDG_NG3_SDK_PATH}")

add_executable(11-3-PMDG-MCP-Monitor-C mcp_monitor.cpp)

target_include_directories(11-3-PMDG-MCP-Monitor-C
    PRIVATE
        "${PMDG_NG3_SDK_PATH}"
)

target_link_libraries(11-3-PMDG-MCP-Monitor-C
    PRIVATE
        cmake_cpp_simconnect::cmake_cpp_simconnect_options
)

set_target_properties(11-3-PMDG-MCP-Monitor-C PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    CXX_CLANG_TIDY ""
)
