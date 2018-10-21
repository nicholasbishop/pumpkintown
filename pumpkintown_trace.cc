#include "pumpkintown_trace.hh"

#include <map>
#include <string>
#include <vector>

#include "pumpkintown_dlib.hh"
#include "pumpkintown_function_id.hh"
#include "pumpkintown_function_structs.hh"
#include "pumpkintown_gl_enum.hh"
#include "pumpkintown_io.hh"
#include "pumpkintown_serialize.hh"
#include "pumpkintown_trace_gen.hh"

namespace pumpkintown {

void begin_message(const std::string& name) {
  static std::map<std::string, uint16_t> map;
  static uint16_t next_id{0};
  const auto iter = map.find(name);
  if (iter == map.end()) {
    const auto id = next_id;
    next_id++;
    serialize()->write(static_cast<uint8_t>(MsgType::FunctionId));
    serialize()->write(static_cast<uint16_t>(id));
    serialize()->write(static_cast<uint8_t>(name.size()));
    serialize()->write(reinterpret_cast<const uint8_t*>(name.c_str()),
                       name.size());
    map[name] = id;
    begin_message(name);
  } else {
    serialize()->write(static_cast<uint8_t>(MsgType::Call));
    serialize()->write(static_cast<uint16_t>(map[name]));
  }
}

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

static void check_gl_error(const int line) {
  const auto err = glGetError();
  switch (err) {
    case GL_NO_ERROR:
      break;
    case GL_INVALID_ENUM:
      fprintf(stderr, "GL error(%d): GL_INVALID_ENUM\n", line);
      break;
    case GL_INVALID_VALUE:
      fprintf(stderr, "GL error(%d): GL_INVALID_VALUE\n", line);
      break;
    case GL_INVALID_OPERATION:
      fprintf(stderr, "GL error(%d): GL_INVALID_OPERATION\n", line);
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      fprintf(stderr, "GL error(%d): GL_INVALID_FRAMEBUFFER_OPERATION\n", line);
      break;
    case GL_OUT_OF_MEMORY:
      fprintf(stderr, "GL error(%d): GL_OUT_OF_MEMORY\n", line);
      break;
    default:
      fprintf(stderr, "GL error(%d): %d\n", line, err);
      break;
  }
}

// TODO this is copied out of the autogen code
static void write_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels) {
  begin_message("glTexImage2D");
  pumpkintown::FnGlTexImage2D fn;
  fn.target = target;
  fn.level = level;
  fn.internalformat = internalformat;
  fn.width = width;
  fn.height = height;
  fn.border = border;
  fn.format = format;
  fn.type = type;
  fn.pixels = reinterpret_cast<const uint8_t*>(pixels);
  fn.finalize();
  pumpkintown::serialize()->write(fn.num_bytes());
  fn.write_to_file(pumpkintown::serialize()->file());
  fn.pixels = nullptr;
}

void trace_append_glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image) {
  const GLint level{0};  // TODO is this always correct?
  GLint internalformat{GL_RGBA}; // TODO
  GLsizei width{0};
  GLsizei height{0};
  GLint border{0};
  GLenum format{GL_RGBA};
  GLenum type{GL_UNSIGNED_BYTE};
  //const GLvoid * data;

  GLint orig_fbo{0};
  pumpkintown::real::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &orig_fbo);

  GLuint rb{0};
  pumpkintown::real::glGenRenderbuffers(1, &rb);
  pumpkintown::real::glBindRenderbuffer(GL_RENDERBUFFER_OES, rb);
  pumpkintown::real::glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_OES, image);
  pumpkintown::real::glGetRenderbufferParameteriv(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH, &width);
  pumpkintown::real::glGetRenderbufferParameteriv(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT, &height);

  GLuint fbo{0};
  pumpkintown::real::glGenFramebuffers(1, &fbo);
  pumpkintown::real::glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  pumpkintown::real::glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER_OES, rb);
  check_gl_error(__LINE__);

  const auto status = pumpkintown::real::glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "GL error, framebuffer status=%d\n", status);
    return;
  }

  std::vector<uint8_t> pixels;
  pixels.resize(width * height * 4);
  pumpkintown::real::glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                                  pixels.data());

  write_glTexImage2D(GL_TEXTURE_2D, level, internalformat, width, height,
               border, format, type, pixels.data());

  pumpkintown::real::glBindFramebuffer(GL_FRAMEBUFFER, orig_fbo);

  check_gl_error(__LINE__);

  // TODO: do we need to gen and bind a new texture here?
#if 0
  pumpkintown::real::glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
  check_gl_error(__LINE__);

  GLint orig_tex{0};
  pumpkintown::real::glGetIntegerv(GL_TEXTURE_BINDING_2D, &orig_tex);
  check_gl_error(__LINE__);
  if (!orig_tex)
    return;

  GLint orig_fbo{0};
  pumpkintown::real::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &orig_fbo);
  check_gl_error(__LINE__);

  GLuint fbo{0};
  pumpkintown::real::glGenFramebuffers(1, &fbo);
  check_gl_error(__LINE__);
  pumpkintown::real::glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  check_gl_error(__LINE__);

  pumpkintown::real::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                            GL_TEXTURE_2D, orig_tex, level);
  const auto status = pumpkintown::real::glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "status=%d\n", status);
    return;
  }

  pumpkintown::real::glGetNamedFramebufferParameteriv(
      fbo, GL_FRAMEBUFFER_DEFAULT_WIDTH, &width);
  check_gl_error(__LINE__);
  pumpkintown::real::glGetNamedFramebufferParameteriv(
      fbo, GL_FRAMEBUFFER_DEFAULT_HEIGHT, &height);
  check_gl_error(__LINE__);

#endif
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
