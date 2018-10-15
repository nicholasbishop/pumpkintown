#include "pumpkintown_dlib.hh"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include <dlfcn.h>

#include "pumpkintown_gl_enum.hh"
#include "pumpkintown_gl_types.hh"
#include "pumpkintown_serialize.hh"

void* get_libgl() {
  static void* libgl = NULL;
  if (!libgl) {
    libgl = dlopen("libGL.so", RTLD_LAZY);
  }
  return libgl;
}

void* get_real_proc_addr(const char* name) {
  static void* (*get)(const char* name) = NULL;
  if (!get) {
    get = reinterpret_cast<void* (*)(const char* name)>(dlsym(get_libgl(), "glXGetProcAddress"));
  }
  return get(name);
}

void* get_real_proc_addr(const unsigned char* name) {
  return get_real_proc_addr(reinterpret_cast<const char*>(name));
}

void* get_proc_address_gl(const char* name);
void* get_proc_address_glx(const char* name);

void* glXGetProcAddress(const char* name) {
  printf("glXGetProcAddress: looking up %s\n", name);
  void* result = get_proc_address_gl(name);
  if (result) {
    return result;
  }
  result = get_proc_address_glx(name);
  if (result) {
    return result;
  }
  printf("glXGetProcAddress: %s not found\n", name);
  return NULL;
}

void write_all(FILE* f, const uint8_t* buf, const uint64_t size) {
  uint64_t bytes_remaining = size;
  while (bytes_remaining > 0) {
    const auto bytes_written = fwrite(buf, 1, bytes_remaining, f);
    if (bytes_written <= 0) {
      fclose(f);
      throw std::runtime_error("write failed");
    }
    bytes_remaining -= bytes_written;
    buf += bytes_written;
  }
}

void write_trace_item(const uint8_t* buf, const uint64_t size) {
  // TODO cache file handle?
  FILE* f = fopen("trace.fb", "ab");
  const uint8_t* size_ptr = reinterpret_cast<const uint8_t*>(&size);
  write_all(f, size_ptr, sizeof(size));
  write_all(f, buf, size);
  fclose(f);
}

namespace pumpkintown {

Serialize* serialize() {
  static Serialize s;
  if (!s.is_open()) {
    if (!s.open("trace")) {
      // TODO
      fprintf(stderr, "failed to open trace file\n");
    }
  }
  return &s;
}

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

uint64_t glTexImage2D_pixels_num_bytes(GLenum target,
                                       GLint level,
                                       GLint internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLint border,
                                       GLenum format,
                                       GLenum type,
                                       const void *pixels) {
  if (!pixels) {
    return 0;
  }
  return (gl_texture_type_num_bytes(type) *
          gl_texture_format_num_components(format) *
          width * height);
}

uint64_t glGenTextures_textures_num_bytes(GLsizei n, GLuint* textures) {
  return n * sizeof(*textures);
}

}
