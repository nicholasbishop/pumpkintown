#include "pumpkintown_replay.hh"

#include <vector>

#include <stdlib.h>
#include <unistd.h>

#define WAFFLE_API_VERSION 0x0106
#include <waffle.h>

#include "pumpkintown_deserialize.hh"
#include "pumpkintown_function_structs.hh"
#include "pumpkintown_gl_functions.hh"
#include "pumpkintown_gl_types.hh"
#include "pumpkintown_io.hh"

namespace pumpkintown {

Replay::Replay(const std::string& path, waffle_window* waffle_window)
    : iter_{path}, waffle_window_{waffle_window} {}

void Replay::replay() {
  while (!iter_.done()) {
    iter_.next();
    replay_one();
  }
}

void Replay::gen_textures() {
  FnGlGenTextures fn;
  fn.read(iter_.file());

  std::vector<uint32_t> new_ids;
  new_ids.resize(fn.textures_length);
  glGenTextures(fn.textures_length, new_ids.data());

  for (uint32_t i{0}; i < fn.textures_length; i++) {
    texture_ids_[fn.textures[i]] = new_ids[i];
  }
}

void Replay::bind_texture() {
  FnGlBindTexture fn;
  fn.read(iter_.file());

  glBindTexture(fn.target, texture_ids_[fn.texture]);
}

}

int main(int argc, char** argv) {
  if (argc != 2) {
    return 1;
  }

  struct waffle_display *dpy;
  struct waffle_config *config;
  struct waffle_window *window;
  struct waffle_context *ctx;

  const int32_t init_attrs[] = {
    WAFFLE_PLATFORM, WAFFLE_PLATFORM_GLX,
    0,
  };

  const int32_t config_attrs[] = {
    WAFFLE_CONTEXT_API,         WAFFLE_CONTEXT_OPENGL,

    WAFFLE_RED_SIZE,            8,
    WAFFLE_BLUE_SIZE,           8,
    WAFFLE_GREEN_SIZE,          8,

    WAFFLE_DEPTH_SIZE, 8,

         
    WAFFLE_DOUBLE_BUFFERED, false,

    0,
  };

  const int32_t window_width = 800;
  const int32_t window_height = 600;

  waffle_init(init_attrs);
  dpy = waffle_display_connect(NULL);

  config = waffle_config_choose(dpy, config_attrs);
  window = waffle_window_create(config, window_width, window_height);
  ctx = waffle_context_create(config, NULL);

  waffle_window_show(window);
  waffle_make_current(dpy, window, ctx);

  pumpkintown::Replay replay(argv[1], window);


  //waffle_window_swap_buffers(window);

  // Clean up.
  waffle_make_current(dpy, NULL, NULL);
  waffle_window_destroy(window);
  waffle_context_destroy(ctx);
  waffle_config_destroy(config);
  waffle_display_disconnect(dpy);

  waffle_teardown();

  return 0;
}
