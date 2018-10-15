cmake_minimum_required(VERSION 3.0)

project(pumpkintown)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_custom_command(
  OUTPUT
  pumpkintown.capnp.c++
  pumpkintown_dump_gen.cc
  pumpkintown_function_id.hh
  pumpkintown_function_name.cc
  pumpkintown_gl_enum.hh
  pumpkintown_gl_functions.cc
  pumpkintown_gl_gen.cc
  pumpkintown_gl_gen.hh
  pumpkintown_replay_gen.cc
  COMMAND ${PROJECT_SOURCE_DIR}/gen2.py ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
  DEPENDS gen2.py parse_xml.py types.json overrides.json)

add_library(pumpkintown
  SHARED
  pumpkintown.capnp.c++
  pumpkintown_dlib.cc
  pumpkintown_function_name.cc
  pumpkintown_gl_gen.cc
  pumpkintown_serialize.cc)

# add_executable(pumpkintown_dump
#   pumpkintown_deserialize.cc
#   pumpkintown_dump.cc
#   pumpkintown_dump_gen.cc)

# TODO
include_directories(/usr/local/include/waffle-1)

add_executable(pumpkintown_texture_dump
  pumpkintown_deserialize.cc
  pumpkintown_function_name.cc
  pumpkintown_gl_util.cc
  pumpkintown_texture_dump.cc)

add_executable(pumpkintown_replay
  pumpkintown_deserialize.cc
  pumpkintown_gl_functions.cc
  pumpkintown_replay.cc
  pumpkintown_replay_gen.cc
  pumpkintown_gl_util.cc)

target_link_libraries(pumpkintown_replay waffle-1)

add_custom_command(TARGET pumpkintown POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy libpumpkintown.so libGL.so.1)
