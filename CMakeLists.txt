cmake_minimum_required(VERSION 3.4.3)

project (netpp)

option(BUILD_NETPP_SHARED "Build as a shared library." ON)
option(BUILD_NETPP_TESTS "Build the tests" ON)

if(BUILD_NETPP_TESTS)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()
if(BUILD_NETPP_SHARED)
	set(BUILD_SHARED_LIBS ON)
endif()

if(WIN32)
	set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/asio/asio")
endif()

add_subdirectory(3rdparty)
add_subdirectory(netpp)

if(BUILD_NETPP_TESTS)
	add_subdirectory(tests)
endif()
