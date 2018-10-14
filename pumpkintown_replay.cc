#include "pumpkintown_replay.hh"

#include <vector>

#include <stdlib.h>
#include <unistd.h>

#define WAFFLE_API_VERSION 0x0106
#include <waffle.h>

#include "pumpkintown_deserialize.hh"
#include "pumpkintown_gl_functions.hh"
#include "pumpkintown_gl_types.hh"

namespace pumpkintown {

Replay::Replay(Deserialize* deserialize, waffle_window* waffle_window)
    : deserialize_(deserialize), waffle_window_(waffle_window) {}

void Replay::gen_textures() {
  int32_t count{0};
  deserialize_->read(&count);
  std::vector<uint32_t> old_ids;
  for (int32_t i{0}; i < count; i++) {
    uint32_t id{0};
    deserialize_->read(&id);
    old_ids.emplace_back(id);
  }
  std::vector<uint32_t> new_ids;
  new_ids.resize(count);
  glGenTextures(count, new_ids.data());
  for (int32_t i{0}; i < count; i++) {
    texture_ids_[old_ids[i]] = new_ids[i];
  }
}

void Replay::bind_texture() {
  uint32_t target{0};
  deserialize_->read(&target);
  uint32_t old_texture{0};
  deserialize_->read(&old_texture);

  glBindTexture(target, texture_ids_[old_texture]);
}

}

int main() {
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

  pumpkintown::Deserialize deserialize;
  // TODO
  if (!deserialize.open("trace")) {
    fprintf(stderr, "failed to open trace\n");
    exit(1);
  }

  pumpkintown::Replay replay(&deserialize, window);
  while (!deserialize.done()) {
    replay.replay();
  }

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
