#include <string>

#include <gtest/gtest.h>

#include "reir/exec/compiler.hpp"
#include "reir/exec/ast_statement.hpp"
#include "reir/exec/compiler_context.hpp"

#include "reir/exec/executor.hpp"
#include "reir/exec/storage.hpp"
#include "reir/engine/foedus_runner.hpp"

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

class AstExprTest : public testing::Test {
protected:
  virtual void SetUp() override {
  }

  void compile_and_exec(node::Node* ast) {
    CompilerContext ctx(c, &d, &md);
    c.compile_and_exec(ctx, d, md, ast);
    results = std::move(ctx.outputs_);
  }
  void expect(bool, Expression* expr);

  std::vector<RawRow> results;
  Compiler c;
  DummyDB d;
  MetaData md;
  CompilerContext ctx{c, &d, &md};
};

void AstExprTest::expect(bool expected, Expression* expr) {
  node::Block blk({
                      new Let("x", new PrimaryExpression(MaybeValue(10))),
                      new Let("y", new PrimaryExpression(MaybeValue(20))),
                      new Let("z", new PrimaryExpression(MaybeValue(30))),
                      new If(expr, new Block({
                                                 new Emit(new RowLiteral({new PrimaryExpression(MaybeValue(1))}))
                                             }), {})
                  });
  compile_and_exec(&blk);
  EXPECT_EQ((expected ? 1 : 0), results.size());
}

TEST_F(AstExprTest, test_10_less_than_20) {
  // x = 10, test x < 20
  expect(true, new BinaryExpression(new VariableReference("x"),
                                    operators::LESSTHAN,
                                    new PrimaryExpression(MaybeValue(20))));
}

TEST_F(AstExprTest, test_x_less_than_y) {
  // x = 10, y = 20, test x < y
  expect(true, new BinaryExpression(new VariableReference("x"),
                                    operators::LESSTHAN,
                                    new VariableReference("y")));
}
TEST_F(AstExprTest, test_fail_x_more_than_x) {
  // x = 10, test x > x
  expect(false, new BinaryExpression(new VariableReference("x"),
                                     operators::MORETHAN,
                                     new VariableReference("x")));
}

TEST_F(AstExprTest, test_x_more_equal_than_x) {
  // x = 10, test x >= x
  expect(true, new BinaryExpression(new VariableReference("x"),
                                    operators::MOREEQUAL,
                                    new VariableReference("x")));
}

/////////////////////
// TEST for &&
/////////////////////
TEST_F(AstExprTest, test_T_AND_T) {
  // x = 10, y = 20, z = 30, test x < y AND y < z
  expect(true, new BinaryExpression(
      new BinaryExpression(new VariableReference("x"), operators::LESSTHAN, new VariableReference("y")),
      operators::CONDITIONAL_AND,
      new BinaryExpression(new VariableReference("y"), operators::LESSTHAN, new VariableReference("z"))));
}
TEST_F(AstExprTest, test_T_AND_F) {
  // x = 10, y = 20, z = 30, test x < y AND y > z
  expect(false, new BinaryExpression(
      new BinaryExpression(new VariableReference("x"), operators::LESSTHAN, new VariableReference("y")),
      operators::CONDITIONAL_AND,
      new BinaryExpression(new VariableReference("y"), operators::MORETHAN, new VariableReference("z"))));
}

TEST_F(AstExprTest, test_F_AND_T) {
  // x = 10, y = 20, z = 30, test x > y AND y < z
  expect(false, new BinaryExpression(
      new BinaryExpression(new VariableReference("x"), operators::MORETHAN, new VariableReference("y")),
      operators::CONDITIONAL_AND,
      new BinaryExpression(new VariableReference("y"), operators::LESSTHAN, new VariableReference("z"))));
}
TEST_F(AstExprTest, test_F_AND_F) {
  // x = 10, y = 20, z = 30, test x > y AND y > z
  expect(false, new BinaryExpression(
      new BinaryExpression(new VariableReference("x"), operators::MORETHAN, new VariableReference("y")),
      operators::CONDITIONAL_AND,
      new BinaryExpression(new VariableReference("y"), operators::MORETHAN, new VariableReference("z"))));
}

TEST_F(AstExprTest, test_non_bool_expression) {
  // 1 && 2
  expect(true, new BinaryExpression(
      new PrimaryExpression(MaybeValue(1)),
      operators::CONDITIONAL_AND,
      new PrimaryExpression(MaybeValue(2)))
  );
}

TEST_F(AstExprTest, test_non_bool_expression2) {
  // 1 && 0
  expect(false, new BinaryExpression(
      new PrimaryExpression(MaybeValue(1)),
      operators::CONDITIONAL_AND,
      new PrimaryExpression(MaybeValue(0)))
  );
}

/////////////////////
// TEST for ||
/////////////////////
TEST_F(AstExprTest, test_T_OR_T) {
  // x = 10, y = 20, z = 30, test x < y OR y < z
  expect(true, new BinaryExpression( new BinaryExpression(new VariableReference("x"), operators::LESSTHAN, new VariableReference("y")),
                                     operators::CONDITIONAL_OR,
                                     new BinaryExpression(new VariableReference("y"), operators::LESSTHAN, new VariableReference("z"))));
}
TEST_F(AstExprTest, test_T_OR_F) {
  // x = 10, y = 20, z = 30, test x < y OR y > z
  expect(true, new BinaryExpression(
      new BinaryExpression(new VariableReference("x"), operators::LESSTHAN, new VariableReference("y")),
      operators::CONDITIONAL_OR,
      new BinaryExpression(new VariableReference("y"), operators::MORETHAN, new VariableReference("z"))));
}

TEST_F(AstExprTest, test_F_OR_T) {
  // x = 10, y = 20, z = 30, test x > y OR y < z
  expect(true, new BinaryExpression(
      new BinaryExpression(new VariableReference("x"), operators::MORETHAN, new VariableReference("y")),
      operators::CONDITIONAL_OR,
      new BinaryExpression(new VariableReference("y"), operators::LESSTHAN, new VariableReference("z"))));
}
TEST_F(AstExprTest, test_F_OR_F) {
  // x = 10, y = 20, z = 30, test x > y OR y > z
  expect(false, new BinaryExpression(
      new BinaryExpression(new VariableReference("x"), operators::MORETHAN, new VariableReference("y")),
      operators::CONDITIONAL_OR,
      new BinaryExpression(new VariableReference("y"), operators::MORETHAN, new VariableReference("z"))));
}
TEST_F(AstExprTest, test_non_bool_expression3) {
  // 1 || 2
  expect(true, new BinaryExpression(
      new PrimaryExpression(MaybeValue(1)),
      operators::CONDITIONAL_OR,
      new PrimaryExpression(MaybeValue(2)))
  );
}
TEST_F(AstExprTest, test_non_bool_expression4) {
  // 0 || 1
  expect(true, new BinaryExpression(
      new PrimaryExpression(MaybeValue(0)),
      operators::CONDITIONAL_OR,
      new PrimaryExpression(MaybeValue(1)))
  );
}
/////////////////////
// TEST for nested
/////////////////////
TEST_F(AstExprTest, test_nested_condition1) {
  // x = 10, y = 20, z = 30, test (10 == x) && ((20 <= y) AND (z >= 30))
  expect(true, new BinaryExpression(
      new BinaryExpression(new PrimaryExpression(MaybeValue(10)), operators::EQUAL, new VariableReference("x")),
      operators::CONDITIONAL_AND,
      new BinaryExpression(
          new BinaryExpression(new PrimaryExpression(MaybeValue(20)), operators::LESSEQUAL, new VariableReference("y")),
          operators::CONDITIONAL_AND,
          new BinaryExpression(new VariableReference("z"), operators::MOREEQUAL, new PrimaryExpression(MaybeValue(20)))))
  );
}


TEST_F(AstExprTest, test_short_circuit) {
  node::Block blk({
                      new Let("x", new PrimaryExpression(MaybeValue(0))),
                      new Let("y", new PrimaryExpression(MaybeValue(0))),
                      new Let("z", new PrimaryExpression(MaybeValue(0))),
                      new Let("w", new PrimaryExpression(MaybeValue(0))),
                      new If( // 1 && x = 1
                          new BinaryExpression(
                              new PrimaryExpression(MaybeValue(1)),
                              operators::CONDITIONAL_AND,
                              new Assign(new VariableReference("x"), new PrimaryExpression(MaybeValue(1)))),
                          new Block({
                                        new Emit(new RowLiteral({new PrimaryExpression(MaybeValue(1))}))
                                    }), {}),
                      new If( // 0 && y = 1
                          new BinaryExpression(
                              new PrimaryExpression(MaybeValue(0)),
                              operators::CONDITIONAL_AND,
                              new Assign(new VariableReference("y"), new PrimaryExpression(MaybeValue(1)))),
                          new Block({
                                        // should not come here
                                        new Emit(new RowLiteral({new PrimaryExpression(MaybeValue(1))}))
                                    }), {}),
                      new If( // 1 || z = 1
                          new BinaryExpression(
                              new PrimaryExpression(MaybeValue(1)),
                              operators::CONDITIONAL_OR,
                              new Assign(new VariableReference("z"), new PrimaryExpression(MaybeValue(1)))),
                          new Block({
                                        new Emit(new RowLiteral({new PrimaryExpression(MaybeValue(1))}))
                                    }), {}),
                      new If( // 0 || w = 1
                          new BinaryExpression(
                              new PrimaryExpression(MaybeValue(0)),
                              operators::CONDITIONAL_OR,
                              new Assign(new VariableReference("w"), new PrimaryExpression(MaybeValue(1)))),
                          new Block({
                                        new Emit(new RowLiteral({new PrimaryExpression(MaybeValue(1))}))
                                    }), {}),
                      new If( // verify x == 1, y == 0, z == 0, w == 1
                          new BinaryExpression(
                              new BinaryExpression(
                                  new BinaryExpression(new VariableReference("x"),
                                                       operators::EQUAL,
                                                       new PrimaryExpression(MaybeValue(1))),
                                  operators::CONDITIONAL_AND,
                                  new BinaryExpression(new VariableReference("y"),
                                                       operators::EQUAL,
                                                       new PrimaryExpression(MaybeValue(0)))
                              ),
                              operators::CONDITIONAL_AND,
                              new BinaryExpression(
                                  new BinaryExpression(new VariableReference("z"),
                                                       operators::EQUAL,
                                                       new PrimaryExpression(MaybeValue(0))),
                                  operators::CONDITIONAL_AND,
                                  new BinaryExpression(new VariableReference("w"),
                                                       operators::EQUAL,
                                                       new PrimaryExpression(MaybeValue(1)))
                              )
                          ),
                          new Block({
                                        new ExprStatement(new FunctionCall("print_string", {new PrimaryExpression(
                                            MaybeValue("x,y,z,w are expected\n"))}))
                                    }),
                          new Block({  // something is wrong, print the integers to investigate
                                        new ExprStatement(new FunctionCall("print_string", {new PrimaryExpression(MaybeValue("debug print x,y,z,w\n"))})),
                                        new ExprStatement(new FunctionCall("print_int", { new VariableReference("x") })),
                                        new ExprStatement(new FunctionCall("print_int", { new VariableReference("y") })),
                                        new ExprStatement(new FunctionCall("print_int", { new VariableReference("z") })),
                                        new ExprStatement(new FunctionCall("print_int", { new VariableReference("w") })),
                                        // emit to make test case fail
                                        new Emit(new RowLiteral({new PrimaryExpression(MaybeValue(99999))})),
                                        new Emit(new RowLiteral({new PrimaryExpression(MaybeValue(99999))})),
                                        new Emit(new RowLiteral({new PrimaryExpression(MaybeValue(99999))})),
                                        new Emit(new RowLiteral({new PrimaryExpression(MaybeValue(99999))}))
                                    })
                      ),
                  });
  compile_and_exec(&blk);
  EXPECT_EQ(3, results.size());
}

}  // namespace reir