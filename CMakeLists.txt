cmake_minimum_required(VERSION 3.80)

project(gram)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(MSVC)
    add_compile_options(
        /W4 /WX
    )
else()
    add_compile_options(
        -Wall -Wextra
    )
endif()

include(FetchContent)
#Commands are left empty so that we only checkout the source and no not perform any kind of build
FetchContent_Declare(gitplap
    GIT_REPOSITORY "https://github.com/JakubCygaro/plap.git"
    GIT_TAG "origin/master"
)
FetchContent_MakeAvailable(gitplap)

include_directories(${gitplap_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(gram
    ${CMAKE_SOURCE_DIR}/src/main.c
)

target_link_libraries(gram
    PRIVATE raylib
    PRIVATE -lm
)

add_executable(gram_csv
    ${CMAKE_SOURCE_DIR}/src/gram_csv.c
)

add_library(gram_update SHARED
    ${CMAKE_SOURCE_DIR}/src/gram_update.c
)

