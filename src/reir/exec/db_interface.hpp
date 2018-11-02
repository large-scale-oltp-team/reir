#ifndef REIR_DB_INTERFACE_HPP_
#define REIR_DB_INTERFACE_HPP_
#include <string>
#include "util/slice.hpp"

namespace llvm {
class Value;
}

namespace foedus {
namespace proc {
struct ProcArguments;
}
}
namespace leveldb {
class DB;
}

namespace reir {
/* it may hold FOEDUS or any other Transaction Engine */
class CompilerContext;
class CursorBase;

class DBInterface {
 public:
  virtual std::string get_name() {
    return name_;
  }

  virtual void define_functions(CompilerContext& ctx) = 0;
  virtual void emit_begin_txn(CompilerContext& ctx) = 0;
  virtual void emit_precommit_txn(CompilerContext& ctx) = 0;
  virtual void emit_insert(CompilerContext& ctx,
                           llvm::Value* key, llvm::Value* key_len,
                           llvm::Value* value, llvm::Value* value_len) = 0;
  virtual void emit_update(CompilerContext& ctx,
                           llvm::Value* key, llvm::Value* key_len,
                           llvm::Value* value, llvm::Value* value_len) = 0;

  virtual void emit_delete(CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len) = 0;
  virtual void emit_scan(CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len, llvm::Value* offset) = 0;
  virtual CursorBase* emit_get_cursor(CompilerContext& ctx,
                                      llvm::Value* from_prefix, llvm::Value* from_len,
                                      llvm::Value* to_prefix, llvm::Value* to_len) = 0;
  virtual llvm::Value* emit_cursor_next(CompilerContext& ctx, CursorBase* cursor) = 0;

  virtual void emit_cursor_copy_key(CompilerContext& ctx, CursorBase* c, llvm::Value* buffer) = 0;
  virtual void emit_cursor_copy_value(CompilerContext& ctx, CursorBase* c, llvm::Value* buffer) = 0;
  virtual void emit_cursor_destroy(CompilerContext& ctx, CursorBase* c) = 0;
  virtual llvm::Value* emit_is_valid_cursor(CompilerContext& ctx, CursorBase* c) = 0;
 private:
  std::string name_;
};

class FoedusInterface : public DBInterface {
public:
  const foedus::proc::ProcArguments* arg;
  FoedusInterface(const foedus::proc::ProcArguments* a) : arg(a) {}
  std::string get_name() override {
    return "FOEDUS";
  }
  void define_functions(CompilerContext& ctx) override;
  void emit_begin_txn(CompilerContext& ctx) override;
  void emit_precommit_txn(CompilerContext& ctx) override;
  void emit_insert(CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len, llvm::Value* value, llvm::Value* value_len) override;
  void emit_update(CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len, llvm::Value* value, llvm::Value* value_len) override;
  void emit_delete(CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len) override;
  void emit_scan(CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len, llvm::Value* offset) override;
  CursorBase* emit_get_cursor(CompilerContext& ctx,
                              llvm::Value* from_prefix, llvm::Value* from_len,
                              llvm::Value* to_prefix, llvm::Value* to_len) override;

  llvm::Value* emit_cursor_next(CompilerContext& ctx, CursorBase* c) override;
  void emit_cursor_copy_key(CompilerContext& ctx, CursorBase* c, llvm::Value* buffer) override;
  void emit_cursor_copy_value(CompilerContext& ctx, CursorBase* cursor, llvm::Value* buffer) override;
  void emit_cursor_destroy(CompilerContext& ctx, CursorBase* cursor) override;
  llvm::Value* emit_is_valid_cursor(CompilerContext& ctx, CursorBase* c) override;
};

}  // namespace reir

#endif  // REIR_DB_INTERFACE_HPP_
