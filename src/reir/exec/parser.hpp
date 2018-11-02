#ifndef REIR_PARSER_HPP_
#define REIR_PARSER_HPP_

#include <cstdint>
#include <functional>
#include "ast_node.hpp"

namespace reir {
namespace node {
struct Statement;
}
class TokenStream;
void parse(const std::string& code, const std::function<void(node::Node*)>& fun);

}  // namespace reir

#endif  // REIR_PARSER_HPP_
