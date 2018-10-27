#include "pumpkintown/text_trace_iterator.hh"

namespace pumpkintown {

namespace {

std::vector<std::string> split_string(const std::string& str) {
  std::vector<std::string> vec;
  std::string word;
  for (const auto c : str) {
    if (c == ' ' || c == '\t') {
      if (!word.empty()) {
        vec.emplace_back(word);
        word.clear();
      }
    } else {
      word += c;
    }
  }
  if (!word.empty()) {
    vec.emplace_back(word);
  }
  return vec;
}

}

TextTraceIterator::TextTraceIterator(const std::string& path)
    : file_(path) {}

bool TextTraceIterator::next() {
  std::string line;
  if (!std::getline(file_, line)) {
    return false;
  }

  auto parts{split_string(line)};

  if (parts.size() < 1) {
    return next();
  }

  function_ = parts[0];
  parts.erase(parts.begin());

  if (parts.size() >= 2 && parts[parts.size() - 2] == "->") {
    return_value_ = parts[parts.size() - 1];
    parts.resize(parts.size() - 2);
  }

  args_ = parts;

  return true;
}

}
