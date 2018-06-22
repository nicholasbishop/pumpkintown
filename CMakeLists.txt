cmake_minimum_required(VERSION 3.0)

project(pumpkintown)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

add_library(pumpkintown
  SHARED
  pumpkintown_call.c
  pumpkintown_commands.c
  pumpkintown_dlib.c
  pumpkintown_glx_commands.c
  pumpkintown_types.c)

add_executable(pumpkintown_replay
  pumpkintown_file.cc
  pumpkintown_replay.cc
  pumpkintown_replay_commands.cc)

target_link_libraries(pumpkintown_replay epoxy waffle-1)
