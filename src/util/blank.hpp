#ifndef REIR_BLANK_HPP
#define REIR_BLANK_HPP

#include <ostream>

namespace util {
class blank {
 public:
  blank(size_t idt) : indent_(idt) {}
  friend std::ostream &operator<<(std::ostream& o, const blank& b) {
    for (int i = 0; i < b.indent_; ++i) {
      o << " ";
    }
    return o;
  }
 private:
  int indent_;
};

}  // namespace util

#endif // REIR_BLANK_HPP
