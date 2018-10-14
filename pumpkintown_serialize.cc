#include "pumpkintown_serialize.hh"

namespace pumpkintown {

namespace {

bool write_all(FILE* f, const uint8_t* buf, const uint64_t size) {
  uint64_t bytes_remaining = size;
  while (bytes_remaining > 0) {
    const auto bytes_written = fwrite(buf, 1, bytes_remaining, f);
    if (bytes_written <= 0) {
      return false;
    }
    bytes_remaining -= bytes_written;
    buf += bytes_written;
  }
  return true;
}

}

Serialize::~Serialize() {
  if (file_) {
    fclose(file_);
  }
}

bool Serialize::open(const std::string& path) {
  if (file_) {
    return false;
  }
  file_ = fopen(path.c_str(), "wb");
  return file_ != nullptr;
}

bool Serialize::is_open() {
  return file_ != nullptr;
}

bool Serialize::write(const uint8_t* value, uint64_t num_bytes) {
  return write_all(file_, value, num_bytes);
}

bool Serialize::write(const int8_t value) {
  return write_all(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

bool Serialize::write(const int16_t value) {
  return write_all(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

bool Serialize::write(const int32_t value) {
  return write_all(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

bool Serialize::write(const uint8_t value) {
  return write_all(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

bool Serialize::write(const uint16_t value) {
  return write_all(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

bool Serialize::write(const uint32_t value) {
  return write_all(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

bool Serialize::write(const uint64_t value) {
  return write_all(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

bool Serialize::write(const float value) {
  return write_all(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

bool Serialize::write(const double value) {
  return write_all(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

bool Serialize::write(const FunctionId value) {
  return write_all(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

}
