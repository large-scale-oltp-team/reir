#include "storage.hpp"
#include "ast_node.hpp"

#include <iostream>

namespace reir {

void Storage::execute(DBInterface& dbi, Procedure& proc) {
  switch (ast->type_) {
  case node::Node::NodeType::CreateTableT: {
    node::CreateTable* ct = static_cast<node::CreateTable*>(proc.ast);
    md_.create_table(ct);
    break;
  }
  case node::Node::NodeType::DropTableT: {
    node::DropTable* dt = static_cast<node::DropTable*>(proc.ast);
    md_.drop_table(dt);
    break;
  }
  case node::Node::NodeType::InsertT: {
    node::Insert* ins = static_cast<node::Insert*>(proc.ast);
    std::string key, value;
    std::cout << "inserting: ";
    ins->dump(std::cout);
    std::cout << std::endl;
    exec_.execute(dbi, md_, ast, true);
    break;
  }
  case node::Node::NodeType::TransactionT: {
    exec_.execute(dbi, md_, proc, true);
    break;
  }
  default:
    exec_.execute(dbi, md_, proc, false);
  }
}

}  // namespace reir
