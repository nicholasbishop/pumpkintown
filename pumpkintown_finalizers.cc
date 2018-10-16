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

void FnGlLightfv::finalize() {
  params_length = 0;
  switch (pname) {
    case GL_AMBIENT:
      params_length = 4;
      break;
    case GL_DIFFUSE:
      params_length = 4;
      break;
    case GL_SPECULAR:
      params_length = 4;
      break;
    case GL_POSITION:
      params_length = 4;
      break;
    case GL_SPOT_CUTOFF:
      params_length = 1;
      break;
    case GL_SPOT_DIRECTION:
      params_length = 3;
      break;
    case GL_SPOT_EXPONENT:
      params_length = 1;
      break;
    case GL_CONSTANT_ATTENUATION:
      params_length = 1;
      break;
    case GL_LINEAR_ATTENUATION:
      params_length = 1;
      break;
    case GL_QUADRATIC_ATTENUATION:
      params_length = 1;
      break;
  }
}

void FnGlMaterialfv::finalize() {
  switch (pname) {
    case GL_AMBIENT:
      params_length = 4;
      break;
    case GL_DIFFUSE:
      params_length = 4;
      break;
    case GL_SPECULAR:
      params_length = 4;
      break;
    case GL_EMISSION:
      params_length = 4;
      break;
    case GL_SHININESS:
      params_length = 1;
      break;
    case GL_AMBIENT_AND_DIFFUSE:
      params_length = 4;
      break;
    case GL_COLOR_INDEXES:
      params_length = 3;
      break;
  }
}

}
