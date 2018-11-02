//
// Created by kumagi on 18/06/25.
//

#include <vector>
#include <iostream>
#include <fstream>
#include "reir_context.hpp"

#include "reir/exec/parser.hpp"
#include "reir/exec/compiler.hpp"
#include "reir/engine/foedus_runner.hpp"
#include "reir/exec/db_interface.hpp"

namespace reir {

reir_context::reir_context()
    : c(new Compiler), runner(new FoedusRunner), md(new MetaData) {}

void reir_context::execute(const std::string& code) {
  runner->run([&](DBInterface& dbi) {
    parse(code, [&](node::Node* ast) {
      c->compile_and_exec(dbi, *md, ast);
    });
  });
}
}


