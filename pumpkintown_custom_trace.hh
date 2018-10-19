#ifndef PUMPKINTOWN_CUSTOM_TRACE_HH_
#define PUMPKINTOWN_CUSTOM_TRACE_HH_

#include "pumpkintown_gl_types.hh"

namespace pumpkintown {

void trace_append_glLinkProgram(GLuint program);

void trace_append_glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image);

}

#endif  // PUMPKINTOWN_CUSTOM_TRACE_HH_
