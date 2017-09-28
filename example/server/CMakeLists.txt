cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_VERBOSE_MAKEFILE on)
SET(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(MYFLAG "-g -D__STDC_FORMAT_MACROS")
add_definitions(${MYFLAG})

SET(COMMON_STATIC_LIB 
    libevent.a 
    libcares_static.a 
    libjsoncpp.a)

SET(COMMON_SHARE_LIB 
    -lrt z libssl.so libcrypto.so libcurl.so)

project (server)

# you can change include path and library path if necessary
link_directories(/usr/local/lib/tinyco/)

add_executable(server main.cc)
include_directories(/usr/local/include/tinyco)
target_link_libraries(server 
    libtinyco_static.a 
    ${COMMON_STATIC_LIB} 
    ${COMMON_SHARE_LIB})
