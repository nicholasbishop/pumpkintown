#include "pumpkintown_replay.hh"

#include <cassert>
#include <vector>

#include <stdlib.h>
#include <unistd.h>

#define WAFFLE_API_VERSION 0x0106
#include <waffle.h>

#include "pumpkintown_deserialize.hh"
#include "pumpkintown_function_structs.hh"
#include "pumpkintown_gl_enum.hh"
#include "pumpkintown_gl_functions.hh"
#include "pumpkintown_gl_types.hh"
#include "pumpkintown_io.hh"
#include <unistd.h>
namespace pumpkintown {

namespace {

void check_gl_error() {
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

}

Replay::Replay(const std::string& path)
    : iter_{path} {
  texture_ids_[0] = 0;
  framebuffer_ids_[0] = 0;
  vertex_arrays_ids_[0] = 0;

  const int32_t init_attrs[] = {
    WAFFLE_PLATFORM, WAFFLE_PLATFORM_GLX,
    0,
  };

  const int32_t config_attrs[] = {
    WAFFLE_CONTEXT_API,         WAFFLE_CONTEXT_OPENGL_ES2,

#if 0
    WAFFLE_CONTEXT_MAJOR_VERSION, 3,
    WAFFLE_CONTEXT_MINOR_VERSION, 3,

    //WAFFLE_CONTEXT_PROFILE,  WAFFLE_CONTEXT_COMPATIBILITY_PROFILE,
#endif

    WAFFLE_RED_SIZE,            8,
    WAFFLE_BLUE_SIZE,           8,
    WAFFLE_GREEN_SIZE,          8,
    WAFFLE_ALPHA_SIZE,          8,

    WAFFLE_DEPTH_SIZE, 24,

    WAFFLE_STENCIL_SIZE, 1,

         
    WAFFLE_DOUBLE_BUFFERED, false,

    //WAFFLE_CONTEXT_DEBUG, true,

    0,
  };

  const int32_t window_width = 800;
  const int32_t window_height = 600;

  waffle_init(init_attrs);
  display_ = waffle_display_connect(NULL);

  config_ = waffle_config_choose(display_, config_attrs);
  assert(config_);
  window_ = waffle_window_create(config_, window_width, window_height);
  assert(window_);
  default_context_ = waffle_context_create(config_, NULL);
  assert(default_context_);

  waffle_window_show(window_);
  waffle_make_current(display_, window_, default_context_);

  glClearColor(0.4, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  sleep(1);
  //waffle_window_swap_buffers(window_);
}

void Replay::replay() {
  while (!iter_.done()) {
    iter_.next();
    replay_one();

    check_gl_error();
  }
}

void Replay::custom_glXCreateContext(const FnGlXCreateContext& fn) {
  contexts_[fn.return_value] = waffle_context_create(config_, nullptr);
}

void Replay::custom_glXCreateContextAttribsARB(const FnGlXCreateContextAttribsARB& fn) {
  contexts_[fn.return_value] = waffle_context_create(config_, nullptr);
}

void Replay::custom_glXCreateNewContext(const FnGlXCreateNewContext& fn) {
  contexts_[fn.return_value] = waffle_context_create(config_, nullptr);
}

void Replay::custom_glXMakeContextCurrent(const FnGlXMakeContextCurrent& fn) {
  waffle_make_current(display_, window_, contexts_[fn.ctx]);
}

void Replay::custom_glXMakeCurrent(const FnGlXMakeCurrent& fn) {
  waffle_make_current(display_, window_, contexts_[fn.ctx]);
}

void Replay::custom_glGenBuffers(const FnGlGenBuffers& fn) {
  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.buffers_length);
  glGenBuffers(fn.buffers_length, new_ids.data());

  for (uint32_t i{0}; i < fn.buffers_length; i++) {
    buffer_ids_[fn.buffers[i]] = new_ids[i];
  }
}

void Replay::custom_glBindBuffer(const FnGlBindBuffer& fn) {
  if (fn.buffer != 0) {
    assert(buffer_ids_[fn.buffer] != 0);
  }
  glBindBuffer(fn.target, buffer_ids_[fn.buffer]);
}

void Replay::custom_glGenTextures(const FnGlGenTextures& fn) {
  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.textures_length);
  glGenTextures(fn.textures_length, new_ids.data());

  for (uint32_t i{0}; i < fn.textures_length; i++) {
    texture_ids_[fn.textures[i]] = new_ids[i];
  }
}

void Replay::custom_glBindTexture(const FnGlBindTexture& fn) {
  glBindTexture(fn.target, texture_ids_[fn.texture]);
}

void Replay::custom_glGenFramebuffers(const FnGlGenFramebuffers& fn) {
  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.framebuffers_length);
  glGenFramebuffers(fn.framebuffers_length, new_ids.data());

  for (uint32_t i{0}; i < fn.framebuffers_length; i++) {
    framebuffer_ids_[fn.framebuffers[i]] = new_ids[i];
  }
}

void Replay::custom_glBindFramebuffer(const FnGlBindFramebuffer& fn) {
  glBindFramebuffer(fn.target, framebuffer_ids_[fn.framebuffer]);
}

void Replay::custom_glGenVertexArrays(const FnGlGenVertexArrays& fn) {
  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.arrays_length);
  glGenVertexArrays(fn.arrays_length, new_ids.data());

  for (uint32_t i{0}; i < fn.arrays_length; i++) {
    vertex_arrays_ids_[fn.arrays[i]] = new_ids[i];
  }
}

void Replay::custom_glBindVertexArray(const FnGlBindVertexArray& fn) {
  glBindVertexArray(vertex_arrays_ids_[fn.array]);
}

void Replay::custom_glGenRenderbuffers(const FnGlGenRenderbuffers& fn) {
  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.renderbuffers_length);
  glGenRenderbuffers(fn.renderbuffers_length, new_ids.data());

  for (uint32_t i{0}; i < fn.renderbuffers_length; i++) {
    renderbuffer_ids_[fn.renderbuffers[i]] = new_ids[i];
  }
}

void Replay::custom_glBindRenderbuffer(const FnGlBindRenderbuffer& fn) {
  glBindRenderbuffer(fn.target, renderbuffer_ids_[fn.renderbuffer]);
}

void Replay::custom_glFramebufferTexture2D(const FnGlFramebufferTexture2D& fn) {
  glFramebufferTexture2D(fn.target, fn.attachment, fn.textarget,
                         texture_ids_[fn.texture], fn.level);
}

void Replay::custom_glGenLists(const FnGlGenLists& fn) {
  const auto new_start{glGenLists(fn.range)};

  for (int32_t i{0}; i < fn.range; i++) {
    display_list_ids_[fn.return_value + i] = new_start + i;
  }
}

void Replay::custom_glNewList(const FnGlNewList& fn) {
  glNewList(display_list_ids_[fn.list], fn.mode);
}

void Replay::custom_glCallList(const FnGlCallList& fn) {
  glCallList(display_list_ids_[fn.list]);
}

void Replay::custom_glCompileShader(const FnGlCompileShader& fn) {
  glCompileShader(fn.shader);

  // Print shader source
  if (false) {
    GLint srclen = 0;
    glGetShaderiv(fn.shader, GL_SHADER_SOURCE_LENGTH, &srclen);
    std::vector<char> src(srclen);
    glGetShaderSource(fn.shader, srclen, &srclen, src.data());
    fprintf(stderr, "shader source: %s\n", src.data());
  }

  GLint success = 0;
  glGetShaderiv(fn.shader, GL_COMPILE_STATUS, &success);
  if (success) {
    return;
  }

  fprintf(stderr, "compile failed:\n");

  GLint length = 0;
  glGetShaderiv(fn.shader, GL_INFO_LOG_LENGTH, &length);
  std::vector<char> log(length);
  glGetShaderInfoLog(fn.shader, length, &length, log.data());

  fprintf(stderr, "%s\n", log.data());
}

void Replay::custom_glLinkProgram(const FnGlLinkProgram& fn) {
  glLinkProgram(fn.program);

  GLint success = 0;
  glGetProgramiv(fn.program, GL_LINK_STATUS, &success);
  if (success) {
    return;
  }

  fprintf(stderr, "link failed:\n");

  GLint length = 0;
  glGetProgramiv(fn.program, GL_INFO_LOG_LENGTH, &length);
  std::vector<char> log(length);
  glGetProgramInfoLog(fn.program, length, &length, log.data());

  fprintf(stderr, "%s\n", log.data());
}

}

int main(int argc, char** argv) {
  if (argc != 2) {
    return 1;
  }

  pumpkintown::Replay replay(argv[1]);
  replay.replay();

  //waffle_window_swap_buffers(window);

  // Clean up.
  // waffle_make_current(dpy, NULL, NULL);
  // waffle_window_destroy(window);
  // //waffle_context_destroy(ctx);
  // waffle_config_destroy(config);
  // waffle_display_disconnect(dpy);

  waffle_teardown();

  return 0;
}
