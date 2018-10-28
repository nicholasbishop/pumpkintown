#ifndef PUMPKINTOWN_PARSER_HH_
#define PUMPKINTOWN_PARSER_HH_

#include <cstdint>
#include <string>

namespace pumpkintown {

class NumberParser {
 public:
  explicit NumberParser(const std::string& string);

  bool is_symbolic() const;
  std::string symbol() const;

  float as_float() const;
  double as_double() const;
  int8_t as_int8() const;
  int16_t as_int16() const;
  int32_t as_int32() const;
  int64_t as_int64() const;
  uint8_t as_uint8() const;
  uint16_t as_uint16() const;
  uint32_t as_uint32() const;
  uint64_t as_uint64() const;

 private:
  void ensure_not_symbolic() const;
  void ensure_not_negative() const;
  bool is_hex() const;

  std::string orig_;
};

}

#endif  // PUMPKINTOWN_PARSER_HH_
