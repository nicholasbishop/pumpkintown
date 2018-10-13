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

  bool read(int8_t* value);
  bool read(int16_t* value);
  bool read(int32_t* value);
  bool read(int64_t* value);

  bool read(uint8_t* value);
  bool read(uint16_t* value);
  bool read(uint32_t* value);
  bool read(uint64_t* value);

  bool read(float* value);
  bool read(double* value);

  bool read(FunctionId* value);

 private:
  FILE* file_{nullptr};
};

}

#endif  // PUMPKINTOWN_DESERIALIZE_HH_
