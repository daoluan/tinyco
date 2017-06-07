cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_VERBOSE_MAKEFILE on)
SET(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
SET(CMAKE_CXX_COMPILER /usr/local/bin/g++)
SET(MYFLAG "-g -D__STDC_FORMAT_MACROS -std=c++11")
add_definitions(${MYFLAG})

project (tinco)
add_executable(udp_client
    ./tool/udp_client/main.cc
    frame.cc
    thread.cc)

add_executable(udp_server
    ./tool/udp_server/main.cc)

include_directories(
    ./)

target_link_libraries(udp_client
    libevent.a -lrt)
