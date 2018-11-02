//
// Created by kumagi on 18/06/15.
//
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>
#include "compiler_context.hpp"
#include "db_interface.hpp"
#include "debug.hpp"
#include "reir/exec/llvm_environment.hpp"
#include "compiler.hpp"

namespace reir {

CompilerContext::CompilerContext(Compiler& c, DBInterface* dbi, MetaData* md)
    : ctx_(),
      mod_(new llvm::Module("global_module", ctx_)),
      builder_(ctx_),
      dbi_(dbi),
      md_(md),
      target_machine_(c.get_target_machine()),
      in_txn_(false) {
  func_ = llvm::Function::Create(
              llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx_),
                                      {llvm::Type::getInt64PtrTy(ctx_)},
                                      false),
              llvm::Function::ExternalLinkage,
              get_name(),
              mod_.get());
  auto iter = func_->arg_begin();
  iter->setName("ast");
  mod_->setDataLayout(target_machine_->createDataLayout());
  loop_ = nullptr;

  // default types
  analyze_type_table_.emplace("integer", new node::PrimaryType(node::type_id::INTEGER));
  analyze_type_table_.emplace("string", new node::PrimaryType(node::type_id::STRING));
  analyze_type_table_.emplace("date", new node::PrimaryType(node::type_id::DATE));
  analyze_type_table_.emplace("double", new node::PrimaryType(node::type_id::DOUBLE));
  analyze_type_table_.emplace("bool", new node::PrimaryType(node::type_id::BOOL));

  define_functions();
}

void CompilerContext::define_functions() {
  dbi_->define_functions(*this);
}

CompilerContext::~CompilerContext() {
  auto* ptr = loop_;
  while (ptr) {
    auto* next = ptr->out_;
    delete ptr;
    ptr = next;
  }
  for (auto& a : local_schema_table_) {
    delete a.second;
  }
  for (auto& it : analyze_type_table_) {
    delete it.second;
  }
}

CursorBase* CompilerContext::get_cursor(llvm::Value* from_prefix, llvm::Value* from_len,
                         llvm::Value* to_prefix, llvm::Value* to_len) {
  return dbi_->emit_get_cursor(*this, from_prefix, from_len, to_prefix, to_len);
}

void CompilerContext::init() {
  mod_ = llvm::make_unique<llvm::Module>("global_module", ctx_);
  mod_->setDataLayout(target_machine_->createDataLayout());
}

void CompilerContext::get_output(char* buff, uint64_t length) {
  RawRow r(buff, length);
  outputs_.emplace_back(std::move(r));
}

void CompilerContext::dump() const {
  llvm::outs() << *mod_;
}

void CompilerContext::emit_begin_txn() {
  dbi_->emit_begin_txn(*this);
}

void CompilerContext::emit_precommit_txn() {
  dbi_->emit_precommit_txn(*this);
}

void CompilerContext::emit_insert(llvm::Value* key, llvm::Value* key_len,
                                  llvm::Value* value, llvm::Value* value_len) {
  dbi_->emit_insert(*this, key, key_len, value, value_len);
  LLVM_DUMP(key);
}

llvm::Value* CompilerContext::emit_cursor_next(CursorBase* cursor) {
  return dbi_->emit_cursor_next(*this, cursor);
}

llvm::Value* CompilerContext::emit_is_valid_cursor(reir::CursorBase* cursor) {
  return dbi_->emit_is_valid_cursor(*this, cursor);
}

void CompilerContext::emit_cursor_copy_key(CursorBase* c, llvm::Value* buffer) {
  dbi_->emit_cursor_copy_key(*this, c, buffer);
}
void CompilerContext::emit_cursor_copy_value(CursorBase* c, llvm::Value* buffer) {
  dbi_->emit_cursor_copy_value(*this, c, buffer);
}

void CompilerContext::emit_cursor_destroy(reir::CursorBase* c) {
  dbi_->emit_cursor_destroy(*this, c);
}

std::string CompilerContext::get_name() const {
  return "__top_function";
}

}  // namespace reir