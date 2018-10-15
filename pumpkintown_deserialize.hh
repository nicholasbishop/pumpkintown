#ifndef PUMPKINTOWN_DESERIALIZE_HH_
#define PUMPKINTOWN_DESERIALIZE_HH_

#include <cstdint>
#include <string>

#include "pumpkintown_function_id.hh"

namespace pumpkintown {

class Deserialize {
 public:
  ~Deserialize();

  bool open(const std::string& path);

  bool done();

  uint64_t position();

  void read_u8v(uint8_t* value, uint64_t size);

  int8_t read_i8();
  int16_t read_i16();
  int32_t read_i32();
  int64_t read_i64();

  uint8_t read_u8();
  uint16_t read_u16();
  uint32_t read_u32();
  uint64_t read_u64();

  float read_f32();
  double read_f64();

  FunctionId read_function_id();

  void advance(uint64_t num_bytes);

 private:
  FILE* file_{nullptr};
};

}

#endif  // PUMPKINTOWN_DESERIALIZE_HH_
