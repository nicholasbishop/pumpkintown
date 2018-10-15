#include <vector>

#include "pumpkintown_deserialize.hh"
#include "pumpkintown_dlib.hh"
#include "pumpkintown_gl_util.hh"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace pumpkintown;

int main(int argc, char** argv) {
  if (argc != 2) {
    // TODO
    return 1;
  }

  pumpkintown::Deserialize deserialize;
  // TODO
  if (!deserialize.open(argv[1])) {
    fprintf(stderr, "failed to open trace\n");
    exit(1);
  }

  int call_no{-1};

  while (!deserialize.done()) {
    call_no++;
    FunctionId function_id{deserialize.read_function_id()};
    // printf("%s, pos=%lu\n", function_id_to_string(function_id),
    //        deserialize.position());

    uint64_t msg_size{0};
    deserialize.read(&msg_size);

    if (function_id != FunctionId::glTexImage2D) {
      deserialize.advance(msg_size);
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
    printf("%d, %d\n", width, height);
    int32_t border;
    deserialize.read(&border);
    uint32_t format;
    deserialize.read(&format);
    uint32_t type;
    deserialize.read(&type);
    std::vector<uint8_t> pixels;
    uint64_t pixels_num_bytes = 0;
    deserialize.read(&pixels_num_bytes);
    if (pixels_num_bytes == 0) {
      continue;
    }

    pixels.resize(pixels_num_bytes);
    deserialize.read(pixels.data(), pixels_num_bytes);

    const auto num_components{gl_texture_format_num_components(format)};

    const std::string output{"tex_" + std::to_string(call_no) + ".png"};
    const int stride{0};  // TODO
    stbi_write_png(output.c_str(), width, height,
                   num_components, pixels.data(), stride);
  }
}
