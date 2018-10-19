#ifndef PUMPKINTOWN_DUMP_HH_
#define PUMPKINTOWN_DUMP_HH_

#include <string>

#include "pumpkintown_io.hh"

namespace pumpkintown {

class Dump {
 public:
  explicit Dump(const std::string& path);

  void dump();

 private:
  void dump_one();

  TraceIterator iter_;
};  

}

#endif  // PUMPKINTOWN_DUMP_HH_
