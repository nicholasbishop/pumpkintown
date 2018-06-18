cmake_minimum_required(VERSION 3.0)

project(pumpkintown)

add_library(pumpkintown
  SHARED
  pumpkintown_call.c
  pumpkintown_commands.c
  pumpkintown_types.c)
