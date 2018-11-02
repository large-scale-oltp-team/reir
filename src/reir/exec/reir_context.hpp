//
// Created by kumagi on 18/06/25.
//

#ifndef PROJECT_EXEC_HPP
#define PROJECT_EXEC_HPP

#include <string>
#include <memory>
#include <reir/db/metadata.hpp>

namespace reir {
class Compiler;
class FoedusRunner;
class Metadata;

class reir_context {
public:
  reir_context();
  void execute(const std::string& code);

private:
  std::shared_ptr<Compiler> c;
  std::shared_ptr<FoedusRunner> runner;
  std::shared_ptr<MetaData> md;
};
}  // namespace reir
#endif //PROJECT_EXEC_HPP
