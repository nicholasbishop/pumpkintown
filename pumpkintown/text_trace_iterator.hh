#ifndef TEXT_TRACE_ITERATOR_HH_
#define TEXT_TRACE_ITERATOR_HH_

#include <fstream>
#include <string>
#include <vector>

namespace pumpkintown {

class TextTraceIterator {
 public:
  explicit TextTraceIterator(const std::string& path);

  bool next();

  const std::string& function() const { return function_; }
  const std::vector<std::string>& args() const { return args_; }
  const std::string& return_value() const { return return_value_; }

 private:
  std::ifstream file_;
  std::string function_;
  std::vector<std::string> args_;
  std::string return_value_;
};

}

#endif  // TEXT_TRACE_ITERATOR_HH_
