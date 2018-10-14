#ifndef PUMPKINTOWN_DLIB_H_
#define PUMPKINTOWN_DLIB_H_

#include <cstdint>

#include "pumpkintown_gl_types.hh"

void* get_real_proc_addr(const char* name);
void* get_real_proc_addr(const unsigned char* name);

namespace pumpkintown {

class Serialize;

Serialize* serialize();

bool serialize_standard_gl_gen(int32_t count, uint32_t* array);

uint64_t glTexImage2D_pixels_num_bytes(GLenum target,
  	GLint level,
  	GLint internalformat,
  	GLsizei width,
  	GLsizei height,
  	GLint border,
  	GLenum format,
  	GLenum type,
  	const void *pixels);

}

#endif  // PUMPKINTOWN_DLIB_H_
