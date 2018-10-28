#ifndef PUMPKINTOWN_EXPRESS_HH_
#define PUMPKINTOWN_EXPRESS_HH_

#include <cstdint>
#include <map>
#include <string>

#include "pumpkintown/text_trace_iterator.hh"
#include "pumpkintown_any.hh"

struct waffle_config;
struct waffle_context;
struct waffle_display;
struct waffle_window;

namespace pumpkintown {

class Express {
 public:
  explicit Express(const std::string& path);

  void replay();

 private:
  const std::string& arg(int n) { return iter_.args().at(n); }

  void dispatch();

  void create_context();

  void make_context_current();

  void shader_source();

  void capture_fb(const std::string& path);

  void load_array();

  const void* to_offset(const std::string& str);
  char to_char(const std::string& str);
  float to_float(const std::string& str);
  double to_double(const std::string& str);
  int8_t to_int8(const std::string& str);
  int16_t to_int16(const std::string& str);
  int32_t to_int32(const std::string& str);
  int64_t to_int64(const std::string& str);
  uint8_t to_uint8(const std::string& str);
  uint16_t to_uint16(const std::string& str);
  uint32_t to_uint32(const std::string& str);
  uint64_t to_uint64(const std::string& str);

  TextTraceIterator iter_;
  waffle_display* display_{nullptr};
  waffle_config* config_{nullptr};
  waffle_window* window_{nullptr};

  std::map<std::string, waffle_context*> contexts_;
  std::map<std::string, Any> vars_;
  std::map<std::string, std::vector<char>> char_arrays_;
  std::map<std::string, std::vector<float>> float_arrays_;
  std::map<std::string, std::vector<int32_t>> int32_arrays_;
  std::map<std::string, std::vector<uint8_t>> uint8_arrays_;
  std::map<std::string, std::vector<uint32_t>> uint32_arrays_;
};

}

#endif  // PUMPKINTOWN_EXPRESS_HH_
