cmake_minimum_required(VERSION 3.80)

project(gram)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(gram
    ${CMAKE_SOURCE_DIR}/src/main.c
)

target_link_libraries(gram
    PRIVATE raylib
    PRIVATE -lm
)

add_library(gram_update SHARED
    ${CMAKE_SOURCE_DIR}/src/gram_update.c
)
