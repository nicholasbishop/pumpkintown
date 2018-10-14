#ifndef PUMPKINTOWN_REPLAY_HH_
#define PUMPKINTOWN_REPLAY_HH_

#include <map>

struct waffle_window;

namespace pumpkintown {

class Deserialize;

class Replay {
 public:
  Replay(Deserialize* deserialize, waffle_window* waffle_window);

  bool replay();

  void gen_textures();

  void bind_texture();

 private:
  Deserialize* deserialize_;
  waffle_window* waffle_window_;
  std::map<uint32_t, uint32_t> texture_ids_;
};

}

#endif  // PUMPKINTOWN_REPLAY_HH_
