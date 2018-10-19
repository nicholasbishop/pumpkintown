#include <cassert>
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
  const int32_t init_attrs[] = {
    WAFFLE_PLATFORM, WAFFLE_PLATFORM_GLX,
    0,
  };

  const int32_t config_attrs[] = {
    WAFFLE_CONTEXT_API,         WAFFLE_CONTEXT_OPENGL,

#if 1
    WAFFLE_CONTEXT_MAJOR_VERSION, 4,
    WAFFLE_CONTEXT_MINOR_VERSION, 5,

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
