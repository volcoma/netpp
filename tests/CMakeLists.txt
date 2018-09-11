message(STATUS "Enabled tests.")

set(target_name netpp_tests)

file(GLOB_RECURSE libsrc *.h *.cpp *.hpp *.c *.cc)

add_executable(${target_name} ${libsrc})

target_link_libraries(${target_name} PUBLIC netpp)

set_target_properties(${target_name} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_compile_definitions(${target_name} PUBLIC CERT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../certificates/")
