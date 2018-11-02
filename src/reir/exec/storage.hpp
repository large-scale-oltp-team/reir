#ifndef REIR_STORAGE_HPP_
#define REIR_STORAGE_HPP_

#include "reir/db/metadata.hpp"
#include "reir/exec/executor.hpp"

namespace reir {
class DBInterface;

class Storage {
 public:
  Storage() {}

  void execute(DBInterface& dbi, node::Node* ast);

 private:
  MetaData md_;
  Executor exec_;
};

}  // namespace

#endif  // REIR_STORAGE_HPP_
