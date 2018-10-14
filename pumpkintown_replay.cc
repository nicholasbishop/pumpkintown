#include "pumpkintown_replay.hh"

#include <stdlib.h>
#include <unistd.h>

#define WAFFLE_API_VERSION 0x0106
#include <waffle.h>

#include "pumpkintown_deserialize.hh"

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

  const int32_t window_width = 320;
  const int32_t window_height = 240;

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

  while (!deserialize.done()) {
    pumpkintown::replay(&deserialize, window);
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
