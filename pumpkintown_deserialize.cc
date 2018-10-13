#include "pumpkintown_deserialize.hh"

#include <cstdio>

namespace pumpkintown {

namespace {

bool read_exact(FILE* f, uint8_t* buf, uint64_t num_bytes) {
  uint64_t bytes_remaining = num_bytes;
  while (bytes_remaining > 0) {
    uint64_t bytes_read = fread(buf, 1, bytes_remaining, f);
    if (bytes_read == 0) {
      return false;
    }
    bytes_remaining -= bytes_read;
    buf += bytes_read;
  }
  return true;
}

}

Deserialize::~Deserialize() {
  if (file_) {
    fclose(file_);
  }
}

bool Deserialize::open(const std::string& path) {
  if (file_) {
    return false;
  }
  file_ = fopen(path.c_str(), "rb");
  return file_ != nullptr;
}

bool Deserialize::done() {
  return feof(file_);
}

// TODO(nicholasbishop): byte ordering

bool Deserialize::read(int8_t* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

bool Deserialize::read(int16_t* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

bool Deserialize::read(int32_t* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

bool Deserialize::read(int64_t* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

bool Deserialize::read(uint8_t* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

bool Deserialize::read(uint16_t* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

bool Deserialize::read(uint32_t* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

bool Deserialize::read(uint64_t* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

bool Deserialize::read(float* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

bool Deserialize::read(double* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

bool Deserialize::read(FunctionId* value) {
  return read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

}
