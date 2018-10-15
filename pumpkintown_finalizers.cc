#include <stdexcept>

#include "pumpkintown_function_structs.hh"
#include "pumpkintown_gl_enum.hh"
#include "pumpkintown_gl_types.hh"
#include "pumpkintown_gl_util.hh"

namespace pumpkintown {

namespace {

uint64_t gl_texture_type_num_bytes(const GLenum format) {
  switch (format) {
    case GL_UNSIGNED_BYTE:
      return 1;
    case GL_FLOAT:
      return 4;
  }
  fprintf(stderr, "unknown texture type: 0x%x\n", format);
  throw std::runtime_error("unknown texture type");
}

}

void FnGlTexImage2D::finalize() {
  if (pixels) {
    pixels_length = (gl_texture_type_num_bytes(type) *
                     gl_texture_format_num_components(format) *
                     width * height);
  } else {
    pixels_length = 0;
  }
}

void FnGlGenTextures::finalize() {
  textures_length = n * sizeof(*textures);
}

}
