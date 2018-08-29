cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS "-std=c99 -g")

project(tcpdash)

set(CMAKE_SOURCE_DIR /home/user/tcpdash)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/apps/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/apps/lib)

include_directories("${PROJECT_SOURCE_DIR}" /usr/include/glib-2.0 /usr/lib/x86_64-linux-gnu/glib-2.0/include)

add_executable(sf_test ${PROJECT_SOURCE_DIR}/socket_factory/sf_main.c)

target_link_libraries(sf_test glib-2.0)

