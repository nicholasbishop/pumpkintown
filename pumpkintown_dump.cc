#include "pumpkintown_dump.hh"

namespace pumpkintown {

Dump::Dump(const std::string& path)
    : iter_(path) {}

void Dump::dump() {
  while (!iter_.done()) {
    iter_.next();
    dump_one();
  }
}

}

int main(int argc, char** argv) {
  if (argc != 2) {
    // TODO
    return 1;
  }

  pumpkintown::Dump dump(argv[1]);
  dump.dump();
}
