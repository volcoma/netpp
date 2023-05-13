cmake_minimum_required(VERSION 3.4.3)

set(LIB_NAME netpp)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
                          "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
                          "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
                          "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
add_library(${LIB_NAME} ${SOURCES})

# Add definitions
set_target_properties(${LIB_NAME} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
