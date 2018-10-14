#include "pumpkintown_deserialize.hh"

#include <cstdio>
#include <stdexcept>

namespace pumpkintown {

namespace {

void read_exact(FILE* f, uint8_t* buf, uint64_t num_bytes) {
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

void Deserialize::read(uint8_t* value, uint64_t size) {
  read_exact(file_, value, size);
}

void Deserialize::read(int8_t* value) {
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

void Deserialize::read(int16_t* value) {
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

void Deserialize::read(int32_t* value) {
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

void Deserialize::read(int64_t* value) {
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

void Deserialize::read(uint8_t* value) {
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

void Deserialize::read(uint16_t* value) {
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

void Deserialize::read(uint32_t* value) {
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

void Deserialize::read(uint64_t* value) {
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

void Deserialize::read(float* value) {
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

void Deserialize::read(double* value) {
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(*value));
}

FunctionId Deserialize::read_function_id() {
  FunctionId value{FunctionId::Invalid};
  read_exact(file_, reinterpret_cast<uint8_t*>(value), sizeof(value));
  return value;
}

}
