#include <cassert>
#include <cstring>
#include <stdexcept>

#include <epoxy/gl.h>
#include <waffle-1/waffle.h>

#include "replay.hh"

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
