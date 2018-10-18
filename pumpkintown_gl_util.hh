#ifndef PUMPKINTOWN_GL_UTIL_HH_
#define PUMPKINTOWN_GL_UTIL_HH_

#include "pumpkintown_gl_types.hh"
#include "pumpkintown_function_id.hh"

namespace pumpkintown {

uint64_t gl_texture_format_num_components(const GLenum type);

const char* function_id_to_string(const FunctionId id);

const char* gl_draw_elements_mode_string(const GLenum mode);

}

#endif  // PUMPKINTOWN_GL_UTIL_HH_
