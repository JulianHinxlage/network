cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)


project(Network)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/util/*.cpp src/crypto/*.cpp src/net/*.cpp src/peer/*.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()

find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES})


################
# Test Targets #
################

project(test_tcp)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/test/test_tcp.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PRIVATE Network)

project(test_udp)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/test/test_udp.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PRIVATE Network)

project(test_server)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/test/test_server.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PRIVATE Network)

project(test_crypto)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/test/test_crypto.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PRIVATE Network)

project(test_peer)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/test/test_peer.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PRIVATE Network)

add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
                   COMMAND ${CMAKE_COMMAND} -E make_directory
                   ${CMAKE_BINARY_DIR}/tmp)
#foreach(i RANGE 0 10)
#    MATH(EXPR i2 "${i}+1")
#    add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
#                   COMMAND ${CMAKE_COMMAND} -E rename
#                   ${CMAKE_BINARY_DIR}/tmp/test_peer_${i}.exe
#                   ${CMAKE_BINARY_DIR}/tmp/test_peer_${i2}.exe)
#endforeach()
add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
                   COMMAND ${CMAKE_COMMAND} -E rename
                   ${CMAKE_BINARY_DIR}/test_peer.exe
                   ${CMAKE_BINARY_DIR}/tmp/test_peer_0.exe)
