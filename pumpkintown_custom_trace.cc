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
  const GLint level{0};  // TODO is this always correct?
  //GLint internalformat;
  GLsizei width{0};
  GLsizei height{0};
  //GLint border;
  //GLenum format;
  //GLenum type;
  //const GLvoid * data;

  // TODO: do we need to gen and bind a new texture here?

  pumpkintown::real::glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

  GLint orig_tex{0};
  pumpkintown::real::glGetIntegerv(GL_TEXTURE_BINDING_2D, &orig_tex);
  check_gl_error();
  if (!orig_tex)
    return;

  GLint orig_fbo{0};
  pumpkintown::real::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &orig_fbo);
  check_gl_error();

  GLuint fbo{0};
  pumpkintown::real::glGenFramebuffers(1, &fbo);
  check_gl_error();
  pumpkintown::real::glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  check_gl_error();

  pumpkintown::real::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                            GL_TEXTURE_2D, orig_tex, level);
  const auto status = pumpkintown::real::glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "status=%d\n", status);
    return;
  }

  pumpkintown::real::glGetNamedFramebufferParameteriv(
      fbo, GL_FRAMEBUFFER_DEFAULT_WIDTH, &width);
  pumpkintown::real::glGetNamedFramebufferParameteriv(
      fbo, GL_FRAMEBUFFER_DEFAULT_HEIGHT, &height);

  fprintf(stderr, "%d x %d\n", width, height);

  // Bind to the current texture target (TODO, do we need to create a new texture?)
  // using Fn = void (*)(GLenum, GLeglImageOES);
  // static Fn real_fn = reinterpret_cast<Fn>(get_real_proc_addr("glEGLImageTargetTexture2DOES"));
  // real_fn(GL_TEXTURE_2D, image);

  // check_gl_error();

  // glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &width);
  // check_gl_error();
  // glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &height);
  // check_gl_error();
  // fprintf(stderr, "%d x %d\n", width, height);

  // glTexImage2D(target2, level, internalformat, width, height, border,
  //              format, type, data);
}

}
