#include <stdexcept>
#include <string>
#include <vector>

#include "pumpkintown_dump.hh"
#include "pumpkintown_schema_generated.h"

void read_exact(FILE* f, uint8_t* buf, const uint64_t size) {
  uint64_t bytes_remaining = size;
  while (bytes_remaining > 0) {
    uint64_t bytes_read = fread(buf, 1, size, f);
    if (bytes_read == 0) {
      fclose(f);
      throw std::runtime_error("read failed");
    }
    bytes_remaining -= bytes_read;
    buf += bytes_read;
  }
}

void read_item(FILE* f, std::vector<uint8_t>* vec) {
  uint64_t size{0};
  read_exact(f, reinterpret_cast<uint8_t*>(&size), sizeof(size));
  vec->resize(size);
  read_exact(f, vec->data(), size);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    // TODO
    return 1;
  }

  FILE* f = fopen(argv[1], "rb");
  std::vector<uint8_t> vec;
  while (!feof(f)) {
    read_item(f, &vec);
    auto trace_item = pumpkintown::GetTraceItem(vec.data());
    handle_trace_item(*trace_item);
  }
  
  fclose(f);
}
