#ifndef PUMPKINTOWN_SERIALIZE_HH_
#define PUMPKINTOWN_SERIALIZE_HH_

#include <cstdint>
#include <string>

#include "pumpkintown_function_id.hh"

namespace pumpkintown {

class Serialize {
 public:
  ~Serialize();

  bool open(const std::string& path);

  bool is_open();

  void write(const uint8_t* value, uint64_t num_bytes);

  // TODO, pick an interface already
  FILE* file() { return file_; }

  void write(int8_t value);
  void write(int16_t value);
  void write(int32_t value);
  void write(int64_t value);

  void write(uint8_t value);
  void write(uint16_t value);
  void write(uint32_t value);
  void write(uint64_t value);

  void write(float value);
  void write(double value);

  void write(FunctionId value);

 private:
  FILE* file_{nullptr};
};

}

#endif  // PUMPKINTOWN_SERIALIZE_HH_
