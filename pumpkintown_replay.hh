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

  void custom_glGenBuffers(const FnGlGenBuffers& fn);
  void custom_glBindBuffer(const FnGlBindBuffer& fn);

  void custom_glGenTextures(const FnGlGenTextures& fn);
  void custom_glBindTexture(const FnGlBindTexture& fn);

  void custom_glGenFramebuffers(const FnGlGenFramebuffers& fn);
  void custom_glBindFramebuffer(const FnGlBindFramebuffer& fn);

  void custom_glGenVertexArrays(const FnGlGenVertexArrays& fn);
  void custom_glBindVertexArray(const FnGlBindVertexArray& fn);

  void custom_glFramebufferTexture2D(const FnGlFramebufferTexture2D& fn);

  void custom_glGenLists(const FnGlGenLists& fn);
  void custom_glNewList(const FnGlNewList& fn);
  void custom_glCallList(const FnGlCallList& fn);

 private:
  void replay_one();

  TraceIterator iter_;
  waffle_window* waffle_window_;
  std::map<uint32_t, uint32_t> buffer_ids_;
  std::map<uint32_t, uint32_t> vertex_arrays_ids_;
  std::map<uint32_t, uint32_t> framebuffer_ids_;
  std::map<uint32_t, uint32_t> texture_ids_;
  std::map<uint32_t, uint32_t> display_list_ids_;
};

}

#endif  // PUMPKINTOWN_REPLAY_HH_
