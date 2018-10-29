#ifndef PUMPKINTOWN_PATH_HH_
#define PUMPKINTOWN_PATH_HH_

#include <stdexcept>
#include <string>

namespace pumpkintown {

class Path {
 public:
  explicit Path(const std::string& path) : path_{path} {}

  Path(const Path& path) : path_{path.path_} {}

  const std::string& value() const { return path_; }

  Path parent() const {
    const auto& stripped{strip_trailing_slash()};
    const auto index{stripped.rfind('/')};
    if (index == std::string::npos) {
      throw std::runtime_error("no path separator");
    }
    return Path{stripped.substr(0, index)};
  }

  Path append(const std::string& piece) {
    if (piece.at(0) == '/') {
      throw std::runtime_error("cannot append absolute path");
    }
    const auto len{path_.size()};
    if (path_.at(len - 1) == '/') {
      return Path{path_ + piece};
    }
    return Path{path_ + '/' + piece};
  }

 private:
  std::string strip_trailing_slash() const {
    const auto len{path_.size()};
    if (path_.at(len - 1) == '/') {
      return path_.substr(0, len - 1);
    }
    return path_;
  }

  const std::string path_;
};

}

#endif  // PUMPKINTOWN_PATH_HH_
