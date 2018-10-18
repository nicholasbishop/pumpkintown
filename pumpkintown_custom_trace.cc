#include "pumpkintown_custom_trace.hh"

#include <string>

#include "pumpkintown_gl_enum.hh"
#include "pumpkintown_trace.hh"

namespace pumpkintown {

void trace_append_glLinkProgram(const GLuint program) {
  GLint num_active_attributes{0};
  glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &num_active_attributes);
  for (GLint attrib{0}; attrib < num_active_attributes; attrib++) {
    // TODO(nicholasbishop): proper way to size this buffer?
    GLchar name[512];
    GLint size{0};
    GLenum type{0};
                
    glGetActiveAttrib(program, attrib, sizeof(name), NULL, &size, &type, name);
    if (std::string(name).substr(0, 3) == "gl_") {
      continue;
    }

    const GLint location{glGetAttribLocation(program, name)};
    if (location >= 0) {
      glBindAttribLocation(program, location, name);
    }
  }
}

}
