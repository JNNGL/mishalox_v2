cmake_minimum_required(VERSION 3.23)
project(mishalox_v2 C)

set(CMAKE_C_STANDARD 99)

if (WIN32)
    message(FATAL_ERROR "Мишалохи не играют на форточке")
endif()

file(GLOB_RECURSE MISHALOX_SOURCES src/**)
add_executable(mishalox_v2 ${MISHALOX_SOURCES})
target_link_libraries(mishalox_v2 event)