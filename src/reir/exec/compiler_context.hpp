//
// Created by kumagi on 18/01/19.
//

#ifndef PROJECT_COMPILER_CONTEXT_HPP
#define PROJECT_COMPILER_CONTEXT_HPP

#include <unordered_map>
#include <utility>
#include "reir/db/schema.hpp"
#include <llvm/IR/IRBuilder.h>
#include "db_interface.hpp"
#include "ast_node.hpp"
#include "llvm_environment.hpp"

namespace llvm {
class LLVMContext;
class Function;
class Type;
class Module;
class AllocaInst;
class FunctionType;
class BasicBlock;
class Value;
class TargetMachine;
}  // namespace llvm

namespace reir {
namespace node {
class Expression;
class Type;
class Node;
}
class Compiler;
class MetaData;

class DBInterface;

struct RawRow {
  char* buff_;
  uint64_t len_;

  RawRow(char* buff, uint64_t l) : buff_(new char[l]), len_(l) {
    std::memcpy(buff_, buff, len_);
  }

  ~RawRow() {
    delete[] buff_;
  }

  friend std::ostream& operator<<(std::ostream& o, const RawRow& r) {
    size_t size = r.len_ / 8;
    auto* data = reinterpret_cast<uint64_t*>(r.buff_);
    o << "[";
    for (size_t i = 0; i < size; ++i) {
      if (0 < i) { o << ", "; }
      o << *data++;
    }
    o << "]";
    return o;
  }

  RawRow(RawRow&& o) noexcept : buff_(o.buff_), len_(o.len_) {
    o.buff_ = nullptr;
    o.len_ = 0;
  }
};

struct LoopContext {
  llvm::BasicBlock* begin_;
  llvm::BasicBlock* body_;
  llvm::BasicBlock* every_;
  llvm::BasicBlock* finish_;
  LoopContext* out_{};

  LoopContext(llvm::BasicBlock* b,
              llvm::BasicBlock* bo,
              llvm::BasicBlock* e,
              llvm::BasicBlock* f)
      : begin_(b), body_(bo), every_(e), finish_(f) {}


};

struct CursorBase {
  virtual ~CursorBase() = default;
};

struct CompilerContext {
  CompilerContext(Compiler& c, DBInterface* dbi, MetaData* md);

  void define_functions();
  void dump() const;
  void emit_begin_txn();
  void emit_precommit_txn();
  void emit_insert(llvm::Value*key, llvm::Value*key_len, llvm::Value*value, llvm::Value* value_len);
  CursorBase* get_cursor(llvm::Value* from_prefix, llvm::Value* from_len,
                         llvm::Value* to_prefix, llvm::Value* to_len);
  void emit_cursor_destroy(CursorBase* c);
  llvm::Value* emit_cursor_next(CursorBase* cursor);
  llvm::Value* emit_is_valid_cursor(CursorBase* cursor);
  void emit_cursor_copy_key(CursorBase* c, llvm::Value* buffer);
  void emit_cursor_copy_value(CursorBase* c, llvm::Value* buffer);
  void init();
  void get_output(char* buff, uint64_t length);
  MetaData* get_metadata() { return md_; }
  std::string get_name() const;

  void enter_loop(llvm::BasicBlock* b,
              llvm::BasicBlock* bo,
              llvm::BasicBlock* e,
              llvm::BasicBlock* f) {
    auto* new_loop = new LoopContext(b,bo, e, f);
    new_loop->out_ = loop_;
    loop_ = new_loop;
  }

  void exit_loop_ctx() {
    auto* outer_loop = loop_->out_;
    delete loop_;
    loop_ = outer_loop;
  }
  ~CompilerContext();

  std::unordered_map<std::string, reir::Schema*> local_schema_table_;
  std::unordered_map<std::string, node::Type*> analyze_type_table_;
  std::unordered_map<std::string, llvm::Type*> type_table_;
  std::unordered_map<std::string, llvm::Function*> functions_table_;
  std::unordered_map<std::string, reir::node::Type*> variable_type_table_;
  std::unordered_map<std::string, llvm::AllocaInst*> variable_table_;
  std::unordered_map<const node::Node*, llvm::Value*> stack_table_;
  std::unordered_map<std::string, llvm::Constant*> global_variable_table_;
  std::vector<RawRow> outputs_;
  LoopContext* loop_;

  llvm::LLVMContext ctx_;
  std::unique_ptr<llvm::Module> mod_;
  llvm::Function* func_;
  llvm::IRBuilder<> builder_;
  DBInterface* dbi_;
  MetaData* md_;
  llvm::TargetMachine* target_machine_;
  bool in_txn_;
};
}

#endif //PROJECT_COMPILER_CONTEXT_HPP
