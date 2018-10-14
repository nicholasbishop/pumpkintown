#include <vector>

#include "pumpkintown_deserialize.hh"

using namespace pumpkintown;

int main(int argc, char** argv) {
  if (argc != 2) {
    // TODO
    return 1;
  }

  pumpkintown::Deserialize deserialize;
  // TODO
  if (!deserialize.open("trace")) {
    fprintf(stderr, "failed to open trace\n");
    exit(1);
  }

  while (!deserialize.done()) {
    FunctionId function_id{deserialize.read_function_id()};
    if (function_id != FunctionId::glTexImage2D) {
      continue;
    }

    uint32_t target;
    deserialize.read(&target);
    int32_t level;
    deserialize.read(&level);
    int32_t internalformat;
    deserialize.read(&internalformat);
    int32_t width;
    deserialize.read(&width);
    int32_t height;
    deserialize.read(&height);
    int32_t border;
    deserialize.read(&border);
    uint32_t format;
    deserialize.read(&format);
    uint32_t type;
    deserialize.read(&type);
    std::vector<uint8_t> pixels;
    uint64_t pixels_num_bytes = 0;
    pixels.resize(pixels_num_bytes);
    deserialize.read(&pixels_num_bytes);
    deserialize.read(pixels.data(), pixels_num_bytes);
  }
}
