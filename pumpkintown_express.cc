#include "pumpkintown_express.hh"

#include <cstring>
#include <sstream>

#include <epoxy/gl.h>
#include <waffle-1/waffle.h>

#include "pumpkintown/parser.hh"

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
  if (false) {
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

std::vector<uint8_t> read_file(const std::string& path) {
  std::ifstream f{path};
  f.seekg(0, std::ios::end);
  const auto size{f.tellg()};

  std::vector<uint8_t> vec;
  vec.resize(size);

  f.seekg(0);
  f.read(reinterpret_cast<char*>(vec.data()), size); 
  return vec;
}

}

Express::Express(const std::string& path)
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

  waffle_window_show(window_);
}

void Express::replay() {
  while (iter_.next()) {
    fprintf(stderr, "%s\n", iter_.function().c_str());

    for (const auto& arg : iter_.args()) {
      if (arg.substr(0, 5) == "file:") {
        uint8_arrays_[arg] = read_file(arg.substr(5));
      }
    }

    if (iter_.function() == "array") {
      load_array();
    } else {
      dispatch();
    }

    if (iter_.function() == "glCompileShader") {
      check_shader_compile(to_uint32(arg(0)));
    } else if (iter_.function() == "glLinkProgram") {
      check_program_link(to_uint32(arg(0)));
    }

    check_gl_error();
  }
}

void Express::create_context() {
  const auto share_ctx_name{iter_.args()[0]};
  waffle_context* share_ctx{nullptr};
  if (share_ctx_name != "null") {
    share_ctx = contexts_.at(share_ctx_name);
  }
  auto* ctx{waffle_context_create(config_, share_ctx)};
  contexts_[iter_.return_value()] = ctx;
}

void Express::make_context_current() {
  const auto ctx_name{iter_.args()[0]};
  waffle_context* ctx{nullptr};
  if (ctx_name != "null") {
    ctx = contexts_.at(ctx_name);
  }
  waffle_make_current(display_, window_, ctx);
}

void Express::shader_source() {
  const auto shader{vars_.at(arg(0)).as_uint32()};
  const GLchar* src = reinterpret_cast<const char*>(
      uint8_arrays_[arg(1)].data());
  glShaderSource(shader, 1, &src, nullptr);
}

void Express::capture_fb(const std::string& path) {
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

void Express::load_array() {
#define LOAD_ARRAY(map_, type_, conv_)              \
  map_[name] = std::vector<type_>();                \
  auto& vec = map_[name];                           \
  vec.resize(iter_.args().size() - 2);              \
  for (size_t i{2}; i < iter_.args().size(); i++) { \
    vec[i - 2] = conv_(arg(i));                     \
  }

  const auto& name = arg(0);
  const auto& type = arg(1);
  if (type == "float") {
    LOAD_ARRAY(float_arrays_, float, to_float);
  } else if (type == "char") {
    LOAD_ARRAY(char_arrays_, char, to_char);
  } else if (type == "int32_t") {
    LOAD_ARRAY(int32_arrays_, int32_t, to_int32);
  } else if (type == "uint32_t") {
    LOAD_ARRAY(uint32_arrays_, uint32_t, to_uint32);
  } else {
    throw std::runtime_error("invalid array type: " + type);
  }
#undef LOAD_ARRAY
}

const void* Express::to_offset(const std::string& str) {
  std::istringstream iss{str};
  uint64_t val;
  iss >> val;
  return reinterpret_cast<const void*>(val);
}

char Express::to_char(const std::string& str) {
  std::istringstream iss{str};
  char val;
  iss >> val;
  return val;
}

float Express::to_float(const std::string& str) {
  std::istringstream iss{str};
  float val;
  iss >> val;
  return val;
}

double Express::to_double(const std::string& str) {
  std::istringstream iss{str};
  double val;
  iss >> val;
  return val;
}

int8_t Express::to_int8(const std::string& str) {
  std::istringstream iss{str};
  int8_t val;
  iss >> val;
  return val;
}

int16_t Express::to_int16(const std::string& str) {
  std::istringstream iss{str};
  int16_t val;
  iss >> val;
  return val;
}

int32_t Express::to_int32(const std::string& str) {
  NumberParser np{str};
  if (np.is_symbolic()) {
    return vars_[np.symbol()].as_int32();
  } else {
    return np.as_int32();
  }
}

int64_t Express::to_int64(const std::string& str) {
  std::istringstream iss{str};
  int64_t val;
  iss >> val;
  return val;
}

uint8_t Express::to_uint8(const std::string& str) {
  std::istringstream iss{str};
  uint8_t val;
  iss >> val;
  return val;
}

uint16_t Express::to_uint16(const std::string& str) {
  std::istringstream iss{str};
  uint16_t val;
  iss >> val;
  return val;
}

uint32_t Express::to_uint32(const std::string& str) {
  NumberParser np{str};
  if (np.is_symbolic()) {
    return vars_[np.symbol()].as_uint32();
  } else {
    return np.as_uint32();
  }
}

uint64_t Express::to_uint64(const std::string& str) {
  std::istringstream iss{str};
  uint64_t val;
  iss >> val;
  return val;
}

}

int main(int argc, char** argv) {
  if (argc != 2) {
    return 1;
  }

  pumpkintown::Express express(argv[1]);
  express.replay();
}
