#include "pumpkintown/parser.hh"

#include <cctype>
#include <stdexcept>

namespace pumpkintown {

NumberParser::NumberParser(const std::string& string)
    : orig_(string) {
  if (string.empty()) {
    throw std::runtime_error("empty number string");
  }
}

bool NumberParser::is_symbolic() const {
  return !std::isdigit(orig_.at(0));
}

std::string NumberParser::symbol() const {
  if (!is_symbolic()) {
    throw std::runtime_error("number is not a symbol");
  }
  return orig_;
}

int32_t NumberParser::as_int32() const {
  ensure_not_symbolic();
  ensure_not_negative();

  return std::stol(orig_, nullptr, 0);
}

uint32_t NumberParser::as_uint32() const {
  ensure_not_symbolic();
  ensure_not_negative();

  return std::stoul(orig_, nullptr, 0);
}

void NumberParser::ensure_not_symbolic() const {
  if (is_symbolic()) {
    throw std::runtime_error("number is symbolic");
  }
}

void NumberParser::ensure_not_negative() const {
  if (orig_.at(0) == '-') {
    throw std::runtime_error("number is negative");
  }
}

bool NumberParser::is_hex() const {
  const auto c0 = orig_.at(0);
  const auto c1 = orig_.at(1);
  const auto c2 = orig_.at(2);
  return (((c0 == '+' || c0 == '-') && c1 == '0' &&
           (c2 == 'x' || c2 == 'X')) ||
          (c0 == '0' && (c1 == 'x' || c1 == 'X')));
}

}
