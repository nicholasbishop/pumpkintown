#include <cassert>
#include <cstring>
#include <stdexcept>

#include <unistd.h>

#include <epoxy/gl.h>
#include <waffle-1/waffle.h>

#include "replay.hh"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

std::vector<uint8_t> read_all(const std::string& path) {
  FILE* file{fopen(path.c_str(), "rb")};
  assert(file);

  fseek(file, 0, SEEK_END);
  auto num_bytes{ftell(file)};
  fseek(file, 0, SEEK_SET);

  std::vector<uint8_t> vec;
  vec.resize(num_bytes);

  // TODO
  uint8_t* buf = reinterpret_cast<uint8_t*>(vec.data());
  uint64_t bytes_remaining = num_bytes;
  while (bytes_remaining > 0) {
    uint64_t bytes_read = fread(buf, 1, bytes_remaining, file);
    if (bytes_read == 0) {
      throw std::runtime_error("read failed");
    }
    bytes_remaining -= bytes_read;
    buf += bytes_read;
  }

  return vec;
}

void check_program_link(const GLuint program) {
  GLint success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (success) {
    return;
  }

  fprintf(stderr, "link failed:\n");

  GLint length = 0;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
  std::vector<char> log(length);
  glGetProgramInfoLog(program, length, &length, log.data());

  fprintf(stderr, "%s\n", log.data());
}

void check_shader_compile(const GLuint shader) {
  // Print shader source
  if (true) {
    GLint srclen = 0;
    glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &srclen);
    std::vector<char> src(srclen);
    glGetShaderSource(shader, srclen, &srclen, src.data());
    fprintf(stderr, "shader source: %s\n", src.data());
  }

  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success) {
    return;
  }

  fprintf(stderr, "compile failed:\n");

  GLint length = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
  std::vector<char> log(length);
  glGetShaderInfoLog(shader, length, &length, log.data());

  fprintf(stderr, "%s\n", log.data());
}

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

int main() {
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
  } else if (pt_platform && strcmp(pt_platform, "egl") == 0) {
    platform = WAFFLE_PLATFORM_X11_EGL;
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

    //WAFFLE_CONTEXT_DEBUG, true,

    0,
  };

  const int32_t window_width = 1280;
  const int32_t window_height = 1024;

  waffle_init(init_attrs);
  waffle_display* display = waffle_display_connect(NULL);
  assert(display);

  waffle_config* config = waffle_config_choose(display, config_attrs);
  assert(config);
  waffle_window* window = waffle_window_create(config, window_width, window_height);
  assert(window);

  waffle_window_show(window);

  draw(display, window, config);
}

void capture_fb(const std::string& path) {
  GLint orig_read_fb{0}, orig_draw_fb{0};
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &orig_read_fb);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &orig_draw_fb);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, orig_draw_fb);

  GLint type{0};
  glGetFramebufferAttachmentParameteriv(
      GL_READ_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
      &type);

  if (type != GL_TEXTURE) {
    fprintf(stderr, "capture: attachment is not a texture\n");
    return;
  }

  GLint tex_id{0};
  glGetFramebufferAttachmentParameteriv(
      GL_READ_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
      &tex_id);

  if (tex_id == 0) {
    fprintf(stderr, "capture: bad texture ID\n");
    return;
  }

  GLint tex_level{0};
  glGetFramebufferAttachmentParameteriv(
      GL_READ_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,
      &tex_level);

  if (tex_level != 0) {
    fprintf(stderr, "capture: bad texture level\n");
    return;
  }

  GLint orig_tex{0};
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &orig_tex);

  GLint width{0}, height{0};
  glBindTexture(GL_TEXTURE_2D, tex_id);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, tex_level, GL_TEXTURE_WIDTH,
                           &width);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, tex_level, GL_TEXTURE_HEIGHT,
                           &height);
  glBindTexture(GL_TEXTURE_2D, orig_tex);

  const int comp = 4;
  std::vector<uint8_t> pixels;
  pixels.resize(width * height * comp);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  glBindFramebuffer(GL_READ_FRAMEBUFFER, orig_read_fb);

  check_gl_error();

  bool boring = true;
  for (const auto p : pixels) {
    if (p != 0 && p != 255) {
      boring = false;
      break;
    }
  }

  if (boring) {
    return;
  }

  const int stride = width * 4;
  stbi_write_png(path.c_str(), width, height, comp, pixels.data(), stride);
}

void probe2(waffle_window* window) {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 43);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  const int srcX0 = 0;
  const int srcY0 = 0;
  const int srcX1 = 1000;
  const int srcY1 = 1000;
  const int dstX0 = 0;
  const int dstY0 = 0;
  const int dstX1 = 1000;
  const int dstY1 = 1000;
  glBlitFramebuffer(
     srcX0, srcY0, srcX1, srcY1,
     dstX0, dstY0, dstX1, dstY1,
     GL_COLOR_BUFFER_BIT, GL_NEAREST);
  waffle_window_swap_buffers(window);
  sleep(1);
}

void probe(waffle_window* window) {
  const auto program = glCreateProgram();
  const auto vs = glCreateShader(GL_VERTEX_SHADER);
  const auto fs = glCreateShader(GL_FRAGMENT_SHADER);

  const char* vs_src =
      "attribute vec2 pos;"
      "void main() { gl_Position = vec4(pos, 0, 1); }";

  const char* fs_src =
      "void main() { gl_FragColor = vec4(1, 0, 0, 1); }";

  glShaderSource(vs, 1, &vs_src, nullptr);
  glCompileShader(vs);
  check_shader_compile(vs);

  glShaderSource(fs, 1, &fs_src, nullptr);
  glCompileShader(fs);
  check_shader_compile(fs);

  glAttachShader(program, vs);
  glAttachShader(program, fs);

  glLinkProgram(program);
  check_program_link(program);

  //glUseProgram(program);

  GLuint arr{0};
  glGenVertexArrays(1, &arr);
  glBindVertexArray(arr);

  GLuint buf{0};
  glGenBuffers(1, &buf);
  glBindBuffer(GL_ARRAY_BUFFER, buf);

  const float o = 0.6;
  float verts[2 * 3 * 2] = {
    -o, -o,
     o, -o,
    -o,  o,

     o, -o,
    -o,  o,
     o,  o
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 3 * 2,
               verts, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);

  glDrawArrays(GL_TRIANGLES, 0, 2 * 3 * 2);

  glDeleteShader(vs);
  glDeleteShader(fs);
  glDeleteProgram(program);

  check_gl_error();

  waffle_window_swap_buffers(window);

  sleep(1);
}
