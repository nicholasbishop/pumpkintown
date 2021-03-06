cmake_minimum_required(VERSION 3.0)

project(pumpkintown)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -std=c++11")

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_custom_command(
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT glmeta.py
  COMMAND ${PROJECT_SOURCE_DIR}/gen_meta.py ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
  DEPENDS
  gen_meta.py
  pumpkinpy/parse_xml.py
  pumpkinpy/types.py
  types.json
  overrides.json)

add_custom_command(
  OUTPUT
  #pumpkintown_dump_gen.cc
  pumpkintown_express_gen.cc
  pumpkintown_function_id.hh
  pumpkintown_function_id.cc
  pumpkintown_function_structs.cc
  pumpkintown_function_structs.hh
  pumpkintown_gl_enum.hh
  pumpkintown_gl_enum.cc
  pumpkintown_gl_functions.cc
  pumpkintown_replay_gen.cc
  pumpkintown_trace_gen.cc
  pumpkintown_trace_gen.hh
  COMMAND ${PROJECT_SOURCE_DIR}/gen2.py ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
  DEPENDS
  gen2.py
  glmeta.py
  pumpkinpy/types.py)

add_library(pumpkintown
  SHARED
  pumpkintown_trace.cc
  pumpkintown_dlib.cc
  pumpkintown_finalizers.cc
  pumpkintown_function_structs.cc
  pumpkintown_gl_enum.cc
  pumpkintown_gl_util.cc
  pumpkintown_io.cc
  pumpkintown_serialize.cc
  pumpkintown_trace_gen.cc)

# add_executable(pumpkintown_dump
#   pumpkintown_dump.cc
#   pumpkintown_dump_gen.cc
#   pumpkintown_gl_enum.cc
#   pumpkintown_function_structs.cc
#   pumpkintown_io.cc)

add_executable(pumpkintown_replay
  pumpkintown_finalizers.cc
  pumpkintown_function_id.cc
  pumpkintown_function_structs.cc
  pumpkintown_gl_enum.cc
  pumpkintown_gl_functions.cc
  pumpkintown_gl_util.cc
  pumpkintown_io.cc
  pumpkintown_replay.cc
  pumpkintown_replay_gen.cc)

target_link_libraries(pumpkintown_replay waffle-1)

add_executable(pumpkintown_express
  pumpkintown_express.cc
  pumpkintown_express_gen.cc
  pumpkintown/parser.cc
  pumpkintown/text_trace_iterator.cc)

target_link_libraries(pumpkintown_express epoxy waffle-1)

add_custom_command(TARGET pumpkintown POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy libpumpkintown.so gl/libGL.so.1)

add_custom_command(TARGET pumpkintown POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy libpumpkintown.so gl/libGLESv2.so.2)

add_custom_command(TARGET pumpkintown POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy libpumpkintown.so gl/libEGL.so.1)

install(TARGETS pumpkintown_replay RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gl/libGL.so.1 DESTINATION share/pumpkintown)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gl/libGLESv2.so.2 DESTINATION share/pumpkintown)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gl/libEGL.so.1 DESTINATION share/pumpkintown)
