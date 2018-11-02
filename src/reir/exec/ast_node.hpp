#ifndef REIR_AST_NODE_HPP_
#define REIR_AST_NODE_HPP_

#include <cstdint>
#include <cstddef>
#include <iosfwd>
#include <vector>
#include <functional>

#include "reir/db/attribute.hpp"
#include "util/blank.hpp"
#include "reir/db/maybe_value.hpp"
#include "compiler_context.hpp"
#include <llvm/ADT/StringRef.h>
#include <argp.h>

namespace llvm {
class Function;
class Value;
class Type;
}  // namespace llvm

namespace reir {
class TokenStream;
class CompilerContext;
class Procedure;
namespace node {
struct Expression;

struct Node {
  enum NodeKind {
    ND_None,

    // expression entry should be listed between ND_EXPRESSION_FIRST and _LAST
    ND_EXPRESSION_FIRST,
    ND_Primary,
    ND_Binary,
    ND_Pointer,
    ND_Row,
    ND_Array,
    ND_Variable,
    ND_Func,
    ND_ArrayRef,
    ND_MemberRef,
    ND_Assign,
    ND_EXPRESSION_LAST,

    // statement entry should be listed between ND_STATEMENT_FIRST and _LAST
    ND_STATEMENT_FIRST,
    ND_Block,
    ND_Define,
    ND_Emit,
    ND_ExprStatement,
    ND_For,
    ND_DefineTuple,
    ND_If,
    ND_Jump,
    ND_Insert,
    ND_Scan,
    ND_Let,
    ND_Transaction,
    ND_STATEMENT_LAST,
  };

  Node() : kind_(ND_None) {};

  explicit Node(NodeKind k) : kind_(k) {};

  Node(const Node&) = delete;

  virtual void dump(std::ostream& o, size_t indent) const = 0;

  virtual void each_value(const std::function<void(const Expression*)>& func) const {}

  virtual ~Node() = default;

  NodeKind getKind() const { return kind_; }

  virtual void analyze(CompilerContext& ctx) = 0;

private:
  const NodeKind kind_;
};

enum operators {
  PLUS,
  MINUS,
  MULTIPLE,
  DIVISION,
  EQUAL,
  MODULO,
  LESSTHAN,
  LESSEQUAL,
  MORETHAN,
  MOREEQUAL,
  NOTEQUAL,
  CONDITIONAL_AND,
  CONDITIONAL_OR,
  NONE,
};

inline std::string op_to_string(operators op) {
  const char* names[] = {
      "PLUS",
      "MINUS",
      "MULTIPLE",
      "DIVISION",
      "EQUAL",
      "MODULO",
      "LESSTHAN",
      "LESSEQUAL",
      "MORETHAN",
      "MOREEQUAL",
      "NOTEQUAL",
      "NONE",
  };
  return names[op];
}

enum type_id : int32_t {
  NONE_TYPE,
  INTEGER,
  DOUBLE,
  STRING,
  DATE,
  BOOL,
  FUNCTION,
  POINTER,
  ARRAY,
  TUPLE,

};

inline std::string type_to_string(type_id t) {
  static const char* names[] = {
      "NONE_TYPE",
      "INTEGER",
      "DOUBLE",
      "STRING",
      "DATE",
      "BOOL",
      "FUNCTION",
      "POINTER",
      "ARRAY",
      "TUPLE",
  };
  return names[t];
}

struct Type;
Type* operate(CompilerContext& ctx, Type& lhs, Type& rhs, operators op);

struct Type {
  type_id type_;
  Type() : type_(NONE_TYPE) {}
  explicit Type(type_id t) : type_(t) {}
  virtual ~Type() = default;
  virtual bool equals(const Type& rhs) const = 0;
  bool operator==(const Type& rhs) const {
    return equals(rhs);
  }
  virtual size_t size() const {
    return 8;  // FIXME
  }
  virtual void dump(std::ostream& o) const = 0;
};

struct ArrayType : public Type {
  explicit ArrayType(type_id from) : Type(from) {}
  explicit ArrayType(TokenStream& tokens);
  ~ArrayType() override = default;
  bool equals(const Type& rhs) const override {
    auto* r = dynamic_cast<const ArrayType*>(&rhs);
    if (r == nullptr) {return false;}
    return r->type_ == type_;
  }
  void dump(std::ostream& o) const override {
    o << "[ARRAY]";
  }
};

struct TupleType : public Type {
  std::vector<std::string> names_;
  std::vector<std::string> type_names_;
  std::vector<Attribute::AttrProperty> props_;
  std::vector<std::shared_ptr<int>> params_;
  std::vector<Type*> types_;  // only analyzed
  TupleType(std::vector<Type*>&& types,
            std::vector<std::string>&& names,
            std::vector<Attribute::AttrProperty>&& props = {})
      : Type(type_id::TUPLE), types_(std::move(types)),
      names_(std::move(names)), props_(std::move(props)) {}
  explicit TupleType(std::vector<Type*>&& types)
      : types_(std::move(types)) {
    for (int i = 0; i < types_.size(); ++i) {
      names_.emplace_back("(anonymous)");
    }
  }
  explicit TupleType(TokenStream& tokens);
  size_t size() const override {
    size_t ret = 0;
    for(const auto& type : types_) {
      ret += type->size();
    }
    return ret;
  }
  type_id at(int idx) {
    return types_[idx]->type_;
  }
  type_id at(std::string& name) {
    for (size_t i = 0; i < names_.size(); ++i) {
      if (name == names_[i]) {
        return types_[i]->type_;
      }
    }
    throw std::runtime_error("undefined tuple member");
  }
  void append(Type* type, std::string&& name) {
    types_.push_back(type);
    names_.emplace_back(std::move(name));
  }
  ~TupleType() override = default;

  Type* analyze(CompilerContext& ctx);
  bool equals(const Type& rhs) const override {
    auto* r = dynamic_cast<const TupleType*>(&rhs);
    if (r == nullptr) {return false;}
    if (types_.size() != r->types_.size() ||
        names_.size() != r->names_.size()) {
      return false;
    }
    for (size_t i = 0; i < types_.size(); ++i) {
      if (!types_[i]->equals(*r->types_[i]) ||
          names_[i] != r->names_[i]) {
        return false;
      }
    }
    return true;
  }
  void dump(std::ostream& o) const override {
    o << "{";
    for (size_t i = 0; i < types_.size(); ++i) {
      if (0 < i) o << ", ";
      types_[i]->dump(o);
      o << ":" << names_[i];
    }
    o << "}";
  }
};

struct PrimaryType : public Type {
  PrimaryType(const PrimaryType& o) = default;
  explicit PrimaryType(TokenStream& s);
  PrimaryType()
      : Type(NONE_TYPE) {}
  explicit PrimaryType(type_id t)
      : Type(t) {}

  static type_id type_parse(const std::string& type) {
    if (type == "int") {
      return type_id::INTEGER;
    } else if (type == "double") {
      return type_id::DOUBLE;
    } else if (type == "bool") {
      return type_id::BOOL;
    } else if (type == "string") {
      return type_id::STRING;
    } else if (type == "date") {
      return type_id::DATE;
    } else {
      throw std::runtime_error("type parser: unknown type " + type);
    }
  }

  bool equals(const Type& rhs) const override {
    auto* r = dynamic_cast<const PrimaryType*>(&rhs);
    if (r == nullptr) {return false;}
    return type_ == r->type_;
  }

  void dump(std::ostream& o) const override {
    o << type_to_string(type_);
  }
};

Type* parse_type(TokenStream& tokens);
Type* parse_row_type(TokenStream& tokens);

}  // namespace node
}  // namespace reir

#endif  // REIR_AST_NODE_HPP_
