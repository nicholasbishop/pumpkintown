#include "pumpkintown_io.hh"

#include <stdexcept>

namespace pumpkintown {

void read_exact(FILE* f, void* dst, uint64_t const num_bytes) {
  uint8_t* buf = reinterpret_cast<uint8_t*>(dst);
  uint64_t bytes_remaining = num_bytes;
  while (bytes_remaining > 0) {
    uint64_t bytes_read = fread(dst, 1, bytes_remaining, f);
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

TraceIterator::TraceIterator(const std::string& path) {
  file_ = fopen(path.c_str(), "rb");
  if (!file_) {
    throw std::runtime_error("open failed");
  }
}

void TraceIterator::next() {
  read_exact(file_, &function_id_, sizeof(FunctionId));
  read_exact(file_, &item_size_, sizeof(uint64_t));
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
