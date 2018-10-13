#include "pumpkintown_dump.hh"

#include <stdexcept>
#include <string>
#include <vector>

#include "pumpkintown_deserialize.hh"

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

int main(int argc, char** argv) {
  if (argc != 2) {
    // TODO
    return 1;
  }

  pumpkintown::Deserialize d;
  if (!d.open(argv[1])) {
    fprintf(stderr, "failed to open %s\n", argv[1]);
    return 1;
  }

  while (!d.done()) {
    if (!handle_trace_item(&d)) {
      fprintf(stderr, "handle_trace_item failed\n");
    }
  }
}
