#include "pumpkintown_replay.hh"

#include <cassert>
#include <vector>

#include <stdlib.h>
#include <unistd.h>

#define WAFFLE_API_VERSION 0x0106
#include <waffle-1/waffle.h>

#include "pumpkintown_deserialize.hh"
#include "pumpkintown_function_structs.hh"
#include "pumpkintown_gl_enum.hh"
#include "pumpkintown_gl_functions.hh"
#include "pumpkintown_gl_types.hh"
#include "pumpkintown_io.hh"
#include <unistd.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

Replay::Resources::Resources() {
  buffer_ids[0] = 0;
  display_list_ids[0] = 0;
  framebuffer_ids[0] = 0;
  program_ids[0] = 0;
  renderbuffer_ids[0] = 0;
  shader_ids[0] = 0;
  texture_ids[0] = 0;
  vertex_arrays_ids[0] = 0;
}

Replay::Context::Context(waffle_config* config, Context* share_ctx) {
  if (share_ctx) {
    waffle = waffle_context_create(config, share_ctx->waffle);
    r = share_ctx->r;
  } else {
    waffle = waffle_context_create(config, nullptr);
    r = new Resources();
  }
  if (!waffle) {
    throw std::runtime_error("waffle_context_create failed");
  }
}

Replay::Replay(const std::string& path)
    : iter_{path} {
  int32_t platform = WAFFLE_PLATFORM_GLX;
  int32_t api = WAFFLE_CONTEXT_OPENGL;
  int32_t major = 4;
  int32_t minor = 5;

  const char* pt_platform = getenv("PUMPKINTOWN_PLATFORM");
  if (pt_platform && strcmp(pt_platform, "chromeos") == 0) {
    printf("platform: chromeos\n");
    platform = 0x0019;  // "NULL" platform defined in CrOS's fork
    api = WAFFLE_CONTEXT_OPENGL_ES2;
    major = 2;
    minor = 0;
  }

  const int32_t init_attrs[] = {
    WAFFLE_PLATFORM, platform,
    0,
  };

  const int32_t config_attrs[] = {
    WAFFLE_CONTEXT_API,         api,

    WAFFLE_CONTEXT_MAJOR_VERSION, major,
    WAFFLE_CONTEXT_MINOR_VERSION, minor,

    WAFFLE_RED_SIZE,            8,
    WAFFLE_BLUE_SIZE,           8,
    WAFFLE_GREEN_SIZE,          8,
    WAFFLE_ALPHA_SIZE,          8,

    WAFFLE_DEPTH_SIZE, 24,

    WAFFLE_STENCIL_SIZE, 1,

         
    WAFFLE_DOUBLE_BUFFERED, false,
    0,
  };

  const int32_t window_width = 800;
  const int32_t window_height = 600;

  waffle_init(init_attrs);
  display_ = waffle_display_connect(NULL);

  config_ = waffle_config_choose(display_, config_attrs);
  if (!config_) {
    throw std::runtime_error("waffle_config_choose failed");
  }
  window_ = waffle_window_create(config_, window_width, window_height);
  if (!window_) {
    throw std::runtime_error("waffle_window_create failed");
  }
  default_context_ = new Context(config_);
  c_ = default_context_;

  waffle_window_show(window_);
  waffle_make_current(display_, window_, default_context_->waffle);

  glClearColor(0.4, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  sleep(1);
  //waffle_window_swap_buffers(window_);
}

void Replay::capture() {
  GLint current{0};
  glGetIntegerv(GL_RENDERBUFFER_BINDING, &current);
  for (const auto id : c_->r->renderbuffer_ids) {
    glBindRenderbuffer(GL_RENDERBUFFER, id.second);

    int x = 0;
    int y = 0;
    int width = 640;
    int height = 480;
    GLenum format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
    std::vector<uint8_t> data;
    data.resize(width * height * 4);
    glReadPixels(x, y, width, height, format, type, data.data());
    const int comp = 4;
    const int stride = 0;
    static int capture_index = 0;
    stbi_write_png(("rb" + std::to_string(capture_index) + ".png").c_str(),
                   width, height, comp,
                   data.data(), stride);
    capture_index += 1;
  }
  glBindRenderbuffer(GL_RENDERBUFFER, current);
}

void Replay::replay() {
  while (!iter_.done()) {
    iter_.next();
    replay_one();

    if (iter_.function_id() != FunctionId::glGetStringi) {
      check_gl_error();
    } else {
      glGetError();
    }

    //capture();
  }
}

void Replay::custom_glXCreateContext(const FnGlXCreateContext& fn) {
  contexts_[fn.return_value] = new Context(config_);
}

void Replay::custom_glXCreateContextAttribsARB(const FnGlXCreateContextAttribsARB& fn) {
  contexts_[fn.return_value] = new Context(
      config_, fn.share_context ? contexts_.at(fn.share_context) : nullptr);
}

void Replay::custom_glXCreateNewContext(const FnGlXCreateNewContext& fn) {
  contexts_[fn.return_value] = new Context(config_);
}

void Replay::custom_glXMakeContextCurrent(const FnGlXMakeContextCurrent& fn) {
  if (!fn.ctx) {
    waffle_make_current(display_, window_, nullptr);
  } else {
    c_ = contexts_.at(fn.ctx);
    waffle_make_current(display_, window_, c_->waffle);
  }
}

void Replay::custom_glXMakeCurrent(const FnGlXMakeCurrent& fn) {
  if (!fn.ctx) {
    waffle_make_current(display_, window_, nullptr);
  } else {
    c_ = contexts_.at(fn.ctx);
    waffle_make_current(display_, window_, c_->waffle);
  }
}

void Replay::custom_glCreateProgram(const FnGlCreateProgram& fn) {
  const uint32_t new_id{glCreateProgram()};
  c_->r->program_ids[fn.return_value] = new_id;
}

void Replay::custom_glCreateShader(const FnGlCreateShader& fn) {
  const uint32_t new_id{glCreateShader(fn.type)};
  c_->r->shader_ids[fn.return_value] = new_id;
}

void Replay::custom_glDeleteShader(const FnGlDeleteShader& fn) {
  glDeleteShader(c_->r->shader_ids[fn.shader]);
  c_->r->shader_ids.erase(fn.shader);
}

void Replay::custom_glDeleteProgram(const FnGlDeleteProgram& fn) {
  glDeleteProgram(c_->r->program_ids.at(fn.program));
  c_->r->program_ids.erase(fn.program);
}

void Replay::custom_glAttachShader(const FnGlAttachShader& fn) {
  glAttachShader(c_->r->program_ids.at(fn.program), c_->r->shader_ids.at(fn.shader));
}

void Replay::custom_glGenBuffers(const FnGlGenBuffers& fn) {
  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.buffers_length);
  glGenBuffers(fn.buffers_length, new_ids.data());

  for (uint32_t i{0}; i < fn.buffers_length; i++) {
    c_->r->buffer_ids[fn.buffers[i]] = new_ids[i];
  }
}

void Replay::custom_glBindBuffer(const FnGlBindBuffer& fn) {
  if (fn.buffer != 0) {
    assert(c_->r->buffer_ids[fn.buffer] != 0);
  }
  glBindBuffer(fn.target, c_->r->buffer_ids[fn.buffer]);
}

void Replay::custom_glGenTextures(const FnGlGenTextures& fn) {
  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.textures_length);
  glGenTextures(fn.textures_length, new_ids.data());

  for (uint32_t i{0}; i < fn.textures_length; i++) {
    assert(new_ids[i] != 0);
    c_->r->texture_ids[fn.textures[i]] = new_ids[i];
  }
}

void Replay::custom_glBindTexture(const FnGlBindTexture& fn) {
  if (fn.texture != 0) {
    assert(c_->r->texture_ids.at(fn.texture) != 0);
  }
  glBindTexture(fn.target, c_->r->texture_ids.at(fn.texture));
}

void Replay::custom_glDeleteTextures(const FnGlDeleteTextures& fn) {
  std::vector<uint32_t> tex_ids;
  for (uint32_t i{0}; i < fn.textures_length; i++) {
    if (fn.textures[i] != 0) {
      tex_ids.emplace_back(c_->r->texture_ids.at(fn.textures[i]));
      c_->r->texture_ids.erase(fn.textures[i]);
    }
  }
  glDeleteTextures(tex_ids.size(), tex_ids.data());
}

void Replay::custom_glGenFramebuffers(const FnGlGenFramebuffers& fn) {
  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.framebuffers_length);
  glGenFramebuffers(fn.framebuffers_length, new_ids.data());

  for (uint32_t i{0}; i < fn.framebuffers_length; i++) {
    c_->r->framebuffer_ids[fn.framebuffers[i]] = new_ids[i];
  }
}

void Replay::custom_glBindFramebuffer(const FnGlBindFramebuffer& fn) {
  glBindFramebuffer(fn.target, c_->r->framebuffer_ids[fn.framebuffer]);
}

void Replay::custom_glGenVertexArrays(const FnGlGenVertexArrays& fn) {
  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.arrays_length);
  glGenVertexArrays(fn.arrays_length, new_ids.data());

  for (uint32_t i{0}; i < fn.arrays_length; i++) {
    c_->r->vertex_arrays_ids[fn.arrays[i]] = new_ids[i];
  }
}

void Replay::custom_glBindVertexArray(const FnGlBindVertexArray& fn) {
  glBindVertexArray(c_->r->vertex_arrays_ids[fn.array]);
}

void Replay::custom_glGenRenderbuffers(const FnGlGenRenderbuffers& fn) {
  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.renderbuffers_length);
  glGenRenderbuffers(fn.renderbuffers_length, new_ids.data());

  for (uint32_t i{0}; i < fn.renderbuffers_length; i++) {
    c_->r->renderbuffer_ids[fn.renderbuffers[i]] = new_ids[i];
  }
}

void Replay::custom_glBindRenderbuffer(const FnGlBindRenderbuffer& fn) {
  glBindRenderbuffer(fn.target, c_->r->renderbuffer_ids[fn.renderbuffer]);
}

void Replay::custom_glFramebufferTexture2D(const FnGlFramebufferTexture2D& fn) {
  glFramebufferTexture2D(fn.target, fn.attachment,
                         fn.textarget,
                         c_->r->texture_ids[fn.texture], fn.level);
}

void Replay::custom_glGenLists(const FnGlGenLists& fn) {
  const auto new_start = glGenLists(fn.range);

  for (int32_t i{0}; i < fn.range; i++) {
    c_->r->display_list_ids[fn.return_value + i] = new_start + i;
  }
}

void Replay::custom_glNewList(const FnGlNewList& fn) {
  glNewList(c_->r->display_list_ids[fn.list], fn.mode);
}

void Replay::custom_glCallList(const FnGlCallList& fn) {
  glCallList(c_->r->display_list_ids[fn.list]);
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
