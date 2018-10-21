#include "pumpkintown_io.hh"

#include <sstream>
#include <stdexcept>
#include <vector>

namespace pumpkintown {

void read_exact(FILE* f, void* dst, uint64_t const num_bytes) {
  uint8_t* buf = reinterpret_cast<uint8_t*>(dst);
  uint64_t bytes_remaining = num_bytes;
  while (bytes_remaining > 0) {
    uint64_t bytes_read = fread(buf, 1, bytes_remaining, f);
    if (bytes_read == 0) {
      throw std::runtime_error("read failed");
    }
    bytes_remaining -= bytes_read;
    buf += bytes_read;
  }
}

void write_exact(FILE* f, const void* src, const uint64_t num_bytes) {
  const uint8_t* buf = reinterpret_cast<const uint8_t*>(src);
  uint64_t bytes_remaining = num_bytes;
  while (bytes_remaining > 0) {
    const auto bytes_written = fwrite(buf, 1, bytes_remaining, f);
    if (bytes_written <= 0) {
      throw std::runtime_error("write failed");
    }
    bytes_remaining -= bytes_written;
    buf += bytes_written;
  }
}

std::string to_string(const void* ptr) {
  std::ostringstream oss;
  oss << ptr;
  return oss.str();
}

TraceIterator::TraceIterator(const std::string& path) {
  file_ = fopen(path.c_str(), "rb");
  if (!file_) {
    throw std::runtime_error("open failed");
  }
}

void TraceIterator::next() {
  MsgType msg_type{MsgType::Invalid};
  read_exact(file_, &msg_type, sizeof(uint8_t));

  switch (msg_type) {
    case MsgType::FunctionId:
      {
        uint16_t function_id{0};
        read_exact(file_, &function_id, sizeof(uint16_t));
        uint8_t name_len{0};
        read_exact(file_, &name_len, sizeof(uint8_t));
        std::vector<uint8_t> name;
        name.resize(name_len);
        read_exact(file_, name.data(), name_len);

        std::string name_str{name.begin(), name.end()};
        function_map_[function_id] = function_id_from_name(name_str);

        next();
        break;
      }

    case MsgType::Call:
      {
        uint16_t function_id{0};
        read_exact(file_, &function_id, sizeof(uint16_t));
        function_id_ = function_map_.at(function_id);
        read_exact(file_, &item_size_, sizeof(uint64_t));
        break;
      }

    default:
      throw std::runtime_error("invalid message");
  }
}

void TraceIterator::skip() {
  if (fseek(file_, item_size_, SEEK_CUR) != 0) {
    throw std::runtime_error("seek failed");
  }
}

bool TraceIterator::done() {
  return feof(file_);
}

}
