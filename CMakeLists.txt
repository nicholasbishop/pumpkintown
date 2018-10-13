cmake_minimum_required(VERSION 3.0)

project(pumpkintown)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_INCLUDE_CURRENT_DIR true)

add_custom_command(
  OUTPUT pumpkintown_gl_gen.cc pumpkintown_gl_gen.hh pumpkintown_dump_gen.cc
  COMMAND ${PROJECT_SOURCE_DIR}/gen2.py ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
  DEPENDS gen2.py glinfo.json)

add_library(pumpkintown
  SHARED
  pumpkintown_dlib.cc
  pumpkintown_gl.cc
  pumpkintown_gl_gen.cc
  pumpkintown_init.cc
)

add_executable(pumpkintown_dump
  pumpkintown_dump.cc
  pumpkintown_dump_gen.cc)

add_custom_command(TARGET pumpkintown POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy libpumpkintown.so libGL.so.1)
