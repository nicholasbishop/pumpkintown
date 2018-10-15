#include "pumpkintown_gl_util.hh"
#include "pumpkintown_io.hh"
#include "pumpkintown_serialize.hh"

namespace pumpkintown {

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

void Serialize::write(const uint8_t* value, uint64_t num_bytes) {
  return write_exact(file_, value, num_bytes);
}

void Serialize::write(const int8_t value) {
  return write_exact(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void Serialize::write(const int16_t value) {
  return write_exact(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void Serialize::write(const int32_t value) {
  return write_exact(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void Serialize::write(const uint8_t value) {
  return write_exact(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void Serialize::write(const uint16_t value) {
  return write_exact(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void Serialize::write(const uint32_t value) {
  return write_exact(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void Serialize::write(const uint64_t value) {
  return write_exact(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void Serialize::write(const float value) {
  return write_exact(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void Serialize::write(const double value) {
  return write_exact(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

void Serialize::write(const FunctionId value) {
  // TODO
  if (true) {
    fprintf(stderr, "%s\n", function_id_to_string(value));
  }
  return write_exact(file_, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
}

}
