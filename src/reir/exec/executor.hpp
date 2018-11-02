#ifndef REIR_EXECUTOR_HPP_
#define REIR_EXECUTOR_HPP_

#include <memory>

namespace reir {

namespace node {
struct Node;
}

class Compiler;
class MetaData;
class DBInterface;

class Executor {
 public:
  Executor();
  ~Executor();
  void execute(DBInterface& dbi, MetaData& m, node::Node* ast, bool jit = true);

 private:
  std::unique_ptr<Compiler> jit_compiler_;
};

}  // namespace reir

#endif  // REIR_EXECUTOR_HPP_
