#include "pumpkintown_custom_trace.hh"

#include <string>

#include "pumpkintown_dlib.hh"
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

static void check_gl_error() {
  const auto err = glGetError();
  switch (err) {
    case GL_NO_ERROR:
      break;
    case GL_INVALID_ENUM:
      fprintf(stderr, "GL error: GL_INVALID_ENUM\n");
      break;
    case GL_INVALID_VALUE:
      fprintf(stderr, "GL error: GL_INVALID_VALUE\n");
      break;
    case GL_INVALID_OPERATION:
      fprintf(stderr, "GL error: GL_INVALID_OPERATION\n");
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      fprintf(stderr, "GL error: GL_INVALID_FRAMEBUFFER_OPERATION\n");
      break;
    case GL_OUT_OF_MEMORY:
      fprintf(stderr, "GL error: GL_OUT_OF_MEMORY\n");
      break;
    default:
      fprintf(stderr, "GL error: %d\n", err);
      break;
  }
}

void trace_append_glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image) {
  const GLint level{0};
  GLint internalformat;
  GLsizei width{0};
  GLsizei height{0};
  GLint border;
  GLenum format;
  GLenum type;
  const GLvoid * data;

  // Bind to the current texture target (TODO, do we need to create a new texture?)
  using Fn = void (*)(GLenum, GLeglImageOES);
  static Fn real_fn = reinterpret_cast<Fn>(get_real_proc_addr("glEGLImageTargetTexture2DOES"));
  real_fn(GL_TEXTURE_2D, image);

  check_gl_error();

  glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &width);
  check_gl_error();
  glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &height);
  check_gl_error();
  fprintf(stderr, "%d x %d\n", width, height);

  // glTexImage2D(target2, level, internalformat, width, height, border,
  //              format, type, data);
}

}
