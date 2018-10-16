#ifndef PUMPKINTOWN_REPLAY_HH_
#define PUMPKINTOWN_REPLAY_HH_

#include <map>

#include "pumpkintown_function_structs.hh"
#include "pumpkintown_io.hh"

struct waffle_window;

namespace pumpkintown {

class Deserialize;

class Replay {
 public:
  Replay(const std::string& path, waffle_window* waffle_window);

  void replay();

  void gen_textures();

  void bind_texture();

  void custom_glGenLists(const FnGlGenLists& fn);
  void custom_glNewList(const FnGlNewList& fn);
  void custom_glCallList(const FnGlCallList& fn);

 private:
  void replay_one();

  TraceIterator iter_;
  waffle_window* waffle_window_;
  std::map<uint32_t, uint32_t> texture_ids_;
  std::map<uint32_t, uint32_t> display_list_ids_;
};

}

#endif  // PUMPKINTOWN_REPLAY_HH_
