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

  void read(uint8_t* value, uint64_t size);

  void read(int8_t* value);
  void read(int16_t* value);
  void read(int32_t* value);
  void read(int64_t* value);

  void read(uint8_t* value);
  void read(uint16_t* value);
  void read(uint32_t* value);
  void read(uint64_t* value);

  void read(float* value);
  void read(double* value);

  FunctionId read_function_id();

 private:
  FILE* file_{nullptr};
};

}

#endif  // PUMPKINTOWN_DESERIALIZE_HH_
