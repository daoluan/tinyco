cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_VERBOSE_MAKEFILE on)
SET(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
SET(CMAKE_CXX_COMPILER /usr/local/bin/g++)
SET(MYFLAG "-g -D__STDC_FORMAT_MACROS -std=c++11 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
add_definitions(${MYFLAG})

project (tinco)
add_executable(udp_client
    ./test/udp_client/main.cc
    frame.cc
    thread.cc)

add_executable(udp_server
    ./test/udp_server/main.cc)

add_executable(http_client
    ./test/http_client/main.cc
    ./http/http_client.cc
    ./http/https_client.cc
    ./thread.cc
    ./http/tcp_network.cc
    ./http/http_request.cc
    ./http/http_response.cc
    ./http/http_op.cc
    ./frame.cc
    ./http/http_tool.cc
    ./third-party/http-parser/http_parser.c)

add_executable(http_server
    ./test/http_server/main.cc
    frame.cc  
    thread.cc
    mutex.cc
    ./http/http_tool.cc
    ./http/http_server.cc
    ./third-party/http-parser/http_parser.c
    ./http/http_request.cc
    ./http/http_response.cc)

include_directories(
    ./
    ./http/
    ./third-party/http-parser)

target_link_libraries(udp_client
    libevent.a -lrt)

target_link_libraries(http_server
    libevent.a -lrt)

target_link_libraries(http_client
    libevent.a -lrt
    z
    rt
    /usr/lib64/libssl.so)
