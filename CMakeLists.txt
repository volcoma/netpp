﻿cmake_minimum_required(VERSION 3.4.3)

project(netpp)
include(CTest)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/utility")

option(BUILD_NETPP_SHARED "Build as a shared library." ON)
option(BUILD_NETPP_TESTS "Build the tests" OFF)
option(BUILD_NETPP_WITH_CODE_STYLE_CHECKS "Build with code style checks." OFF)

if(BUILD_NETPP_TESTS)
	if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
	endif()
	if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
		set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
	endif()
	if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
	endif()
endif()

if(BUILD_NETPP_SHARED)
	set(BUILD_SHARED_LIBS ON)
endif()

if(BUILD_NETPP_WITH_CODE_STYLE_CHECKS)
	set(BUILD_WITH_CODE_STYLE_CHECKS ON)
endif()

add_subdirectory(3rdparty)
add_subdirectory(netpp)

if(BUILD_NETPP_TESTS)
	add_subdirectory(tests)
endif()
