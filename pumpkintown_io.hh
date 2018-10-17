#ifndef IO_HH_
#define IO_HH_

#include <cstdio>
#include <cstdint>
#include <string>

#include "pumpkintown_function_id.hh"

namespace pumpkintown {

void read_exact(FILE* f, void* dst, uint64_t num_bytes);
void write_exact(FILE* f, const void* src, uint64_t num_bytes);

std::string to_string(const void* ptr);

template<typename T>
std::string to_string(const T& t) {
  return std::to_string(t);
}

class TraceIterator {
 public:
  explicit TraceIterator(const std::string& path);

  FILE* file() { return file_; }

  FunctionId function_id() const { return function_id_; }

  void next();

  void skip();

  bool done();

 private:
  FILE* file_;
  FunctionId function_id_{FunctionId::Invalid};
  uint64_t item_size_{0};
};

}

#endif  // IO_HH_
