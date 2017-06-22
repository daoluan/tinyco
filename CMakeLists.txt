cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_VERBOSE_MAKEFILE on)
SET(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(MYFLAG "-g -D__STDC_FORMAT_MACROS -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
add_definitions(${MYFLAG})

project (tinyco)
add_executable(udp_client
    ./test/udp_client/main.cc
    frame.cc
    thread.cc)

add_executable(udp_server
    ./test/udp_server/main.cc
    frame.cc
    thread.cc)

add_executable(http_client
    ./test/http_client/main.cc
    ./http/http_client.cc
    ./http/https_client.cc
    ./thread.cc
    ./http/tcp_conn.cc
    ./http/http_request.cc
    ./http/http_response.cc
    ./dns/dns_resolve_impl.cc
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

add_executable(dns_test
    ./test/dns/main.cc
    frame.cc  
    thread.cc
    ./dns/dns_resolve_impl.cc)

include_directories(
    ./
    ./http/
    ./dns/
    ./third-party/http-parser
    ./third-party/libevent/include/
    ./third-party/c-ares/)

target_link_libraries(udp_client
    ${PROJECT_DIR}/third-party/libevent/lib/libevent.a -lrt)

target_link_libraries(udp_server
    ${PROJECT_DIR}/third-party/libevent/lib/libevent.a -lrt)

target_link_libraries(http_server
    ${PROJECT_DIR}/third-party/libevent/lib/libevent.a -lrt)

target_link_libraries(http_client
    ${PROJECT_DIR}/third-party/libevent/lib/libevent.a
    z
    libssl.so
    ${PROJECT_DIR}/third-party/c-ares/lib/libcares_static.a -lrt)

target_link_libraries(dns_test
    ${PROJECT_DIR}/third-party/libevent/lib/libevent.a ${PROJECT_DIR}/third-party/c-ares/lib/libcares_static.a -lrt)
