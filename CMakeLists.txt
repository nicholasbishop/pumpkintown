cmake_minimum_required(VERSION 3.0)

project(pumpkintown)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_custom_command(
  OUTPUT
  pumpkintown_dump_gen.cc
  pumpkintown_function_id.hh
  pumpkintown_function_name.cc
  pumpkintown_function_structs.cc
  pumpkintown_function_structs.hh
  pumpkintown_gl_enum.hh
  pumpkintown_gl_enum.cc
  pumpkintown_gl_functions.cc
  pumpkintown_replay_gen.cc
  pumpkintown_trace.cc
  pumpkintown_trace.hh
  COMMAND ${PROJECT_SOURCE_DIR}/gen2.py ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
  DEPENDS gen2.py pumpkinpy/parse_xml.py types.json overrides.json)

add_library(pumpkintown
  SHARED
  pumpkintown_dlib.cc
  pumpkintown_finalizers.cc
  pumpkintown_function_name.cc
  pumpkintown_function_structs.cc
  pumpkintown_gl_util.cc
  pumpkintown_io.cc
  pumpkintown_serialize.cc
  pumpkintown_trace.cc)

add_executable(pumpkintown_dump
  pumpkintown_dump.cc
  pumpkintown_dump_gen.cc
  pumpkintown_gl_enum.cc
  pumpkintown_function_structs.cc
  pumpkintown_io.cc)

# TODO
include_directories(/usr/local/include/waffle-1)

add_executable(pumpkintown_texture_dump
  pumpkintown_deserialize.cc
  pumpkintown_function_name.cc
  pumpkintown_gl_util.cc
  pumpkintown_io.cc
  pumpkintown_texture_dump.cc)

add_executable(pumpkintown_replay
  pumpkintown_deserialize.cc
  pumpkintown_function_structs.cc
  pumpkintown_gl_functions.cc
  pumpkintown_gl_util.cc
  pumpkintown_io.cc
  pumpkintown_replay.cc
  pumpkintown_replay_gen.cc)

target_link_libraries(pumpkintown_replay waffle-1)

add_custom_command(TARGET pumpkintown POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy libpumpkintown.so gl/libGL.so.1)
