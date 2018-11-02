//
// Created by kumagi on 18/07/04.
//


#include <llvm/Support/raw_ostream.h>
#include "ast_node.hpp"
#include "ast_expression.hpp"
#include "ast_statement.hpp"

#include "compiler_context.hpp"
#include "debug.hpp"
#include "llvm_util.hpp"
#include "reir/db/metadata.hpp"

namespace reir {
namespace node {

void Block::codegen(CompilerContext& c) const {
  for (auto& s : statements_) {
    s->codegen(c);
  }
}

void Transaction::codegen(CompilerContext& c) const {
  c.emit_begin_txn();
  for (auto& s : sequence_->statements_) {
    s->codegen(c);
  }
  c.emit_precommit_txn();
}

void Let::alloca_stack(CompilerContext& c) const {
  auto* type = expr_->get_type(c);
  auto* store = c.builder_.CreateAlloca(type, nullptr, name_);
  store->setAlignment(8);
  c.variable_table_[name_] = store;
}

void Let::codegen(CompilerContext& c) const {
  llvm::Type* type = expr_->get_type(c);
  auto* stack = c.variable_table_[name_];
  if (stack == nullptr) {
    throw std::runtime_error("undefined variable: " + name_);
  }
  c.builder_.CreateStore(expr_->get_value(c), stack);
}

void If::alloca_stack(reir::CompilerContext& c) const {
  if (true_block_) true_block_->alloca_stack(c);
  if (false_block_) false_block_->alloca_stack(c);
}

void If::codegen(CompilerContext& c) const {
  llvm::BasicBlock* true_block =
      llvm::BasicBlock::Create(c.ctx_, "true_blk", c.func_);
  llvm::BasicBlock* false_block =
      llvm::BasicBlock::Create(c.ctx_, "false_blk", c.func_);
  llvm::BasicBlock* fin =
      llvm::BasicBlock::Create(c.ctx_, "fin", c.func_);

  auto* cond = cond_->get_value(c);
  if (cond->getType() != llvm::Type::getInt1Ty(c.ctx_)) {
    cond = c.builder_.CreateICmpNE(cond, c.builder_.getInt64(0));
  }

  c.builder_.CreateCondBr(cond, true_block, false_block);
  c.builder_.SetInsertPoint(true_block);

  // true
  true_block_->codegen(c);
  c.builder_.CreateBr(fin);

  // false if exists
  c.builder_.SetInsertPoint(false_block);
  if (false_block_) {
    false_block_->codegen(c);
  }
  c.builder_.CreateBr(fin);
  c.builder_.SetInsertPoint(fin);
}

void ExprStatement::alloca_stack(CompilerContext& c) const {
  auto* type = value_->get_type(c);
  if (type->isStructTy()) {
    auto* alloc = c.builder_.CreateAlloca(type, nullptr, "expr");
    c.stack_table_[this] = alloc;
  }
  value_->each_value([&](const Expression* expr) {
    auto* expr_type = expr->get_type(c);
    assert(expr_type);
    if (expr_type->isStructTy()) {
      auto* st_type = expr->get_type(c);
      auto* alloc = c.builder_.CreateAlloca(st_type, nullptr, "expr");
      c.stack_table_[expr] = alloc;
    }
  });
}

void ExprStatement::codegen(reir::CompilerContext& c) const {
  value_->get_value(c);
}

void Define::alloca_stack(reir::CompilerContext& c) const {
  // CAUTION: this code does not emit LLVM-IR, schema creation is done in compilation phase now.
  std::vector<Attribute> attrs;
  auto& types = schema_->types_;
  for (size_t i = 0; i < schema_->names_.size(); ++i) {
    if (types[i]->type_ == type_id::INTEGER) {
      attrs.emplace_back(schema_->names_[i], AttrType(AttrType::INTEGER), schema_->props_[i]);
    } else if (types[i]->type_ == type_id::DOUBLE) {
      attrs.emplace_back(schema_->names_[i], AttrType(AttrType::DOUBLE), schema_->props_[i]);
    } else if (types[i]->type_ == type_id::STRING) {
      if (schema_->params_[i]) {
        attrs.emplace_back(schema_->names_[i], AttrType(AttrType::STRING, (size_t)*schema_->params_[i]), schema_->props_[i]);
      } else {
        attrs.emplace_back(schema_->names_[i], AttrType(AttrType::STRING, 0), schema_->props_[i]);
      }
    }
  }

  c.local_schema_table_[name_] = new Schema(name_, attrs);
  c.md_->create_table(name_, std::move(attrs));
}

void Define::codegen(CompilerContext& c) const {
}

void For::codegen(reir::CompilerContext& c) const {
  if (init_) {
    init_->codegen(c);
  }
  llvm::BasicBlock* begin =
      llvm::BasicBlock::Create(c.ctx_, "for_begin", c.func_);
  llvm::BasicBlock* body =
      llvm::BasicBlock::Create(c.ctx_, "for_body", c.func_);
  llvm::BasicBlock* every =
      llvm::BasicBlock::Create(c.ctx_, "for_every", c.func_);
  llvm::BasicBlock* finish =
      llvm::BasicBlock::Create(c.ctx_, "for_finish", c.func_);
  c.enter_loop(begin, body, every, finish);

  c.builder_.CreateBr(begin);

  c.builder_.SetInsertPoint(begin);
  if (cond_) {
    auto* ret = cond_->get_value(c);
    c.builder_.CreateCondBr(ret, body, finish);
  }
  c.builder_.SetInsertPoint(body);
  blk_->codegen(c);
  c.builder_.CreateBr(every);

  c.builder_.SetInsertPoint(every);
  if (every_) {
    every_->get_value(c);
  }

  c.builder_.CreateBr(begin);

  c.builder_.SetInsertPoint(finish);

  c.exit_loop_ctx();
}

void Jump::codegen(reir::CompilerContext& c) const {
  auto* current_loop = c.loop_;
  switch (t_) {
    case type::continue_jump: {
      c.builder_.CreateBr(current_loop->every_);
      break;
    }
    case type::break_jump: {
      c.builder_.CreateBr(current_loop->finish_);
      break;
    }
    default: {
      throw std::runtime_error("unknown jump type");
    }
  }
}

void Emit::codegen(CompilerContext& c) const {
  auto* stack = c.stack_table_[this];
  if (value_->get_type(c)->isStructTy()) {
    auto value = value_->get_value(c);
    c.builder_.CreateStore(value, stack);
    auto* emit_func = c.functions_table_["__emit_func"];
    auto* type = llvm::dyn_cast<llvm::StructType>(value_->get_type(c));
    const uint64_t elms = type->getNumElements();
    size_t size = 0;
    for (size_t i = 0; i < elms; ++i) {
      llvm::Type* elm = type->getElementType(static_cast<unsigned int>(i));
      size += (elm->getPrimitiveSizeInBits() + 7) / 8;
    }

    auto* from = c.builder_.CreateBitCast(stack, llvm::Type::getInt8PtrTy(c.ctx_));
    std::vector<llvm::Value*> args{ get_ptr(c, &c), from, c.builder_.getInt64(size)};
    c.builder_.CreateCall(emit_func, args);
  } else {
    throw std::runtime_error("non struct type cant be emitted");
  }
}

void Emit::alloca_stack(CompilerContext& c) const {
  auto* type = value_->get_type(c);
  auto* stack = c.builder_.CreateAlloca(type, nullptr, "emit_stack");
  stack->setAlignment(8);
  c.stack_table_[this] = stack;
}

llvm::Constant* find_or_create_prefix(CompilerContext& c, const std::string& prefix, const std::string & name) {
  auto it = c.global_variable_table_.find(prefix);
  llvm::Constant* ret;
  if (it == c.global_variable_table_.end()) {
    ret = llvm::dyn_cast<llvm::Constant>(c.builder_.CreateGlobalStringPtr(prefix, name));
    c.global_variable_table_.emplace(prefix, ret);
  } else {
    ret = it->second;
  }
  return ret;
}

void Insert::codegen(CompilerContext& c) const {
  const auto* schema = c.local_schema_table_[table_];
  auto* row = value_->get_value(c);
  std::string key_prefix = schema->get_key_prefix();
  if (value_->get_type(c)->isStructTy()) {
    auto* row_type = llvm::dyn_cast<llvm::StructType>(value_->get_type(c));
    int elements = row_type->getNumElements();
    c.builder_.CreateStore(row, tuple_stack_);
    auto* buff = c.builder_.CreateBitCast(tuple_stack_, c.builder_.getInt8PtrTy());

    c.builder_.CreateMemCpy(key_stack_, prefix_, c.builder_.getInt64(key_prefix.size()), 8);
    auto key_idx = static_cast<int>(key_prefix.size());
    int value_idx = 0;
    int offset = 0;
    for (int i = 0; i < elements; ++i) {
      auto* src = c.builder_.CreateInBoundsGEP(buff,
                                               {
                                                c.builder_.getInt32(
                                                    static_cast<uint32_t>(offset))});
      if (schema->is_key(i)) {
        auto* dst = c.builder_.CreateInBoundsGEP(key_stack_,
                                                 {c.builder_.getInt64(0),
                                                  c.builder_.getInt32(
                                                      static_cast<uint32_t>(key_idx))});
        c.builder_.CreateMemCpy(dst, src, schema->get_tuple_length(i), 8);
        key_idx += schema->get_tuple_length(i);
      } else {
        auto* dst = c.builder_.CreateInBoundsGEP(value_stack_,
                                                 {c.builder_.getInt32(0),
                                                  c.builder_.getInt32(
                                                      static_cast<uint32_t>(value_idx))});
        c.builder_.CreateMemCpy(dst, src, schema->get_tuple_length(i), 8);
        value_idx += schema->get_tuple_length(i);
      }
      offset += schema->get_tuple_length(i);
    }
    c.emit_insert(c.builder_.CreateBitCast(key_stack_, c.builder_.getInt8PtrTy()),
                  c.builder_.getInt64(static_cast<uint64_t>(key_idx)),
                  c.builder_.CreateBitCast(value_stack_, c.builder_.getInt8PtrTy()),
                  c.builder_.getInt64(static_cast<uint64_t>(value_idx)));
  } else {
    throw std::runtime_error("non row type cant be inserted");
  }
}

void Insert::alloca_stack(CompilerContext& c) const {
  const auto* schema = c.local_schema_table_[table_];
  if (schema == nullptr) {
    throw std::runtime_error("undefined table :" + table_);
  }
  if (!schema->fixed_key_length()) {
    throw std::runtime_error("fixed length key is available");
  }
  if (!schema->fixed_value_length()) {
    throw std::runtime_error("fixed length value is available");
  }
  auto prefix = schema->get_key_prefix();
  prefix_ = find_or_create_prefix(c, prefix, table_ + "_table_prefix");

  auto* key_stk = llvm::ArrayType::get(c.builder_.getInt8Ty(), schema->get_fixed_key_length());
  auto* val_stk = llvm::ArrayType::get(c.builder_.getInt8Ty(), schema->get_fixed_value_length());

  if (key_stack_) {
    throw std::runtime_error("key_stack is already initialized");
  }
  key_stack_ = c.builder_.CreateAlloca(key_stk, nullptr, "insert_key_stack");
  value_stack_ = c.builder_.CreateAlloca(val_stk, nullptr, "insert_val_stack");

  auto* buff_type = value_->get_type(c);
  tuple_stack_ = c.builder_.CreateAlloca(buff_type, nullptr, "tuple_stack");
}

void Scan::codegen(CompilerContext& c) const {
  const auto* schema = c.local_schema_table_[table_];
  std::string key_prefix = schema->get_key_prefix();
  std::string key_prefix_end = key_prefix;
  key_prefix_end[key_prefix_end.size() - 2]++;

  auto* rowtype = c.type_table_[row_name_];

  auto* cursor = c.get_cursor(prefix_begin_, c.builder_.getInt64(key_prefix.size()),
                              prefix_end_, c.builder_.getInt64(key_prefix_end.size()));
  llvm::BasicBlock* check =
      llvm::BasicBlock::Create(c.ctx_, "fullscan_check", c.func_);
  llvm::BasicBlock* begin =
      llvm::BasicBlock::Create(c.ctx_, "fullscan_begin", c.func_);
  llvm::BasicBlock* fin =
      llvm::BasicBlock::Create(c.ctx_, "fullscan_fin", c.func_);

  c.builder_.CreateBr(check);
  c.builder_.SetInsertPoint(check);
  auto* cond = c.builder_.CreateICmpEQ(c.emit_is_valid_cursor(cursor), c.builder_.getInt1(true));
  c.builder_.CreateCondBr(cond, begin, fin);

  c.builder_.SetInsertPoint(begin);
  auto* key = c.builder_.CreateBitCast(key_stack_, llvm::Type::getInt8PtrTy(c.ctx_));
  c.emit_cursor_copy_key(cursor, key);
  auto* value = c.builder_.CreateBitCast(value_stack_, llvm::Type::getInt8PtrTy(c.ctx_));
  c.emit_cursor_copy_value(cursor, value);

 	llvm::Value* prev = llvm::UndefValue::get(rowtype);
  uint32_t key_offset = static_cast<uint32_t>(key_prefix.size()), value_offset = 0;
  for (uint64_t i = 0; i < schema->columns(); ++i) {
    if (schema->is_key((int)i)) {
      auto* offset_key = c.builder_.CreateInBoundsGEP(key,
                                                      {
                                                       c.builder_.getInt32(key_offset)});
      auto* key_r = c.builder_.CreateBitCast(offset_key, llvm::Type::getInt64PtrTy(c.ctx_));
      auto* record = c.builder_.CreateLoad(key_r);


      prev = c.builder_.CreateInsertValue(prev, record, i);
      key_offset += 8;
    } else {
      auto* offset_value = c.builder_.CreateInBoundsGEP(value_stack_,
                                                        {c.builder_.getInt32(0),
                                                         c.builder_.getInt32(value_offset)});
      auto* value_r = c.builder_.CreateBitCast(offset_value, llvm::Type::getInt64PtrTy(c.ctx_));
      auto* record = c.builder_.CreateLoad(value_r);

      prev = c.builder_.CreateInsertValue(prev, record, i);
      value_offset += 8;
    }
  }

  // auto* row = c.builder_.CreateBitCast(tuple_stack_, rowtype->getPointerTo());
  c.builder_.CreateStore(prev, tuple_stack_);

  blk_->codegen(c);
  c.emit_cursor_next(cursor);
  c.builder_.CreateBr(check);

  c.builder_.SetInsertPoint(fin);
  c.emit_cursor_destroy(cursor);
  delete cursor;
}

void Scan::alloca_stack(CompilerContext& c) const {
  const auto* schema = c.local_schema_table_[table_];
  if (!schema->fixed_key_length()) {
    throw std::runtime_error("fixed length row is available");
  }
  if (!schema->fixed_value_length()) {
    throw std::runtime_error("fixed length row is available");
  }
  auto prefix = schema->get_key_prefix();
  prefix_begin_ = find_or_create_prefix(c, prefix, table_ + "_table_prefix_begin");
  auto prefix_end = prefix;
  prefix_end[prefix_end.size() - 2]++;
  prefix_end_ = find_or_create_prefix(c, prefix_end, table_ + "_table_prefix_end");

  auto* key_stk = llvm::ArrayType::get(c.builder_.getInt8Ty(), schema->get_fixed_key_length());
  auto* val_stk = llvm::ArrayType::get(c.builder_.getInt8Ty(), schema->get_fixed_value_length());

  if (key_stack_) {
    throw std::runtime_error("key_stack is already initialized");
  }
  key_stack_ = c.builder_.CreateAlloca(key_stk, nullptr, "scan_key_stack");
  value_stack_ = c.builder_.CreateAlloca(val_stk, nullptr, "scan_val_stack");


  std::vector<llvm::Type*> row_attrs;
  schema->each_attr([&](size_t idx, const Attribute& attr) {
    if (attr.type().is_integer()) {
      row_attrs.emplace_back(c.builder_.getInt64Ty());
    } else {
      throw std::runtime_error("non-integer type is not supported yet");
    }
  });
  auto* row_type = llvm::StructType::create(c.ctx_, row_attrs, row_name_);
  c.type_table_[row_name_] = row_type;
  auto* store = c.builder_.CreateAlloca(row_type, nullptr, row_name_);
  store->setAlignment(8);
  tuple_stack_ = c.variable_table_[row_name_] = store;
}

}  // namespace node
}  // namespace reir