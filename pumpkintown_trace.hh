#ifndef PUMPKINTOWN_CUSTOM_TRACE_HH_
#define PUMPKINTOWN_CUSTOM_TRACE_HH_

#include <string>

#include "pumpkintown_gl_types.hh"

namespace pumpkintown {

void begin_message(const std::string& name);

void trace_append_glLinkProgram(GLuint program);

void trace_append_glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image);

void print_tids();

}

#endif  // PUMPKINTOWN_CUSTOM_TRACE_HH_
