cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_VERBOSE_MAKEFILE on)
SET(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(MYFLAG "-g -D__STDC_FORMAT_MACROS -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
add_definitions(${MYFLAG})

file(GLOB TINYCO_CORE_CC *.cc ./util/*.cc)
file(GLOB TINYCO_HTTP_CC 
    ./http/*.cc 
    ./dns/*.cc 
    ./third-party/http-parser/http_parser.c) 

SET(COMMON_STATIC_LIB 
    ${PROJECT_DIR}/third-party/libevent/lib/libevent.a 
    ${PROJECT_DIR}/third-party/c-ares/lib/libcares_static.a 
    ${PROJECT_DIR}/third-party/jsoncpp/src/lib_json/libjsoncpp.a   
    -lrt z libssl.so libcrypto.so)

project (tinyco)
add_executable(udp_client
    ./example/udp_client/main.cc
    ${TINYCO_CORE_CC})

add_executable(udp_server
    ./example/udp_server/main.cc
    ${TINYCO_CORE_CC})

add_executable(http_client
    ./example/http_client/main.cc
    ${TINYCO_CORE_CC}
    ${TINYCO_HTTP_CC})

add_executable(http_server
    ./example/http_server/main.cc
    ${TINYCO_CORE_CC}
    ${TINYCO_HTTP_CC})

add_executable(dns_test
    ./example/dns/main.cc
    ${TINYCO_CORE_CC}
    ${TINYCO_HTTP_CC})

add_executable(server
    ./example/server/main.cc
    ${TINYCO_CORE_CC} 
    ${TINYCO_HTTP_CC})

include_directories(
    ./
    ./third-party/http-parser/
    ./third-party/libevent/include/
    ./third-party/c-ares/
    ./third-party/jsoncpp/include/)

target_link_libraries(udp_client ${COMMON_STATIC_LIB})

target_link_libraries(udp_server ${COMMON_STATIC_LIB})

target_link_libraries(http_server ${COMMON_STATIC_LIB})

target_link_libraries(http_client ${COMMON_STATIC_LIB})

target_link_libraries(dns_test ${COMMON_STATIC_LIB})

target_link_libraries(server ${COMMON_STATIC_LIB})
