//
// Created by kumagi on 18/02/20.
//
#include <iostream>

#include "db_interface.hpp"
#include "compiler_context.hpp"
#include "llvm_util.hpp"
#include <foedus/proc/proc_id.hpp>

namespace foedus {
namespace storage{
namespace masstree {
class MasstreeCursor;
}
}
}

namespace reir {

struct FoedusCursor : public CursorBase {
  llvm::Value* cursor;
};

void FoedusInterface::define_functions(CompilerContext& ctx) {
  std::vector<llvm::Type*> begin_xct_arg_types({
                                                   llvm::Type::getInt64PtrTy(ctx.ctx_)
                                               });
  llvm::FunctionType* begin_xct_signature =
      llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx.ctx_), begin_xct_arg_types, false);
  llvm::Function* begin_xct_func =
      llvm::Function::Create(begin_xct_signature,
                             llvm::Function::ExternalLinkage,
                             "begin_xct",
                             ctx.mod_.get());
  ctx.functions_table_["__begin_xct"] = begin_xct_func;


  // precommit
  std::vector<llvm::Type*> precommit_xct_arg_types({llvm::Type::getInt64PtrTy(ctx.ctx_)});
  llvm::FunctionType* precommit_xct_signature =
      llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx.ctx_), precommit_xct_arg_types, false);

  llvm::Function* precommit_xct_func =
      llvm::Function::Create(precommit_xct_signature,
                             llvm::Function::ExternalLinkage,
                             "precommit_xct",
                             ctx.mod_.get());
  ctx.functions_table_["__precommit_xct"] = precommit_xct_func;

  // insert
  std::vector<llvm::Type*> insert_args;
  insert_args.emplace_back(llvm::Type::getInt64PtrTy(ctx.ctx_));
  insert_args.emplace_back(llvm::Type::getInt8PtrTy(ctx.ctx_));
  insert_args.emplace_back(llvm::Type::getInt64Ty(ctx.ctx_));
  insert_args.emplace_back(llvm::Type::getInt8PtrTy(ctx.ctx_));
  insert_args.emplace_back(llvm::Type::getInt64Ty(ctx.ctx_));
  llvm::FunctionType* insert_signature =
      llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx.ctx_), insert_args, false);
  llvm::Function* insert_func =
      llvm::Function::Create(insert_signature,
                             llvm::Function::ExternalLinkage,
                             "foedus_insert",
                             ctx.mod_.get());
  ctx.functions_table_["__insert"] = insert_func;

  // generate cursor
  std::vector<llvm::Type*> generate_cursor_args;
  generate_cursor_args.emplace_back(llvm::Type::getInt64PtrTy(ctx.ctx_));
  generate_cursor_args.emplace_back(llvm::Type::getInt8PtrTy(ctx.ctx_));
  generate_cursor_args.emplace_back(llvm::Type::getInt64Ty(ctx.ctx_));
  generate_cursor_args.emplace_back(llvm::Type::getInt8PtrTy(ctx.ctx_));
  generate_cursor_args.emplace_back(llvm::Type::getInt64Ty(ctx.ctx_));
  llvm::FunctionType* generate_cursor_signature =
      llvm::FunctionType::get(llvm::Type::getInt64PtrTy(ctx.ctx_), generate_cursor_args, false);
  llvm::Function* generate_cursor_func =
      llvm::Function::Create(generate_cursor_signature,
                             llvm::Function::ExternalLinkage,
                             "foedus_generate_cursor",
                             ctx.mod_.get());
  ctx.functions_table_["__get_cursor"] = generate_cursor_func;

  // next_cursor
  std::vector<llvm::Type*> next_cursor_args = {
      llvm::Type::getInt64PtrTy(ctx.ctx_)  // cursor
  };

  ctx.functions_table_["__cursor_next"] =
      llvm::Function::Create(
          llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx.ctx_), next_cursor_args, false),
          llvm::Function::ExternalLinkage,
          "foedus_cursor_next",
          ctx.mod_.get());

  // cursor_is_valid
  ctx.functions_table_["__cursor_is_valid"] =
      llvm::Function::Create(
          llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx.ctx_), {
              llvm::Type::getInt64PtrTy(ctx.ctx_)
          }, false),
          llvm::Function::ExternalLinkage,
          "foedus_cursor_is_valid",
          ctx.mod_.get());

  // copy key
  std::vector<llvm::Type*> cursor_copy_key_args = {
      llvm::Type::getInt64PtrTy(ctx.ctx_),  // cursor
      llvm::Type::getInt8PtrTy(ctx.ctx_)  // buff
  };
  llvm::FunctionType* cursor_copy_key_signature =
      llvm::FunctionType::get(llvm::Type::getVoidTy(ctx.ctx_), cursor_copy_key_args, false);
  llvm::Function* cursor_copy_key_func =
      llvm::Function::Create(cursor_copy_key_signature,
                             llvm::Function::ExternalLinkage,
                             "foedus_cursor_copy_key",
                             ctx.mod_.get());
  ctx.functions_table_["__cursor_copy_key"] = cursor_copy_key_func;

  // copy value
  std::vector<llvm::Type*> cursor_copy_value_args = {
      llvm::Type::getInt64PtrTy(ctx.ctx_),
      llvm::Type::getInt8PtrTy(ctx.ctx_)  // buff
  };
  llvm::FunctionType* cursor_copy_value_signature =
      llvm::FunctionType::get(llvm::Type::getVoidTy(ctx.ctx_), cursor_copy_value_args, false);
  llvm::Function* cursor_copy_value_func =
      llvm::Function::Create(cursor_copy_value_signature,
                             llvm::Function::ExternalLinkage,
                             "foedus_cursor_copy_value",
                             ctx.mod_.get());
  ctx.functions_table_["__cursor_copy_value"] = cursor_copy_value_func;

  // destroy cursor
  std::vector<llvm::Type*> cursor_destroy_args = {
      llvm::Type::getInt64PtrTy(ctx.ctx_)  // cursor
  };
  llvm::FunctionType* cursor_destroy_signature =
      llvm::FunctionType::get(llvm::Type::getVoidTy(ctx.ctx_), cursor_destroy_args, false);
  ctx.functions_table_["__cursor_destroy"] =
      llvm::Function::Create(cursor_destroy_signature,
                             llvm::Function::ExternalLinkage,
                             "foedus_cursor_destroy",
                             ctx.mod_.get());
}

void FoedusInterface::emit_begin_txn(CompilerContext& ctx) {
  auto* begin_xct_func = ctx.functions_table_["__begin_xct"];
  std::vector<llvm::Value*> begin_xct_arg({get_ptr(ctx, arg)});
  ctx.builder_.CreateCall(begin_xct_func, begin_xct_arg);
}

void FoedusInterface::emit_precommit_txn(CompilerContext& ctx) {
  auto* precommit_xct_func = ctx.functions_table_["__precommit_xct"];
  std::vector<llvm::Value*> precommit_xct_arg({get_ptr(ctx, arg)});
  ctx.builder_.CreateCall(precommit_xct_func, precommit_xct_arg);
}

void FoedusInterface::emit_insert(CompilerContext& ctx,
                                  llvm::Value* key, llvm::Value* key_len,
                                  llvm::Value*value, llvm::Value* value_len)  {
  auto* insert_func = ctx.functions_table_["__insert"];
  std::vector<llvm::Value*> insert_arg{
      get_ptr(ctx, arg),
      key, key_len,
      value, value_len};
  ctx.builder_.CreateCall(insert_func, insert_arg);
}

CursorBase* FoedusInterface::emit_get_cursor(reir::CompilerContext& ctx, llvm::Value* from_prefix,
                                             llvm::Value* from_len, llvm::Value* to_prefix, llvm::Value* to_len) {
  auto* get_cursor_func = ctx.functions_table_["__get_cursor"];
  std::vector<llvm::Value*> get_cursor_arg{
      get_ptr(ctx, arg),
      from_prefix, from_len,
      to_prefix, to_len};
  auto* ret = new FoedusCursor;
  ret->cursor = ctx.builder_.CreateCall(get_cursor_func, get_cursor_arg);
  return ret;
}

llvm::Value* FoedusInterface::emit_is_valid_cursor(reir::CompilerContext& ctx,
                                               CursorBase* c) {
  auto* func = ctx.functions_table_["__cursor_is_valid"];
  auto* fc = reinterpret_cast<FoedusCursor*>(c);
  return ctx.builder_.CreateCall(func, {fc->cursor});
}

void FoedusInterface::emit_cursor_copy_key(reir::CompilerContext& ctx,
                                           CursorBase* c,
                                           llvm::Value* buffer) {
  auto* func = ctx.functions_table_["__cursor_copy_key"];
  auto* fc = reinterpret_cast<FoedusCursor*>(c);
  std::vector<llvm::Value*> args = {fc->cursor, buffer};
  ctx.builder_.CreateCall(func, args);
}

llvm::Value* FoedusInterface::emit_cursor_next(reir::CompilerContext& ctx,
                                               CursorBase* c) {
  auto* func = ctx.functions_table_["__cursor_next"];
  auto* fc = reinterpret_cast<FoedusCursor*>(c);
  return ctx.builder_.CreateCall(func, {fc->cursor});
}

void FoedusInterface::emit_cursor_copy_value(CompilerContext& ctx, CursorBase* c, llvm::Value* buffer) {
  auto* func = ctx.functions_table_["__cursor_copy_value"];
  auto* fc = reinterpret_cast<FoedusCursor*>(c);
  std::vector<llvm::Value*> args = {fc->cursor, buffer};
  ctx.builder_.CreateCall(func, args);
}

void FoedusInterface::emit_cursor_destroy(CompilerContext& ctx, CursorBase* c) {
  auto* func = ctx.functions_table_["__cursor_destroy"];
  auto* fc = reinterpret_cast<FoedusCursor*>(c);
  std::vector<llvm::Value*> args = {fc->cursor};
  ctx.builder_.CreateCall(func, args);
}

void FoedusInterface::emit_update(CompilerContext& ctx,
                                  llvm::Value* key, llvm::Value* key_len,
                                  llvm::Value* value, llvm::Value* value_len)  {
  auto* thread = arg->context_;
  auto* engine = arg->engine_;
}

void FoedusInterface::emit_delete(CompilerContext& ctx,
                                  llvm::Value* key, llvm::Value* key_len)  {

}

void FoedusInterface::emit_scan(CompilerContext& ctx,
                                llvm::Value* key, llvm::Value* key_len,
                                llvm::Value* offset)  {

}

}  // namespace reir