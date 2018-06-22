#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <epoxy/gl.h>
#include <waffle-1/waffle.h>

#include "pumpkintown_file.h"
#include "pumpkintown_types.h"

void pumpkintown_replay_command_one(const char* name, PumpkintownTypeUnion* args, const int num_args);

PumpkintownFileTag read_tag(FILE* file, bool* err) {
  return static_cast<PumpkintownFileTag>(read_uint32(file, err));
}
bool pumpkintown_replay_command(FILE *file) {
  bool err = false;
  PumpkintownFileTag tag = read_tag(file, &err);
  if (err) {
    return false;
  }

  if (tag != PUMPKINTOWN_FILE_TAG_FUNCTION) {
    return false;
  }

  uint32_t name_len = read_uint32(file, &err);
  std::vector<char> name;
  name.resize(name_len + 1);
  fread(name.data(), name_len, 1, file);
  name[name_len] = 0;
  printf("%s\n", name.data());

  PumpkintownTypeUnion args[16];
  int num_args = 0;

  while (true) {
    tag = read_tag(file, &err);
    if (err) {
      return false;
    }

    if (tag == PUMPKINTOWN_FILE_TAG_END_FUNCTION) {
      break;
    } else if (tag == PUMPKINTOWN_FILE_TAG_ARG) {
      fread(&args[num_args], sizeof(PumpkintownTypeUnion), 1, file);
      num_args++;
    }
  }

  pumpkintown_replay_command_one(name.data(), args, num_args);

  return true;
}

int main() {
  FILE* file = fopen("trace", "rb");

  const int32_t init_attrs[] = {
    WAFFLE_PLATFORM, WAFFLE_PLATFORM_X11_EGL,
    0,
  };

  const int32_t config_attrs[] = {
    WAFFLE_CONTEXT_API,         WAFFLE_CONTEXT_OPENGL,

    WAFFLE_RED_SIZE,            8,
    WAFFLE_BLUE_SIZE,           8,
    WAFFLE_GREEN_SIZE,          8,

    WAFFLE_DOUBLE_BUFFERED,     false,

    0,
  };

  waffle_init(init_attrs);

  struct waffle_display *dpy = waffle_display_connect(NULL);

  struct waffle_config *config = waffle_config_choose(dpy, config_attrs);

  struct waffle_window *window = waffle_window_create(config, 640, 480);
  struct waffle_context *ctx =  waffle_context_create(config, NULL);

  waffle_window_show(window);
  waffle_make_current(dpy, window, ctx);

  while (true) {
    if (!pumpkintown_replay_command(file)) {
      break;
    }
    glFinish();
    waffle_window_swap_buffers(window);
  }

  return 0;
}
