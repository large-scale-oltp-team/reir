//
// Created by kumagi on 18/04/04.
//

#include <gtest/gtest.h>
#include <reir/exec/debug.hpp>
#include <reir/exec/compiler_context.hpp>
#include "reir/exec/compiler.hpp"

#include "reir/exec/parser.hpp"
#include "reir/exec/executor.hpp"
#include "reir/exec/storage.hpp"
#include "reir/exec/llvm_environment.hpp"
#include "reir/engine/foedus_runner.hpp"
#include "reir/engine/db_handle.hpp"
#include "reir/exec/db_interface.hpp"
#include "reir/exec/parser.hpp"

namespace reir {

struct DummyDB : public DBInterface {
  std::string get_name()  override {
    return DBInterface::get_name();
  }

  void define_functions(CompilerContext& ctx) override {}

  void emit_precommit_txn(CompilerContext& ctx)  override {}

  void emit_begin_txn(CompilerContext& ctx)  override {};

  void emit_insert( CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len, llvm::Value* value,
                   llvm::Value* value_len)  override {
  }

  void emit_update( CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len, llvm::Value* value,
                   llvm::Value* value_len)  override {}

  void emit_delete( CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len)  override {}

  void emit_scan( CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len, llvm::Value* offset)  override {}

  CursorBase* emit_get_cursor(CompilerContext& ctx,
                              llvm::Value* from_prefix, llvm::Value* from_len,
                              llvm::Value* to_prefix, llvm::Value* to_len) override {
    return nullptr;
  }
  llvm::Value* emit_cursor_next(CompilerContext& ctx, CursorBase* cursor) override {
    return ctx.builder_.getInt1(false);
  }
  llvm::Value* emit_is_valid_cursor(CompilerContext& ctx, CursorBase* cursor) override {
    return ctx.builder_.getInt1(false);
  }
  void emit_cursor_copy_key(CompilerContext& ctx, CursorBase* c, llvm::Value* buffer) override {}
  void emit_cursor_copy_value(CompilerContext& ctx, CursorBase* c, llvm::Value* buffer) override {}
  void emit_cursor_destroy(CompilerContext& ctx, CursorBase* c) override {}
};

class CompilerTest : public testing::Test {
 protected:
  virtual void SetUp() override {
  }
  void compile_and_exec(const std::string& ir) {
    parse(ir, [&](node::Node* ast) {
      c.compile_and_exec(d, md, ast);
    });
  }

  Compiler c;
  DummyDB d;
  MetaData md;
};

TEST_F(CompilerTest, ruct) {
}

TEST_F(CompilerTest, empty_code) {
  compile_and_exec("");
}

TEST_F(CompilerTest, empty_block) {
  compile_and_exec("{}");
}

TEST_F(CompilerTest, just_value) {
  compile_and_exec("let x = 123");
  compile_and_exec("print_int(3)");
}

TEST_F(CompilerTest, expr) {
  compile_and_exec("1+2");
  compile_and_exec("1+2*3/4-5");
  compile_and_exec("1*(2+3)/4*(5-6-7)-8");
  // compile_and_exec("x + y - 4*(3-y)");
}

TEST_F(CompilerTest, print_expr) {
  compile_and_exec("print_int(1+2+3*4)");
}

TEST_F(CompilerTest, inner_transaction) {
  compile_and_exec("transaction {}");
  compile_and_exec("transaction{"
//                   " insert(hoge, [fo, 1])"
                   "}");
}

TEST_F(CompilerTest, if_stmt) {
  compile_and_exec("if 1 {"
                   "print_int(10)"
                   "}");
  compile_and_exec("if 10 < 10 {"
                   "print_int(10)"
                   "} else {"
                   "print_int(120)"
                   "}");
}

TEST_F(CompilerTest, define_tuple) {
  compile_and_exec("tuple human { int:name, int:score}");
}

TEST_F(CompilerTest, let_assign) {
  compile_and_exec("let x = 10");
  compile_and_exec("let x = 12"
                   " x = 18"
                   " print_int(x)");
  compile_and_exec("let x = 2"
                   " if x {"
                   " print_int(x)"
                   "}");
}

TEST_F(CompilerTest, for_loop) {
  compile_and_exec("for let x=0; x < 256; x = x + 1 {"
                   " print_int(x*x)"
                   "}");
  compile_and_exec("let x = 0"
                   " let y = 0"
                   " for x=0; x < 100; x = x + 1 {"
                   "   for y=0; y < 100; y = y + 1 {"
                   "     if (x * y) < 10 {"
                   "       print_int(x*y)"
                   "     }"
                   "   }"
                   " }");
}

TEST_F(CompilerTest, create_table) {
  compile_and_exec("define <{int:a}> foo");
}

TEST_F(CompilerTest, string_literal) {
  compile_and_exec("let x = \"fooo\n\"");
  compile_and_exec("print_string(\"hoooo\n\")");
  compile_and_exec("let x = \"fooo\n\""
                   " print_string(&x)");
}

TEST_F(CompilerTest, literals) {
  compile_and_exec("let x = {1,2,3,4}");
  compile_and_exec("let x = [1,2,3,4]");
  compile_and_exec("let x = [1,2,3,4]\n"
                   "print_int(x[2])\n"
                   "print_int(x[1])\n"
                   "print_int(x[0])\n"
                   "print_int(x[3])\n");
}

TEST_F(CompilerTest, emit) {
  compile_and_exec("emit {42}");
  compile_and_exec("let x = {1,2,3,4}\n"
                   "emit x");
  compile_and_exec("for let x = 0; x < 20; x = x + 1 {\n"
                   "  emit {x, x+1, x+2, x+3}\n"
                   "}");
}

TEST_F(CompilerTest, insert) {
  compile_and_exec("define<{int:a key, int:b}> foo\n"
                   "insert foo {1, 2}");
}
TEST_F(CompilerTest, full_scan) {
  compile_and_exec("define<{int:x key, int:y, int:z}> test\n"
                   "transaction {\n"
                   "  scan test, row {\n"
                   "    emit row"
                   "  }"
                   "}");
}

TEST_F(CompilerTest, member_read) {
  compile_and_exec("let <{int:x key, int:y}> p = {1,4}\n"
                   "print_int(p.x)\n"
                   "print_int(p.y)\n"
                   "print_int(p.x)");
}

TEST_F(CompilerTest, emit_member) {
  compile_and_exec("transaction {"
                   "  define<{int:x key, int:y, int:z}> fuga\n"
                   "  insert fuga {1,2,3}"
                   "  scan fuga, row {"
                   "    print_int(row.x)"
                   "    emit {row.x, row.z}\n"
                   "  }"
                   "}");
}

}  // namespace reir
