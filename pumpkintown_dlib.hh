#ifndef PUMPKINTOWN_DLIB_H_
#define PUMPKINTOWN_DLIB_H_

#include <cstdint>

#include "pumpkintown_gl_types.hh"

void* get_real_proc_addr(const char* name);
void* get_real_proc_addr(const unsigned char* name);

namespace pumpkintown {

class Serialize;

Serialize* serialize();

uint64_t glTexImage2D_pixels_num_bytes(GLenum target,
  	GLint level,
  	GLint internalformat,
  	GLsizei width,
  	GLsizei height,
  	GLint border,
  	GLenum format,
  	GLenum type,
  	const void *pixels);

uint64_t glGenTextures_textures_num_bytes(GLsizei n, GLuint* textures);

}

#endif  // PUMPKINTOWN_DLIB_H_
