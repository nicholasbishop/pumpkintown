#include "pumpkintown_gl_util.hh"

#include <cstdio>
#include <stdexcept>

#include "pumpkintown_gl_enum.hh"

namespace pumpkintown {

uint64_t gl_texture_format_num_components(const GLenum type) {
  switch (type) {
    case GL_RED:
      return 1;
    case GL_RGB:
      return 3;
    case GL_RGBA:
      return 4;
  }
  fprintf(stderr, "unknown texture format: 0x%x\n", type);
  throw std::runtime_error("unknown texture format");
}

}
