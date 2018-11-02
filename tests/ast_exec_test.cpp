//
// Created by kumagi on 18/06/19.
//

#include <string>

#include <gtest/gtest.h>

#include "reir/exec/compiler.hpp"
#include "reir/exec/ast_statement.hpp"
#include "reir/exec/ast_expression.hpp"
#include "reir/exec/compiler_context.hpp"

#include "reir/exec/parser.hpp"
#include "reir/exec/executor.hpp"
#include "reir/exec/storage.hpp"
#include "reir/exec/llvm_environment.hpp"
#include "reir/engine/foedus_runner.hpp"
#include "reir/engine/db_handle.hpp"
#include "reir/exec/db_interface.hpp"

#include "reir/exec/parser.hpp"
#include "reir/db/maybe_value.hpp"

namespace reir {

struct DummyDB : public DBInterface {

  std::string get_name() override {
    return DBInterface::get_name();
  }

  void define_functions(CompilerContext& ctx) override {}

  void emit_precommit_txn(CompilerContext& ctx) override {}

  void emit_begin_txn(CompilerContext& ctx) override {};

  void emit_insert(CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len, llvm::Value* value,
                   llvm::Value* value_len) override {}

  void emit_update(CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len, llvm::Value* value,
                   llvm::Value* value_len) override {}

  void emit_delete(CompilerContext& ctx, llvm::Value* key, llvm::Value* key_len) override {}

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

using namespace node;

class AstExecTest : public testing::Test {
protected:
  virtual void SetUp() override {
  }

  void compile_and_exec(node::Node* ast) {
    c.compile_and_exec(d, md, ast);
  }

  Compiler c;
  DummyDB d;
  MetaData md;
};


TEST_F(AstExecTest, empty_ast) {
  compile_and_exec(nullptr);
}

TEST_F(AstExecTest, empty_block) {
  node::Block blk{};
  compile_and_exec(&blk);
}

TEST_F(AstExecTest, empty_transaction) {
  node::Transaction txn("serializable", {});
  compile_and_exec(&txn);
}

TEST_F(AstExecTest, let) {
  node::Block blk({
      new Let("x", new PrimaryExpression(MaybeValue("hoge"))),
      new Let("y", new PrimaryExpression(MaybeValue(1LLU)))
  });
  compile_and_exec(&blk);
}

TEST_F(AstExecTest, print) {
  node::Block blk({
      new ExprStatement(
          new FunctionCall("print_int",
                           { new PrimaryExpression(MaybeValue(4649LLU)) })
      )
  });
  compile_and_exec(&blk);
}

TEST_F(AstExecTest, print_variable) {
  node::Block blk({
      new Let("x", new PrimaryExpression(MaybeValue("hoge\n"))),
      new ExprStatement(
          new FunctionCall("print_string",
                           { new PrimaryExpression(MaybeValue("foo\n")) })
      ),
      new ExprStatement(
          new FunctionCall("print_string",
                           { new PointerOf(new VariableReference("x")) })
      )
  });
  compile_and_exec(&blk);
}

TEST_F(AstExecTest, for_loop) {
  node::Block blk({
      new For(
          new Let("x", new PrimaryExpression(MaybeValue(0))),
          new BinaryExpression(new VariableReference("x"),
                               operators::LESSTHAN,
                               new PrimaryExpression(MaybeValue(10))),
          new Assign(new VariableReference("x"),
                     new BinaryExpression(new VariableReference("x"),
                                          operators::PLUS,
                                          new PrimaryExpression(MaybeValue(1)))),
          new Block({
                        new ExprStatement(
                            new FunctionCall("print_int",
                                             { new VariableReference("x") })
                        ),
          })
      )
  });
  compile_and_exec(&blk);
}

TEST_F(AstExecTest, emit) {
  node::Block blk({
      new Emit(
          new RowLiteral({new PrimaryExpression(MaybeValue(1)) })
      )
  });
  compile_and_exec(&blk);
}

TEST_F(AstExecTest, array) {
    node::Block blk({
        new Let("x", new ArrayLiteral({
            new PrimaryExpression(MaybeValue(1)),
            new PrimaryExpression(MaybeValue(2)),
            new PrimaryExpression(MaybeValue(3)),
            new PrimaryExpression(MaybeValue(4))
        })),
        new ExprStatement(
            new FunctionCall("print_int",
                { new ArrayReference(new VariableReference("x"), new PrimaryExpression(MaybeValue(2))) })
        ),
        new ExprStatement(
            new FunctionCall("print_int",
                { new ArrayReference(new VariableReference("x"), new PrimaryExpression(MaybeValue(1))) })
        )
    });
    compile_and_exec(&blk);
}

}  // namespace reir