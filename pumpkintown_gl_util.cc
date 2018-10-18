#include "pumpkintown_gl_util.hh"

#include <cstdio>
#include <stdexcept>

#include "pumpkintown_gl_enum.hh"

namespace pumpkintown {

uint64_t gl_texture_format_num_components(const GLenum type) {
  switch (type) {
    case GL_ALPHA:
    case GL_RED:
      return 1;
    case GL_RGB:
      return 3;
    case GL_BGRA:
    case GL_RGBA:
      return 4;
  }
  fprintf(stderr, "unknown texture format: 0x%x\n", type);
  throw std::runtime_error("unknown texture format");
}

const char* gl_draw_elements_mode_string(const GLenum mode) {
  switch (mode) {
    case GL_POINTS:
      return "GL_POINTS";
    case GL_LINE_STRIP:
      return "GL_LINE_STRIP";
    case GL_LINE_LOOP:
      return "GL_LINE_LOOP";
    case GL_LINES:
      return "GL_LINES";
    case GL_LINE_STRIP_ADJACENCY:
      return "GL_LINE_STRIP_ADJACENCY";
    case GL_LINES_ADJACENCY:
      return "GL_LINES_ADJACENCY";
    case GL_TRIANGLE_STRIP:
      return "GL_TRIANGLE_STRIP";
    case GL_TRIANGLE_FAN:
      return "GL_TRIANGLE_FAN";
    case GL_TRIANGLES:
      return "GL_TRIANGLES";
    case GL_TRIANGLE_STRIP_ADJACENCY:
      return "GL_TRIANGLE_STRIP_ADJACENCY";
    case GL_TRIANGLES_ADJACENCY:
      return "GL_TRIANGLES_ADJACENCY";
    case GL_PATCHES:
      return "GL_PATCHES";
  }
  return "invalid";
}

}
