#include <cstring>
#include <stdexcept>
#include <vector>

#include "pumpkintown_function_structs.hh"
#include "pumpkintown_gl_enum.hh"
#include "pumpkintown_gl_types.hh"
#include "pumpkintown_gl_util.hh"
#include "pumpkintown_io.hh"

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

void FnGlTexSubImage2D::finalize() {
  if (pixels) {
    pixels_length = (gl_texture_type_num_bytes(type) *
                     gl_texture_format_num_components(format) *
                     width * height);
  } else {
    pixels_length = 0;
  }
}

void FnGlGenFramebuffers::finalize() {
  framebuffers_length = n;
}

void FnGlGenTextures::finalize() {
  textures_length = n;
}

void FnGlGenVertexArrays::finalize() {
  arrays_length = n;
}

void FnGlGenBuffers::finalize() {
  buffers_length = n;
}

void FnGlGenRenderbuffers::finalize() {
  renderbuffers_length = n;
}

void FnGlBufferData::finalize() {
  data_length = size;
}

void FnGlVertexAttrib4fv::finalize() {
  v_length = 4;
}

void FnGlProgramBinary::finalize() {
  binary_length = length;
}

void FnGlUniform1iv::finalize() {
  value_length = 1;
}

void FnGlUniform1fv::finalize() {
  value_length = 1;
}

void FnGlUniform2fv::finalize() {
  value_length = 2;
}

void FnGlUniform4fv::finalize() {
  value_length = 4;
}

void FnGlUniformMatrix4fv::finalize() {
  value_length = 16;
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

FnGlShaderSource::~FnGlShaderSource() {
  if (string) {
    for (int32_t i{0}; i < count; i++) {
      delete[] string[i];
    }
  }
  delete[] string;
  delete[] length;
}

static std::vector<int32_t> gl_shader_source_lengths(
    const int32_t count,
    const int32_t* length,
    const char* const* string) {
  std::vector<int32_t> real_lengths;
  for (int32_t i{0}; i < count; i++) {
    if (length && length[i] >= 0) {
      real_lengths.emplace_back(length[i]);
    } else {
      real_lengths.emplace_back(strlen(string[i]) + 1);
    }
  }
  return real_lengths;
}

uint64_t FnGlShaderSource::num_bytes() const {
  uint64_t total{sizeof(*this) + (count * sizeof(int32_t))};

  const auto real_lengths{gl_shader_source_lengths(count, length, string)};
  for (const auto len : real_lengths) {
    total += len;
  }
  return total;
}

std::string FnGlShaderSource::to_string() const {
  std::string result = "glShaderSource:\n";
  for (int32_t i{0}; i < count; i++) {
    result += "source " + std::to_string(i) + ":\n";
    result += string[i];
  }
  return result;
}

void FnGlShaderSource::read_from_file(FILE* f) {
  read_exact(f, this, sizeof(*this));
  if (count > 0) {
    auto length_tmp = new int32_t[count];
    read_exact(f, length_tmp, count * sizeof(*length_tmp));
    length = length_tmp;

    auto string_tmp = new char*[count];
    for (int32_t i{0}; i < count; i++) {
      string_tmp[i] = new char[length[i]];
      read_exact(f, string_tmp[i], length[i]);
    }
    string = string_tmp;
  } else {
    length = nullptr;
    string = nullptr;
  }
}

void FnGlShaderSource::write_from_file(FILE* f) {
  write_exact(f, this, sizeof(*this));

  const auto real_lengths{gl_shader_source_lengths(count, length, string)};

  write_exact(f, real_lengths.data(), count * sizeof(int32_t));
  for (int32_t i{0}; i < count; i++) {
    write_exact(f, string[i], real_lengths[i]);
  }
  fflush(f);
}

}
