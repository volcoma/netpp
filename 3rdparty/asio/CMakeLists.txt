set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/asio/asio")
# Find ASIO (the standalone version) If asio isn't installed in
# a standard directory, add the correct directory to CMAKE_PREFIX_PATH
find_path(ASIO_DIR NAMES asio.hpp)
# Hide this variable from CMake GUIs and `cmake -L`
set_property(CACHE ASIO_DIR PROPERTY ADVANCED TRUE)
if (NOT ASIO_DIR)
    message(FATAL_ERROR
"
ASIO header (asio.hpp) could not be found.
ASIO is used for networking.
On Linux, you should install it using your package manager.
On Windows, please add the correct directory to CMAKE_PREFIX_PATH
e.g set(CMAKE_PREFIX_PATH \$\{CMAKE_PREFIX_PATH} \$\{CMAKE_CURRENT_SOURCE_DIR}/3rdparty/asio/asio)")
endif()

message(STATUS ${ASIO_DIR})
add_library(asio INTERFACE)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(asio INTERFACE Threads::Threads)

# Ubuntu 14.04 packages asio 1.10.1 and clang 3.4.1.
# In this environment, asio stand alone (set ASIO_STANDALONE)
# does not correctly detect that the stl has CHRONO support (c++11).
# Force the use of std::chrono by setting ASIO_HAS_STD_CHRONO
target_include_directories(asio SYSTEM INTERFACE "${ASIO_DIR}")
if(WIN32)
	target_link_libraries(asio INTERFACE wsock32 ws2_32)

	macro(get_WIN32_WINNT version)
		if (CMAKE_SYSTEM_VERSION)
			set(ver ${CMAKE_SYSTEM_VERSION})
			string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
			string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
			# Check for Windows 10, b/c we'll need to convert to hex 'A'.
			if ("${verMajor}" MATCHES "10")
				set(verMajor "A")
				string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
			endif ("${verMajor}" MATCHES "10")
			# Remove all remaining '.' characters.
			string(REPLACE "." "" ver ${ver})
			# Prepend each digit with a zero.
			string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
			set(${version} "0x${ver}")
		endif(CMAKE_SYSTEM_VERSION)
	endmacro(get_WIN32_WINNT)
	get_WIN32_WINNT(ver)

	target_compile_definitions(asio INTERFACE _WIN32_WINNT=${ver})
endif()


if(BUILD_SHARED_LIBS)
	set(OPENSSL_USE_STATIC_LIBS FALSE)
endif()

find_package(OpenSSL 1.1.0 REQUIRED)

if (WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(CRYPTO_LIB_NAME libcrypto-1_1-x64.dll)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(CRYPTO_LIB_NAME libcrypto-1_1.dll)
	endif()

  find_file(DLL_LIBEAY32
    NAMES ${CRYPTO_LIB_NAME}
    HINTS ${_OPENSSL_ROOT_HINTS} PATHS ${_OPENSSL_ROOT_PATHS}
    PATH_SUFFIXES bin)
  if (NOT DLL_LIBEAY32)
    message(WARNING
      "Unable to find OpenSSL crypto (aka libeay32) DLL, executables may not run")
  endif()

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	  set(SSL_LIB_NAME libssl-1_1-x64.dll)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	  set(SSL_LIB_NAME libssl-1_1.dll)
  endif()
  find_file(DLL_SSLEAY32
    NAMES ${SSL_LIB_NAME}
    HINTS ${_OPENSSL_ROOT_HINTS} PATHS ${_OPENSSL_ROOT_PATHS}
    PATH_SUFFIXES bin)
  if (NOT DLL_SSLEAY32)
    message(WARNING
      "Unable to find OpenSSL ssl (aka ssleay32) DLL, executables may not run")
  endif()

  if(DLL_LIBEAY32 AND DLL_SSLEAY32 AND CMAKE_RUNTIME_OUTPUT_DIRECTORY)
	  file(COPY ${DLL_LIBEAY32} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	  file(COPY ${DLL_SSLEAY32} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	  #message(STATUS "copying ... ${DLL_LIBEAY32}")
	  #message(STATUS "copying ... ${DLL_SSLEAY32}")

  endif()
endif()

target_link_libraries(asio INTERFACE OpenSSL::SSL OpenSSL::Crypto)

target_compile_definitions(asio
                           INTERFACE ASIO_STANDALONE
                           INTERFACE ASIO_HAS_STD_CHRONO
                           INTERFACE ASIO_HAS_STD_THREAD
                           INTERFACE ASIO_HAS_MOVE)
