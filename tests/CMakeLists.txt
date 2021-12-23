################################################################################
### Project name
################################################################################
cmake_minimum_required (VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project("constexpr_type_erasure")

################################################################################
### Includes
################################################################################
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include/")

set(${INSTALL_BIN} ${CMAKE_CURRENT_BINARY_DIR}/../bin/)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options( -Wall -fexceptions 
                         -Wno-unknown-pragmas -Wno-unused-function -O3)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options( -Wall -fexceptions 
                         -Wno-unknown-pragmas -Wno-unused-function -O3)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Debug")
    string(REGEX REPLACE "/W[3|4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    add_compile_options(/WX- /permissive-
                        /TP /Gd /Gm- /diagnostics:classic
                        /fp:precise /EHsc)
    add_link_options(/INCREMENTAL /MACHINE:X64)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
    string(REGEX REPLACE "/W[3|4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    add_compile_options(/WX- /permissive-
                        /TP /Gd /Gm- /diagnostics:classic
                        /fp:precise /EHsc /Ob2 /Ox /Oi)
    add_link_options(/INCREMENTAL /MACHINE:X64)
endif()

add_subdirectory(constexpr_any)
add_subdirectory(constexpr_function)
