#ifndef PUMPKINTOWN_REPLAY_HH_
#define PUMPKINTOWN_REPLAY_HH_

struct waffle_window;

namespace pumpkintown {

class Deserialize;

bool replay(Deserialize* deserialize, waffle_window* waffle_window);

}

#endif  // PUMPKINTOWN_REPLAY_HH_
