cmake_minimum_required(VERSION 3.0)

project(pumpkintown)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

add_library(pumpkintown
  SHARED
  pumpkintown_call.c
  pumpkintown_commands.c
  pumpkintown_types.c)
