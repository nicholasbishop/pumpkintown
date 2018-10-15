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

template<typename T>
T read_value(FILE* f) {
  T t;
  read_exact(f, reinterpret_cast<uint8_t*>(&t), sizeof(T));
  return t;
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

uint64_t Deserialize::position() {
  return ftell(file_);
}

// TODO(nicholasbishop): byte ordering

void Deserialize::read_u8v(uint8_t* value, uint64_t size) {
  read_exact(file_, value, size);
}

int8_t Deserialize::read_i8() {
  return read_value<int8_t>(file_);
}

int16_t Deserialize::read_i16() {
  return read_value<int16_t>(file_);
}

int32_t Deserialize::read_i32() {
  return read_value<int32_t>(file_);
}

int64_t Deserialize::read_i64() {
  return read_value<int64_t>(file_);
}

uint8_t Deserialize::read_u8() {
  return read_value<uint8_t>(file_);
}

uint16_t Deserialize::read_u16() {
  return read_value<uint16_t>(file_);
}

uint32_t Deserialize::read_u32() {
  return read_value<uint32_t>(file_);
}

uint64_t Deserialize::read_u64() {
  return read_value<uint64_t>(file_);
}

float Deserialize::read_f32() {
  return read_value<float>(file_);
}

double Deserialize::read_f64() {
  return read_value<double>(file_);
}

FunctionId Deserialize::read_function_id() {
  FunctionId value{FunctionId::Invalid};
  read_exact(file_, reinterpret_cast<uint8_t*>(&value), sizeof(value));
  return value;
}

void Deserialize::advance(uint64_t num_bytes) {
  if (num_bytes == 0) {
    return;
  }
  long offset = num_bytes;
  if (fseek(file_, offset, SEEK_CUR) != 0) {
    printf("error: %d\n", errno);
    throw std::runtime_error("seek failed");
  }
}

}
