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
    WAFFLE_PLATFORM, WAFFLE_PLATFORM_X11_EGL,
    0,
  };

  const int32_t config_attrs[] = {
    WAFFLE_CONTEXT_API,         WAFFLE_CONTEXT_OPENGL_ES2,

    WAFFLE_RED_SIZE,            8,
    WAFFLE_BLUE_SIZE,           8,
    WAFFLE_GREEN_SIZE,          8,

    0,
  };

  const int32_t window_width = 320;
  const int32_t window_height = 240;

  waffle_init(init_attrs);
  dpy = waffle_display_connect(NULL);

  // Exit if OpenGL ES2 is unsupported.
  if (!waffle_display_supports_context_api(dpy, WAFFLE_CONTEXT_OPENGL_ES2)
      || !waffle_dl_can_open(WAFFLE_DL_OPENGL_ES2))
  {
    exit(EXIT_FAILURE);
  }

  // Get OpenGL functions.
  //glClearColor = waffle_dl_sym(WAFFLE_DL_OPENGL_ES2, "glClearColor");
  //glClear = waffle_dl_sym(WAFFLE_DL_OPENGL_ES2, "glClear");

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
