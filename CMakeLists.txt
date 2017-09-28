cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_VERBOSE_MAKEFILE on)
SET(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(MYFLAG "-g -D__STDC_FORMAT_MACROS -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
add_definitions(${MYFLAG})

file(GLOB TINYCO_CORE_CC *.cc ./util/*.cc ./util/*.c)
file(GLOB TINYCO_HTTP_CC 
    ./http/*.cc 
    ./dns/*.cc 
    ./third-party/http-parser/http_parser.c) 

SET(COMMON_STATIC_LIB 
    ${PROJECT_DIR}/third-party/libevent/lib/libevent.a 
    ${PROJECT_DIR}/third-party/c-ares/lib/libcares_static.a 
    ${PROJECT_DIR}/third-party/jsoncpp/src/lib_json/libjsoncpp.a)

SET(COMMON_SHARE_LIB 
    -lrt z libssl.so libcrypto.so libcurl.so)

project (tinyco)

add_library(tinyco SHARED 
    ${TINYCO_CORE_CC} 
    ${TINYCO_HTTP_CC} 
    ${COMMON_STATIC_LIB})

add_library(tinyco_static STATIC 
    ${TINYCO_CORE_CC} 
    ${TINYCO_HTTP_CC})

add_executable(udp_client
    ./example/udp_client/main.cc)

add_executable(udp_server
    ./example/udp_server/main.cc)

add_executable(http_client
    ./example/http_client/main.cc)

add_executable(http_server
    ./example/http_server/main.cc)

add_executable(dns_test
    ./example/dns/main.cc)

add_dependencies(udp_client tinyco_static)
add_dependencies(udp_server tinyco_static)
add_dependencies(http_client tinyco_static)
add_dependencies(http_server tinyco_static)
add_dependencies(dns_test tinyco_static)

include_directories(
    ./
    ./third-party/http-parser/
    ./third-party/libevent/include/
    ./third-party/c-ares/
    ./third-party/jsoncpp/include/)

target_link_libraries(udp_client 
    ${PROJECT_DIR}/libtinyco_static.a 
    ${COMMON_STATIC_LIB} 
    ${COMMON_SHARE_LIB})

target_link_libraries(udp_server 
    ${PROJECT_DIR}/libtinyco_static.a 
    ${COMMON_STATIC_LIB} 
    ${COMMON_SHARE_LIB})

target_link_libraries(http_server 
    ${PROJECT_DIR}/libtinyco_static.a 
    ${COMMON_STATIC_LIB} 
    ${COMMON_SHARE_LIB})

target_link_libraries(http_client 
    ${PROJECT_DIR}/libtinyco_static.a 
    ${COMMON_STATIC_LIB} 
    ${COMMON_SHARE_LIB})

target_link_libraries(dns_test 
    ${PROJECT_DIR}/libtinyco_static.a 
    ${COMMON_STATIC_LIB} 
    ${COMMON_SHARE_LIB})

add_custom_target(thirdparty COMMAND cd ./third-party/c-ares/ && cmake -DCARES_STATIC=ON && make && cd -
    COMMAND cd ./third-party/libevent/ && cmake . && make && cd -
    COMMAND cd ./third-party/jsoncpp/ && cmake . && make && cd -)

# install
install(TARGETS tinyco tinyco_static 
    LIBRARY DESTINATION lib/tinyco
    ARCHIVE DESTINATION lib/tinyco)

file(GLOB HEADERS *.h) 
file(GLOB UTIL_HEADERS util/*.h) 
file(GLOB HTTP_HEADERS http/*.h) 
file(GLOB DNS_HEADERS dns/*.h) 
install(FILES ${HEADERS} DESTINATION include/tinyco)
install(FILES ${UTIL_HEADERS} DESTINATION include/tinyco/util)
install(FILES ${HTTP_HEADERS} DESTINATION include/tinyco/http)
install(FILES ${DNS_HEADERS} DESTINATION include/tinyco/dns)
install(FILES ${COMMON_STATIC_LIB} DESTINATION /usr/local/lib/tinyco)

