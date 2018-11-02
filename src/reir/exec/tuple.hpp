#ifndef REIR_TUPLE_HPP_
#define REIR_TUPLE_HPP_

namespace reir {

class Cell {
  bool exists_;
  int data_;
};

class Tuple {
 public:
  std::vector<Cell> payload_;
};

}  // namespace reir

#endif  // REIR_TUPLE_HPP_
