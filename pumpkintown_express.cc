#include "pumpkintown_express.hh"

#include <cstring>
#include <sstream>

#include <waffle-1/waffle.h>

namespace pumpkintown {

namespace {

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

    dispatch();
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
}

const void* Express::to_offset(const std::string& str) {
  std::istringstream iss{str};
  uint64_t val;
  iss >> val;
  return reinterpret_cast<const void*>(val);
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
  std::istringstream iss{str};
  int32_t val;
  iss >> val;
  return val;
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
  std::istringstream iss{str};
  uint32_t val;
  iss >> val;
  return val;
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
