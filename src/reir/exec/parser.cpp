#include <cstdint>
#include <cstddef>
#include <string>
#include <iostream>
#include <cstdint>
#include <sstream>
#include <string>
#include <reir/db/attribute.hpp>
#include "ast_expression.hpp"
#include "parser.hpp"
#include "tokenize.hpp"
#include "reir/db/maybe_value.hpp"
#include "ast_statement.hpp"

namespace reir {

namespace node {
Statement* parse_statement(TokenStream& tokens);
class Node;
}

// implementation of parse_statement exists in ast_node_parser.cpp

void parse(const std::string& code, const std::function<void(node::Node*)>& fun) {
  auto token_list = tokenize(code);
  // std::cout << token_list << std::endl;
  TokenStream tokens(std::move(token_list));
  std::unique_ptr<node::Block> global_block(new node::Block());
  while (tokens.has_next()) {
    auto* entry = node::parse_statement(tokens);
    if (entry) {
      global_block->add(entry);
    }
  }
  fun(global_block.get());
}

}  // namespace reir
