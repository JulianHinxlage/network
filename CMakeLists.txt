cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)


project(Network)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/util/*.cpp src/net/*.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
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
