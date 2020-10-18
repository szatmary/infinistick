cmake_minimum_required(VERSION 3.1)
project(infinistick)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-D__STDC_CONSTANT_MACROS -D_FILE_OFFSET_BITS=64)

set(SOURCES
      ${CMAKE_SOURCE_DIR}/src/net/net.cpp
      ${CMAKE_SOURCE_DIR}/src/sqlite/sqlite3.c
)


add_executable(test ${SOURCES} ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(test curl)
