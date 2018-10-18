#ifndef PUMPKINTOWN_REPLAY_HH_
#define PUMPKINTOWN_REPLAY_HH_

#include <map>
#include <vector>

#include "pumpkintown_function_structs.hh"
#include "pumpkintown_io.hh"

struct waffle_config;
struct waffle_context;
struct waffle_display;
struct waffle_window;

namespace pumpkintown {

class Deserialize;

class Replay {
 public:
  explicit Replay(const std::string& path);

  void replay();

  void custom_glXCreateContext(const FnGlXCreateContext& fn);
  void custom_glXCreateContextAttribsARB(const FnGlXCreateContextAttribsARB& fn);
  void custom_glXCreateNewContext(const FnGlXCreateNewContext& fn);
  void custom_glXMakeContextCurrent(const FnGlXMakeContextCurrent& fn);
  void custom_glXMakeCurrent(const FnGlXMakeCurrent& fn);

  void custom_glCreateProgram(const FnGlCreateProgram& fn);
  void custom_glCreateShader(const FnGlCreateShader& fn);
  void custom_glAttachShader(const FnGlAttachShader& fn);
  void custom_glDeleteShader(const FnGlDeleteShader& fn);
  void custom_glDeleteProgram(const FnGlDeleteProgram& fn);

  void custom_glGenBuffers(const FnGlGenBuffers& fn);
  void custom_glBindBuffer(const FnGlBindBuffer& fn);

  void custom_glGenTextures(const FnGlGenTextures& fn);
  void custom_glBindTexture(const FnGlBindTexture& fn);
  void custom_glDeleteTextures(const FnGlDeleteTextures& fn);

  void custom_glGenFramebuffers(const FnGlGenFramebuffers& fn);
  void custom_glBindFramebuffer(const FnGlBindFramebuffer& fn);

  void custom_glGenVertexArrays(const FnGlGenVertexArrays& fn);
  void custom_glBindVertexArray(const FnGlBindVertexArray& fn);

  void custom_glGenRenderbuffers(const FnGlGenRenderbuffers& fn);
  void custom_glBindRenderbuffer(const FnGlBindRenderbuffer& fn);

  void custom_glFramebufferTexture2D(const FnGlFramebufferTexture2D& fn);

  void custom_glGenLists(const FnGlGenLists& fn);
  void custom_glNewList(const FnGlNewList& fn);
  void custom_glCallList(const FnGlCallList& fn);

  void custom_glCompileShader(const FnGlCompileShader& fn);
  void custom_glLinkProgram(const FnGlLinkProgram& fn);

 private:
  void replay_one();

  void capture();

  TraceIterator iter_;
  waffle_window* window_{nullptr};
  waffle_config* config_{nullptr};
  waffle_display* display_{nullptr};

  struct Resources {
    Resources();

    std::map<uint32_t, uint32_t> buffer_ids;
    std::map<uint32_t, uint32_t> display_list_ids;
    std::map<uint32_t, uint32_t> framebuffer_ids;
    std::map<uint32_t, uint32_t> program_ids;
    std::map<uint32_t, uint32_t> renderbuffer_ids;
    std::map<uint32_t, uint32_t> shader_ids;
    std::map<uint32_t, uint32_t> texture_ids;
    std::map<uint32_t, uint32_t> vertex_arrays_ids;
  };

  struct Context {
    Context(waffle_config* config, Context* share_ctx=nullptr);

    waffle_context* waffle{nullptr};
    Resources* r{nullptr};
    
  };
  Context* default_context_{nullptr};
  std::map<const void*, Context*> contexts_;
  Context* c_{nullptr};
};

}

#endif  // PUMPKINTOWN_REPLAY_HH_
