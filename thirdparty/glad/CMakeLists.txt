cmake_minimum_required(VERSION 3.20.4 FATAL_ERROR)

project(glad)

set(GLAD_SOURCES src/gl.c)
set(GLAD_HEADERS include/glad/gl.h
                 include/KHR/khrplatform.h)

add_library(glad STATIC ${GLAD_SOURCES} ${GLAD_HEADERS})
target_check_style(glad)

target_include_directories(glad PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
