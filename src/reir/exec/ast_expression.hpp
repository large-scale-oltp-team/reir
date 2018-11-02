//
// Created by kumagi on 18/07/03.
//

#ifndef PROJECT_AST_EXPRESSION_HPP
#define PROJECT_AST_EXPRESSION_HPP

#include <utility>
#include "ast_node.hpp"
#include "boost/variant.hpp"
#include "reir/db/maybe_value.hpp"
#include "token.hpp"
#include "compiler_context.hpp"

namespace reir {
struct Token;
class TokenStream;

namespace node {

llvm::Type* convert_type(node::Type* type, CompilerContext& c);

struct Expression : public Node {
  Expression(NodeKind k) : Node(k), type_(nullptr) {}

  Expression(const Expression&) = delete;

  void dump(std::ostream& o, size_t indent) const override = 0;

  virtual llvm::Type* get_type(CompilerContext& c) const = 0;

  virtual llvm::Value* get_value(CompilerContext& c) const = 0;

  void each_value(const std::function<void(const Expression * )>& func) const override {}

  ~Expression() override = default;

  friend std::ostream& operator<<(std::ostream& o, const Expression& e) {
    e.dump(o, 0);
    return o;
  }
  virtual void analyze(CompilerContext& ctx) override {}
  virtual llvm::Value* get_ref(CompilerContext& ctx) const {
    return nullptr;
  };

  static bool classof(const Node *n) {
    return ND_EXPRESSION_FIRST < n->getKind() && n->getKind() < ND_EXPRESSION_LAST;
  }
  Type* type_;
};

struct PrimaryExpression : public Expression {
  boost::variant<MaybeValue, Expression*> value_;

  explicit PrimaryExpression(TokenStream& tokens);
  explicit PrimaryExpression(MaybeValue&& value)
      : Expression(ND_Primary),
        value_(std::move(value)) {}
  ~PrimaryExpression() override {
    if (value_.which() == 1) {
      delete boost::get<Expression*>(value_);
    }
  }
  void analyze(CompilerContext& ctx) override {
    switch (value_.which()) {
      case 0: {
        auto& v = boost::get<MaybeValue>(value_);
        if (v.exists()) {
          const Value& val(v.value());
          if (val.is_int()) {
            type_ = ctx.analyze_type_table_["integer"];
          } else if (val.is_varchar()) {
            type_ = ctx.analyze_type_table_["string"];
          } else if (val.is_float()) {
            type_ = ctx.analyze_type_table_["double"];
          } else {
            throw std::runtime_error("unknown primary type");
          }
        }
        break;
      }
      case 1: {
        auto* e = boost::get<Expression*>(value_);
        e->analyze(ctx);
        type_ = e->type_;
      }
      default:
        throw std::runtime_error("cannot analyze primary expression type");
    }
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    if (value_.which() == 1) {
      boost::get<Expression*>(value_)->each_value(func);
    }
  }

  llvm::Value* get_value(CompilerContext& ctx) const override;
  void dump(std::ostream& o, size_t indent) const override {
    switch (value_.which()) {  // MaybeValue
      case 0: {
        o << boost::get<MaybeValue>(value_);
        break;
      }
      case 1: {
        auto* e = boost::get<Expression*>(value_);
        o << "Expression(";
        e->dump(o, indent);
        o << ")";
        break;
      }
      default:
        throw std::runtime_error("unknown type");
    }
  }

  llvm::Type* get_type(CompilerContext& c) const override;

  static bool classof(const Node *n) {
    return n->getKind() == ND_Primary;
  }
};

struct BinaryExpression : public Expression {
  operators op_;
  Expression* lhs_;
  Expression* rhs_;

  BinaryExpression(Expression* lhs, operators op, Expression* rhs)
      : Expression(ND_Binary), lhs_(lhs), op_(op), rhs_(rhs) {
  }
  void analyze(CompilerContext& ctx) override {
    lhs_->analyze(ctx), rhs_->analyze(ctx);
    auto* left_type = lhs_->type_, *right_type = rhs_->type_;
    type_ = operate(ctx, *left_type, *right_type, op_);
  };

  static bool is_operator(Token& t) {
    switch (t.type) {
      case token_type::PLUS:
      case token_type::MINUS:
      case token_type::ASTERISK:
      case token_type::SLASH:
      case token_type::EQUAL:
      case token_type::PERCENT:
      case token_type::OPEN_ANGLE:
      case token_type::CLOSE_ANGLE:
        return true;
      default:
        return false;
    }
  }

  ~BinaryExpression() override {
    delete lhs_;
    delete rhs_;
  }

  void dump(std::ostream& o, size_t indent) const override {
    o << "(";
    lhs_->dump(o, indent);
    switch (op_) {
      case PLUS: o << " + "; break;
      case MINUS: o << " - "; break;
      case MULTIPLE: o << " * "; break;
      case DIVISION: o << " / "; break;
      case EQUAL: o << " == "; break;
      case MODULO: o << " % "; break;
      case LESSTHAN: o << " < "; break;
      case LESSEQUAL: o << " <= "; break;
      case MORETHAN: o << " > "; break;
      case MOREEQUAL: o << " >= "; break;
      case NOTEQUAL: o << " != "; break;
      case CONDITIONAL_AND: o << " && "; break;
      case CONDITIONAL_OR: o << " || "; break;
      case NONE: o << " ???? "; break;
    }
    rhs_->dump(o, indent);
    o << ")";
  }

  llvm::Type* get_type(CompilerContext& c) const override;

  llvm::Value* get_value(CompilerContext& c) const override;

  llvm::Value* get_conditional_value(CompilerContext& c, bool forAND) const;
  llvm::Value* to_i1(CompilerContext& c, llvm::Value* lh) const;

  static bool classof(const Node *n) {
    return n->getKind() == ND_Binary;
  }
};

struct PointerOf : public Expression {
  Expression* target_;

  explicit PointerOf(Expression* t) : Expression(ND_Pointer), target_(t) {}

  PointerOf()
      : Expression(ND_Pointer), target_(nullptr) {}

  ~PointerOf() override { delete target_; }

  void analyze(CompilerContext& ctx) override {};
  void dump(std::ostream& o, size_t indent) const override {
    o << "&";
    target_->dump(o, indent);
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    func(target_);
  }

  llvm::Type* get_type(CompilerContext& c) const override;

  llvm::Value* get_value(CompilerContext& c) const override;

  static bool classof(const Node *n) {
    return n->getKind() == ND_Pointer;
  }
};

struct RowLiteral : public Expression {
  std::vector<Expression*> elements_;

  explicit RowLiteral(TokenStream& s);

  ~RowLiteral() override {
    for (auto& e : elements_) {
      delete e;
    }
  }
  void analyze(CompilerContext& ctx) override {
    std::vector<Type*> t;
    for (auto& element : elements_) {
      element->analyze(ctx);
      assert(element->type_);
      t.emplace_back(element->type_);
    }
    type_ = new TupleType(std::move(t));
    std::stringstream ss;
    ss << this;
    ctx.analyze_type_table_["__TupleLiteral:" + ss.str()] = type_;
  }

  explicit RowLiteral(std::vector<Expression*>&& v) : Expression(ND_Row), elements_(std::move(v)) {}

  explicit RowLiteral(std::initializer_list<Expression*>&& v) : RowLiteral(std::vector<Expression*>(v)) {}

  void dump(std::ostream& o, size_t indent ) const override {
    o << "RowLiteral{";
    for (size_t i = 0; i < elements_.size(); ++i) {
      if (0 < i) { o << ", "; }
      elements_[i]->dump(o, indent);
    }
    o << "}";
  }

  llvm::Type* get_type(CompilerContext& c) const override;

  llvm::Value* get_value(CompilerContext& c) const override;

  size_t elements() const {
    return elements_.size();
  }

  static bool classof(const Node *n) {
    return n->getKind() == ND_Row;
  }
};

struct ArrayLiteral : public Expression {
  std::vector<Expression*> elements_;

  explicit ArrayLiteral(TokenStream& tokens);

  explicit ArrayLiteral(std::vector<Expression*>&& v) : Expression(ND_Array), elements_(std::move(v)) {}

  explicit ArrayLiteral(std::initializer_list<Expression*>&& v) : ArrayLiteral(std::vector<Expression*>(v)) {}

  ~ArrayLiteral() override {
    for (auto& e : elements_) {
      delete e;
    }
  }

  void dump(std::ostream& o, size_t indent) const override {
    o << "ArrayLiteral(" << elements_.size() << ")[";
    for (size_t i = 0; i < elements_.size(); ++i) {
      if (0 < i) { o << ", "; }
      elements_[i]->dump(o, indent);
    }
    o << "]";
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    for (const auto& e : elements_) {
      e->each_value(func);
    }
    func(this);
  }

  llvm::Type* get_type(CompilerContext& c) const override;

  llvm::Value* get_value(CompilerContext& c) const override;



  void analyze(CompilerContext& ctx) override {
    if (elements_.empty()) {
      throw std::runtime_error("empty array literal's type cant be resolved\n");
    }
    elements_[0]->analyze(ctx);
    auto* tmp = new ArrayType(elements_[0]->type_->type_);
    std::stringstream ss;
    ss << this;
    ctx.analyze_type_table_["__ArrayLiteral:"+ss.str()] = tmp;
    type_ = tmp;
  }

  static bool classof(const Node *n) {
    return n->getKind() == ND_Array;
  }
};

struct VariableReference : public Expression {
  std::string name_;

  explicit VariableReference(std::string s) : Expression(ND_Variable), name_(std::move(s)) {}

  explicit VariableReference(TokenStream& s);

  ~VariableReference() override = default;

  void dump(std::ostream& o, size_t indent) const override {
    o << "VarRef:" << name_;
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    func(this);
  }

  llvm::Type* get_type(CompilerContext& c) const override;

  llvm::Value* get_value(CompilerContext& c) const override;
  llvm::Value* get_ref(CompilerContext& c) const override;

  void analyze(CompilerContext& ctx) override {
    auto it = ctx.variable_type_table_.find(name_);
    if (it == ctx.variable_type_table_.end()) {
      std::stringstream ss;
      ss << "undefined variable " << name_ << " referenced";
      throw std::runtime_error(ss.str());
    }
    type_ = it->second;
  }
  static bool classof(const Node *n) {
    return n->getKind() == ND_Variable;
  }
};

struct FunctionCall : public Expression {
  Expression* parent_;
  std::vector<Expression*> args_;

  FunctionCall(const std::string& name, std::vector<Expression*>&& arg)
      : Expression(ND_Func), parent_(new VariableReference(name)), args_(std::move(arg)) {}

  FunctionCall(const std::string& name, std::initializer_list<Expression*>&& arg)
      : FunctionCall(name, std::vector<Expression*>(arg)) {}

  FunctionCall(Expression* parent, TokenStream& s);

  ~FunctionCall() override {
    delete parent_;
    for (auto& a : args_) {
      delete a;
    }
  }

  llvm::Type* get_type(CompilerContext& c) const override;

  void dump(std::ostream& o, size_t indent) const override {
    o << "Fun:";
    parent_->dump(o, indent);
    o << "(";
    for (size_t i = 0; i < args_.size(); ++i) {
      if (0 < i) { o << ", "; }
      args_[i]->dump(o, indent);
    }
    o << ")";
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    // parent_->each_value(func);
    for (const auto* a : args_) {
      func(a);
      a->each_value(func);
    }
  }

  llvm::Value* get_value(CompilerContext& c) const override;

  void analyze(CompilerContext& ctx) override {
    type_ = ctx.analyze_type_table_["integer"];
    for (const auto& a : args_) {
      a->analyze(ctx);
    }
  }

  static bool classof(const Node *n) {
    return n->getKind() == ND_Func;
  }
};

struct ArrayReference : public Expression {
  Expression* parent_;
  Expression* idx_;

  ArrayReference(Expression* parent, TokenStream& s);

  ArrayReference(Expression* p, Expression* i) : Expression(ND_ArrayRef), parent_(p), idx_(i) {}

  ~ArrayReference() override;

  void dump(std::ostream& o, size_t indent) const override {
    o << "ArrRef";

    parent_->dump(o, indent);
    o << "[";
    idx_->dump(o, indent);
    o << "]";
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    func(this);
    parent_->each_value(func);
  }

  llvm::Type* get_type(CompilerContext& c) const override;

  llvm::Value* get_value(CompilerContext& c) const override;

  llvm::Value* get_ref(CompilerContext& ctx) const override;

  void analyze(CompilerContext& ctx) override {
    parent_->analyze(ctx);
    idx_->analyze(ctx);
    auto* par = dynamic_cast<ArrayType*>(parent_->type_);
    if (!par) {
      throw std::runtime_error("array reference must detect array");
    }
    type_ = par;
  }
  static bool classof(const Node *n) {
    return n->getKind() == ND_ArrayRef;
  }
};

struct MemberReference : public Expression {
  Expression* parent_;
  std::string name_;
  uint64_t offset_;

  MemberReference(Expression* value, TokenStream& s);
  MemberReference(Expression* parent, std::string name) : Expression(ND_MemberRef),
                                                          parent_(parent), name_(std::move(name)) {}

  ~MemberReference() override;

  void dump(std::ostream& o, size_t indent) const override {
    parent_->dump(o, indent);
    o << "-m->" << name_;
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    func(this);
    parent_->each_value(func);
  }

  llvm::Type* get_type(CompilerContext& c) const override;

  llvm::Value* get_value(CompilerContext& c) const override;

  llvm::Value* get_ref(CompilerContext& ctx) const override;

  void analyze(CompilerContext& ctx) override {
    parent_->analyze(ctx);
    auto* tupletype = dynamic_cast<TupleType*>(parent_->type_);
    if (!tupletype) {
      throw std::runtime_error("type must be tuple");
    }
    auto names = tupletype->names_;
    uint64_t offset = 0;
    for (size_t i = 0; i < names.size(); ++i) {
      if (names[i] == name_) {
        type_ = tupletype->types_[i];
        offset_ = offset;
        return;
      }
      ++offset;
    }
    std::stringstream ss;
    parent_->dump(ss, 0);
    ss << " ";
    tupletype->dump(ss);
    throw std::runtime_error("not found member named " + name_ + " in " + ss.str());
  }
  static bool classof(const Node *n) {
    return n->getKind() == ND_MemberRef;
  }
};

struct Assign : public Expression {
  Expression* target_;
  Expression* value_;

  ~Assign() override {
    delete target_;
    delete value_;
  }

  Assign(Expression* target, Expression* value)
      : Expression(ND_Assign), target_(target), value_(value) {}

  Assign(Expression* target, TokenStream& tokens);

  void dump(std::ostream& o, size_t indent) const override {
    target_->dump(o, indent);
    o << " = ";
    value_->dump(o, indent);
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    func(this);
    value_->each_value(func);
  }

  llvm::Type* get_type(CompilerContext& c) const override;

  llvm::Value* get_value(CompilerContext& c) const override;

  void analyze(CompilerContext& ctx) override {
    target_->analyze(ctx);
    value_->analyze(ctx);
    type_ = value_->type_;
  }
  static bool classof(const Node *n) {
    return n->getKind() == ND_Assign;
  }
};

}  // namespace node
}  // namespace reir

#endif //PROJECT_AST_EXPRESSION_HPP
