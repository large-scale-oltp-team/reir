//
// Created by kumagi on 18/07/03.
//

#ifndef PROJECT_AST_STATEMENT_HPP
#define PROJECT_AST_STATEMENT_HPP

#include <utility>
#include "ast_node.hpp"
#include "ast_expression.hpp"
#include "compiler_context.hpp"

namespace llvm {
class Constant;
}

namespace reir {
class TokenStream;
namespace node {

struct Statement : public Node {
  Statement(NodeKind k) : Node(k) {}

  Statement(const Statement&) = delete;

  void dump(std::ostream& o, size_t indent) const override = 0;

  virtual void codegen(CompilerContext& c) const = 0;

  virtual void alloca_stack(CompilerContext& c) const = 0;

  virtual void each_statement(std::function<void(const Statement*)> func) const = 0;

  void each_value(const std::function<void(const Expression*)>& func) const override = 0;

  ~Statement() override = default;

  void analyze(CompilerContext& ctx) override = 0;
  static bool classof(const Node *n) {
    return ND_STATEMENT_FIRST < n->getKind() && n->getKind() < ND_STATEMENT_LAST;
  }
};

Statement* parse_statement(TokenStream& tokens);

struct Block : public Statement {
  std::vector<Statement*> statements_;

  Block() : Statement(ND_Block) {};

  explicit Block(std::vector<Statement*>&& s) : Statement(ND_Block), statements_(std::move(s)) {}

  explicit Block(std::initializer_list<Statement*>&& s) : Block(std::vector<Statement*>(s)) {}

  explicit Block(TokenStream& s);

  Block(const Block&) = delete;

  ~Block() override {
    for (auto& s : statements_) {
      delete s;
    }
  }

  void add(Statement* child) {
    statements_.emplace_back(child);
  }

  void alloca_stack(CompilerContext& c) const override {
  }

  size_t size() const { return statements_.size(); }

  void dump(std::ostream& o, size_t indent) const override {
    o << "Block [" << std::endl;
    for (const auto& stmt : statements_) {
      o << util::blank(indent + 2);
      stmt->dump(o, indent + 2);
      o << std::endl;
    }
    o << util::blank(indent) << "]";
  }

  void each_statement(std::function<void(const Statement*)> func) const override {
    for (const auto& s : statements_) {
      func(s);
      s->each_statement(func);
    }
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    for (const auto& s : statements_) {
      s->each_value(func);
    }
  }
  void analyze(CompilerContext& ctx) override {
    for (auto& statement : statements_) {
      statement->analyze(ctx);
    }
  }

  void codegen(CompilerContext& c) const override;

  static bool classof(const Node *n) {
    return n->getKind() == ND_Block;
  }
};

struct Transaction : public Statement {
  std::string isolation_;
  Block* sequence_;

  explicit Transaction(TokenStream& s);

  Transaction(std::string isolation,
              std::vector<Statement*>&& s)
      : Statement(ND_Transaction),
        isolation_(std::move(isolation)),
        sequence_(new Block(std::move(s))) {}

  Transaction(std::string isolation, std::initializer_list<Statement*>&& s)
      : Transaction(std::move(isolation), std::vector<Statement*>(s)) {}

  ~Transaction() override {
    delete sequence_;
  }

  void alloca_stack(CompilerContext& c) const override {}

  void dump(std::ostream& o, size_t indent) const override {
    o << "transaction(" << isolation_ << ") [" << std::endl
      << util::blank(indent + 2);
    sequence_->dump(o, indent + 2);
    o << "\n" << util::blank(indent) << "]";
  }

  void each_statement(std::function<void(const Statement*)> func) const override {
    sequence_->each_statement(func);
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    sequence_->each_value(func);
  }
  void analyze(CompilerContext& ctx) override {
    sequence_->analyze(ctx);
  }


  void codegen(CompilerContext& c) const override;

  static bool classof(const Node *n) {
    return n->getKind() == ND_Transaction;
  }
};


struct Define : public Statement {
  TupleType* schema_;
  std::string name_;

  ~Define() override = default;

  explicit Define(TokenStream& s);
  Define(TupleType* s, std::string n) : Statement(ND_Define), schema_(s), name_(std::move(n)) {}

  void dump(std::ostream& o, size_t indent) const override {
    o << "Define(" << name_ << ")";
    schema_->dump(o);
  }

  void alloca_stack(CompilerContext& c) const override;

  void each_value(const std::function<void(const Expression*)>& func) const override {}

  void each_statement(std::function<void(const Statement*)> func) const override {}

  void codegen(CompilerContext& c) const override;

  void analyze(CompilerContext& ctx) override {
    schema_->analyze(ctx);
    ctx.analyze_type_table_[name_] = schema_;
  }

  static bool classof(const Node *n) {
    return n->getKind() == ND_Define;
  }
};

struct For : public Statement {
  Statement* init_;
  Expression* cond_;
  Expression* every_;
  Block* blk_;

  For(Statement* i, Expression* c, Expression* e, Block* b)
      : Statement(ND_For), init_(i), cond_(c), every_(e), blk_(b) {}

  ~For() override {
    delete init_;
    delete cond_;
    delete every_;
    delete blk_;
  }

  explicit For(TokenStream& tokens);

  void dump(std::ostream& o, size_t indent) const override {
    o << "for (";
    if (init_) { init_->dump(o, indent); }
    o << ";";
    if (cond_) { cond_->dump(o, indent); }
    o << ";";
    if (every_) { every_->dump(o, indent); }
    o << ") {" << std::endl << util::blank(indent + 2);
    blk_->dump(o, indent + 2);
    o << std::endl << util::blank(indent) << "}";
  }

  void alloca_stack(CompilerContext& c) const override {
    if (init_) {
      init_->alloca_stack(c);
    }
  }

  void each_statement(std::function<void(const Statement*)> func) const override {
    if (init_) {
      func(init_);
    }
    if (blk_) {
      blk_->each_statement(func);
    }
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {}

  void codegen(CompilerContext& c) const override;

  void analyze(CompilerContext& ctx) override {
    if (init_) {
      init_->analyze(ctx);
    }
    if (cond_) {
      cond_->analyze(ctx);
    }
    if (every_) {
      every_->analyze(ctx);
    }
    if (blk_) {
      blk_->analyze(ctx);
    }
  }

  static bool classof(const Node *n) {
    return n->getKind() == ND_For;
  }
};

struct Jump : public Statement {
  enum type {
    continue_jump,
    break_jump
  };
  type t_;

  explicit Jump(type t) : Statement(ND_Jump), t_(t) {}

  void codegen(CompilerContext& c) const override;

  void alloca_stack(CompilerContext& c) const override {}

  void each_statement(std::function<void(const Statement*)> func) const override {}

  void each_value(const std::function<void(const Expression*)>& func) const override {}

  void dump(std::ostream& o, size_t indent) const override {
    if (t_ == type::continue_jump) {
      o << "continue";
    } else if (t_ == type::break_jump) {
      o << "break";
    } else {
      o << "invalid_jump";
    }
  }

  void analyze(CompilerContext& ctx) override {}

  ~Jump() override = default;

  static bool classof(const Node *n) {
    return n->getKind() == ND_Jump;
  }
};

struct Emit : public Statement {
  Expression* value_;

  Emit() : Statement(ND_Emit), value_(nullptr) {};

  Emit(Expression* v) : Statement(ND_Emit), value_(v) {}

  Emit(TokenStream& tokens);

  void dump(std::ostream& o, size_t indent) const override {
    o << "emit: ";
    value_->dump(o, indent);
  }

  void codegen(CompilerContext& c) const override;

  void alloca_stack(CompilerContext& c) const override;

  void each_statement(std::function<void(const Statement*)> func) const override {}

  void each_value(const std::function<void(const Expression*)>& func) const override {
    func(value_);
  }

  ~Emit() override {
    delete value_;
  }
  void analyze(CompilerContext& ctx) override {
    value_->analyze(ctx);
  }

  static bool classof(const Node *n) {
    return n->getKind() == ND_Emit;
  }
};

struct Insert : public Statement {
  std::string table_;
  Expression* value_;
  mutable llvm::Constant* prefix_;
  mutable llvm::Value* key_stack_;
  mutable llvm::Value* value_stack_;
  mutable llvm::Value* tuple_stack_;

  explicit Insert(TokenStream& tokens);

  Insert(std::string t, Expression* v) : Statement(ND_Insert), table_(std::move(t)), value_(v),
     key_stack_(nullptr), value_stack_(nullptr), tuple_stack_(nullptr), prefix_(nullptr) {}

  void codegen(CompilerContext& c) const override;

  ~Insert() override {
    delete value_;
  }

  void dump(std::ostream& o, size_t indent) const override {
    o << "insert into " << table_ << ": ";
    value_->dump(o, indent);
  }

  void alloca_stack(CompilerContext& c) const override;

  void each_statement(std::function<void(const Statement*)> func) const override {}

  void each_value(const std::function<void(const Expression*)>& func) const override {
    func(value_);
  }
  void analyze(CompilerContext& ctx) override {
    value_->analyze(ctx);
  }
  static bool classof(const Node *n) {
    return n->getKind() == ND_Insert;
  }
};

struct Scan : public Statement {
  std::string table_;
  std::string row_name_;
  Block* blk_;
  mutable llvm::Constant* prefix_begin_;
  mutable llvm::Constant* prefix_end_;
  mutable llvm::Value* key_stack_;
  mutable llvm::Value* value_stack_;
  mutable llvm::Value* tuple_stack_;

  Scan(TokenStream& tokens);

  Scan(std::string t, std::string n, Block* b) : Statement(NodeKind::ND_Scan),
    table_(std::move(t)), row_name_(std::move(n)), blk_(b),
    key_stack_(nullptr), value_stack_(nullptr), tuple_stack_(nullptr), prefix_begin_(nullptr), prefix_end_(nullptr) {}

  void codegen(CompilerContext& c) const override;

  ~Scan() override {
    delete blk_;
  }

  void dump(std::ostream& o, size_t indent) const override {
    o << "full_scan(" << table_ << "): as |" << row_name_ << "|\n" << util::blank(indent);
    blk_->dump(o, indent);
  }

  void alloca_stack(CompilerContext& c) const override;

  void each_statement(std::function<void(const Statement*)> func) const override {
    blk_->each_statement(func);
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
  }

  void analyze(CompilerContext& ctx) override {
    auto* schema = ctx.local_schema_table_[table_];
    ctx.variable_type_table_[row_name_] = ctx.analyze_type_table_[table_];
    blk_->analyze(ctx);
  }

  static bool classof(const Node *n) {
    return n->getKind() == ND_Scan;
  }
};

struct Let : public Statement {
  std::string name_;
  Expression* expr_;
  Type* type_;

  explicit Let(TokenStream& s);

  explicit Let(llvm::StringRef name, Expression* expr)
      : Statement(ND_Let), name_(name), expr_(expr), type_(nullptr) {}

  ~Let() override {
    delete expr_;
  }

  void dump(std::ostream& o, size_t indent) const override {
    o << "let " << name_ << " := ";
    expr_->dump(o, indent);
  }

  void alloca_stack(CompilerContext& c) const override;

  void each_value(const std::function<void(const Expression*)>& func) const override {
    expr_->each_value(func);
  }

  void each_statement(std::function<void(const Statement*)> func) const override {
  }

  void codegen(CompilerContext& c) const override;

  void analyze(CompilerContext& ctx) override {
    expr_->analyze(ctx);
    auto* row_lit = dynamic_cast<RowLiteral*>(expr_);
    if (row_lit) {  // value is row literal
      if (type_ == nullptr) {
        ctx.variable_type_table_[name_] = type_ = row_lit->type_;
      } else {
        auto* tuple_type = dynamic_cast<TupleType*>(type_);
        tuple_type->analyze(ctx);
        ctx.analyze_type_table_[name_] = tuple_type;
        ctx.variable_type_table_[name_] = type_;
      }
      return;
    }
    auto* arr_lit = dynamic_cast<ArrayLiteral*>(expr_);
    if (arr_lit) {
      auto* tmp = new ArrayType(arr_lit->elements_[0]->type_->type_);
      ctx.analyze_type_table_[name_] = tmp;
      ctx.variable_type_table_[name_] = tmp;
      type_ = arr_lit->type_;
    } else if (type_ == nullptr) {
      type_ = expr_->type_;
      ctx.variable_type_table_[name_] = type_;
    } else {
      std::cerr << type_ << " type\n";
      throw std::runtime_error("unknown type variable of " + name_);
    }
  }
  static bool classof(const Node *n) {
    return n->getKind() == ND_Let;
  }
};

struct DefineTuple : public Statement {
  std::string name_;
  TupleType* type_;
  explicit DefineTuple(TokenStream& s);
  void analyze(CompilerContext& ctx) override {
    ctx.analyze_type_table_[name_] = type_->analyze(ctx);
  }
  void dump(std::ostream& o, size_t indent) const override {
    o << "typedef: ";
    type_->dump(o);
    o << " " << name_;
  }
  void codegen(CompilerContext& c) const override {}
  void alloca_stack(CompilerContext& c) const override {
    c.analyze_type_table_[name_] = type_;
  }
  void each_value(const std::function<void(const Expression*)>& func) const override {}
  void each_statement(std::function<void(const Statement*)> func) const override {}
};

struct If : public Statement {
  Expression* cond_;
  Block* true_block_;
  Block* false_block_;

  explicit If(TokenStream& s);

  If(Expression* c, Block* t, Block* f)
    : Statement(ND_If), cond_(c), true_block_(t), false_block_(f) {}

  ~If() override {
    delete cond_;
    delete true_block_;
    delete false_block_;
  }

  void dump(std::ostream& o, size_t indent) const override {
    o << "if ";
    cond_->dump(o, indent);
    o << " then" << std::endl << util::blank(indent + 2);
    true_block_->dump(o, indent + 2);
    if (false_block_) {
      o << std::endl << util::blank(indent) << "else" << std::endl << util::blank(indent + 2);
      false_block_->dump(o, indent + 2);
    }
    o << std::endl << util::blank(indent) << "fi";
  }

  void alloca_stack(CompilerContext& c) const override;

  void each_value(const std::function<void(const Expression*)>& func) const override {
    cond_->each_value(func);
    true_block_->each_value(func);
    if (false_block_) {
      false_block_->each_value(func);
    }
  }

  void each_statement(std::function<void(const Statement*)> func) const override {
    if (true_block_) {
      true_block_->each_statement(func);
    }
    if (false_block_) {
      false_block_->each_statement(func);
    }
  }


  void analyze(CompilerContext& ctx) override {
    true_block_->analyze(ctx);
    if (false_block_) {
      false_block_->analyze(ctx);
    }
  }
  void codegen(CompilerContext& c) const override;

  static bool classof(const Node *n) {
    return n->getKind() == ND_If;
  }
};

struct ExprStatement : public Statement {
  Expression* value_;

  explicit ExprStatement(Expression* e) : Statement(ND_ExprStatement), value_(e) {}

  explicit ExprStatement(TokenStream& s);

  ~ExprStatement() override {
    delete value_;
  }

  void dump(std::ostream& o, size_t indent) const override {
    o << "expr[";
    value_->dump(o, indent);
    o << "]";
  }

  void alloca_stack(CompilerContext& c) const override;

  void each_statement(std::function<void(const Statement*)> func) const override {}

  void codegen(CompilerContext& c) const override;

  void analyze(CompilerContext& ctx) override {
    value_->analyze(ctx);
  }

  void each_value(const std::function<void(const Expression*)>& func) const override {
    value_->each_value(func);
  }

  static bool classof(const Node *n) {
    return n->getKind() == ND_ExprStatement;
  }
};

}  // namespace node
}  // namespace reir

#endif //PROJECT_AST_STATEMENT_HPP
