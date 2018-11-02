#include <cstdint>
#include <vector>
#include <iostream>
#include <string>

#include "ast_node.hpp"
#include "compiler_context.hpp"
#include <llvm/Target/TargetMachine.h>

namespace reir {
namespace node {

Type* TupleType::analyze(CompilerContext& ctx) {
  for (const auto& type_name : type_names_) {
    if (type_name == "int") {
      types_.push_back(ctx.analyze_type_table_["integer"]);
    } else if (type_name == "double") {
      types_.push_back(ctx.analyze_type_table_["double"]);
    } else if (type_name == "string") {
      types_.push_back(ctx.analyze_type_table_["string"]);
    } else if (type_name == "date") {
      types_.push_back(ctx.analyze_type_table_["integer"]);
    } else {
      auto it = ctx.analyze_type_table_.find(type_name);
      if (it == ctx.analyze_type_table_.end()) {
        throw std::runtime_error("undefined type " + type_name);
      } else {
        types_.push_back(it->second);
      }
    }
  }
  return this;
}


Type* operate(CompilerContext& ctx, Type& lhs, Type& rhs, operators op) {
  switch (lhs.type_) {
    case type_id::ARRAY:
      throw std::runtime_error("array with operator not supported yet");
    case type_id::TUPLE:
      throw std::runtime_error("tuple operator not supported yet");
    case type_id::INTEGER:
    case type_id::DOUBLE:
      switch (op) {
        case PLUS:
        case MINUS:
        case MULTIPLE:
        case DIVISION:
        case MODULO:
          if (rhs.type_ == lhs.type_) {
            return &lhs;
          } else {
            throw std::runtime_error("you can't operate with different types");
          }
        case EQUAL:
        case LESSTHAN:
        case LESSEQUAL:
        case MORETHAN:
        case MOREEQUAL:
        case NOTEQUAL:
          if (rhs.type_ == lhs.type_) {
            return ctx.analyze_type_table_["bool"];
          } else {
            throw std::runtime_error("you can't operate with different types");
          }
        case NONE:
          throw std::runtime_error("invalid operator");
        case CONDITIONAL_AND:
        case CONDITIONAL_OR:
          throw std::runtime_error("you cant || or && with double");
      }
      break;
    case type_id::STRING: {
      switch (op) {
        case PLUS:
          if (rhs.type_ == lhs.type_) {
            return ctx.analyze_type_table_["string"];
          } else {
            throw std::runtime_error("you can't operate string and different types");
          }
        case EQUAL:
        case LESSTHAN:
        case LESSEQUAL:
        case MORETHAN:
        case MOREEQUAL:
        case NOTEQUAL:
          if (rhs.type_ == lhs.type_) {
            return ctx.analyze_type_table_["bool"];
          } else {
            throw std::runtime_error("you can't operate with different types");
          }
        default:
          throw std::runtime_error("you cannot string with operator");
      }
    }
    case NONE_TYPE:break;
    case BOOL:break;
    case FUNCTION:break;
    case POINTER:break;
    case DATE:
      throw std::runtime_error("operation with date is not supported");
      break;
  }
  std::stringstream ss;
  lhs.dump(ss);
  ss << " " << op_to_string(op) << " ";
  rhs.dump(ss);
  throw std::runtime_error("not implemented operator for " + ss.str());
}

}  // namespace node
}  // namespace reir
