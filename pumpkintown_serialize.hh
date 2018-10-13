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

  bool write(int8_t value);
  bool write(int16_t value);
  bool write(int32_t value);
  bool write(int64_t value);

  bool write(uint8_t value);
  bool write(uint16_t value);
  bool write(uint32_t value);
  bool write(uint64_t value);

  bool write(float value);
  bool write(double value);

  bool write(FunctionId value);

 private:
  FILE* file_{nullptr};
};

}

#endif  // PUMPKINTOWN_SERIALIZE_HH_
