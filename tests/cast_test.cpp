#include <string>

#include <gtest/gtest.h>

#include "reir/exec/compiler.hpp"
#include "reir/exec/ast_statement.hpp"

#include "reir/exec/parser.hpp"
#include "reir/exec/executor.hpp"
#include "reir/exec/llvm_environment.hpp"

namespace reir {
namespace node {

class AstCastTest : public testing::Test {
protected:
  virtual void SetUp() override {
  }
};

TEST_F(AstCastTest, basicCast) {
  // down cast Expression -> PrimaryExpression
  const node::PrimaryExpression primary{MaybeValue(0)};
  const Expression* expr = &primary;
  ASSERT_TRUE(llvm::isa<PrimaryExpression>(*expr));

  ASSERT_NO_THROW(
      const PrimaryExpression& casted = llvm::cast<PrimaryExpression>(*expr);
      std::cout << casted << std::endl;
  );
  const PrimaryExpression* castedp = llvm::dyn_cast<PrimaryExpression>(expr);
  ASSERT_TRUE(castedp);
  const BinaryExpression* wrongly_castedp = llvm::dyn_cast<BinaryExpression>(expr);
  ASSERT_FALSE(wrongly_castedp);
  EXPECT_DEATH({
                 llvm::cast<BinaryExpression>(*expr);
               }, "");

  // down cast Node -> PrimaryExpression
  const Node* node = &primary;
  ASSERT_NO_THROW(
      const PrimaryExpression& casted2 = llvm::cast<PrimaryExpression>(*node);
      std::cout << casted2 << std::endl;
  );
  ASSERT_TRUE(llvm::isa<PrimaryExpression>(*node));
  const PrimaryExpression* castedp2 = llvm::dyn_cast<PrimaryExpression>(node);
  ASSERT_TRUE(castedp2);
  const BinaryExpression* wrongly_castedp2 = llvm::dyn_cast<BinaryExpression>(node);
  ASSERT_FALSE(wrongly_castedp2);
  EXPECT_DEATH({
                 llvm::cast<BinaryExpression>(*node);
               }, "");
}

TEST_F(AstCastTest, crossExpressionAndStatement) {
  const node::PrimaryExpression primary{MaybeValue(0)};
  const Node* node = &primary;
  ASSERT_TRUE(llvm::isa<Expression>(*node));
  ASSERT_FALSE(llvm::isa<Statement>(*node));
  const Expression* castedp = llvm::dyn_cast<Expression>(node);
  ASSERT_TRUE(castedp);
  const Statement* wrongly_castedp = llvm::dyn_cast<Statement>(node);
  ASSERT_FALSE(wrongly_castedp);
}
// varieties of expressions

TEST_F(AstCastTest, primaryExpression) {
  node::PrimaryExpression expr{MaybeValue(0)};
  Expression* ptr = &expr;
  ASSERT_TRUE(llvm::isa<PrimaryExpression>(*ptr));
  auto* casted = llvm::dyn_cast<PrimaryExpression>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, array) {
  node::ArrayLiteral expr{};
  Expression* ptr = &expr;
  ASSERT_TRUE(llvm::isa<ArrayLiteral>(*ptr));
  auto* casted = llvm::dyn_cast<ArrayLiteral>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, arrayReference) {
  node::ArrayReference expr{new ArrayLiteral{new PrimaryExpression(MaybeValue(100))},
                            new PrimaryExpression(MaybeValue(0))};
  Expression* ptr = &expr;
  ASSERT_TRUE(llvm::isa<ArrayReference>(*ptr));
  auto* casted = llvm::dyn_cast<ArrayReference>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, assign) {
  node::Assign expr{new VariableReference("x"), new PrimaryExpression(MaybeValue(0))};
  Expression* ptr = &expr;
  ASSERT_TRUE(llvm::isa<Assign>(*ptr));
  auto* casted = llvm::dyn_cast<Assign>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, binExpr) {
  node::BinaryExpression expr{new VariableReference("x"),
                              operators::PLUS,
                              new PrimaryExpression(MaybeValue(1))};
  Expression* ptr = &expr;
  ASSERT_TRUE(llvm::isa<BinaryExpression>(*ptr));
  auto* casted = llvm::dyn_cast<BinaryExpression>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, funcCall) {
  std::string name("print_int");
  node::FunctionCall expr{name, {new PrimaryExpression(MaybeValue(1))}};
  Expression* ptr = &expr;
  ASSERT_TRUE(llvm::isa<FunctionCall>(*ptr));
  auto* casted = llvm::dyn_cast<FunctionCall>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, memberRef) {
  std::string name("c1");
  node::MemberReference expr{new VariableReference("x1"), name};
  Expression* ptr = &expr;
  ASSERT_TRUE(llvm::isa<MemberReference>(*ptr));
  auto* casted = llvm::dyn_cast<MemberReference>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, pointer) {
  std::string name("c1");
  node::PointerOf expr{new VariableReference("x1")};
  Expression* ptr = &expr;
  ASSERT_TRUE(llvm::isa<PointerOf>(*ptr));
  auto* casted = llvm::dyn_cast<PointerOf>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, row) {
  std::string name("c1");
  node::RowLiteral expr{new PrimaryExpression(MaybeValue(10))};
  Expression* ptr = &expr;
  ASSERT_TRUE(llvm::isa<RowLiteral>(*ptr));
  auto* casted = llvm::dyn_cast<RowLiteral>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, varRef) {
  node::VariableReference expr{"varName"};
  Expression* ptr = &expr;
  ASSERT_TRUE(llvm::isa<VariableReference>(*ptr));
  auto* casted = llvm::dyn_cast<VariableReference>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

// varieties of statements

TEST_F(AstCastTest, block) {
  node::Block stmt{};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<Block>(*ptr));
  auto* casted = llvm::dyn_cast<Block>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, define) {
  std::unique_ptr<TupleType> t(new TupleType({}, {}));
  node::Define stmt{t.get(), ""};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<Define>(*ptr));
  auto* casted = llvm::dyn_cast<Define>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, emit) {
  node::Emit stmt{new RowLiteral({new PrimaryExpression(MaybeValue(10))})};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<Emit>(*ptr));
  auto* casted = llvm::dyn_cast<Emit>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, exprStatement) {
  node::ExprStatement stmt{new PrimaryExpression(MaybeValue(0))};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<ExprStatement>(*ptr));
  auto* casted = llvm::dyn_cast<ExprStatement>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, forStatement) {
  node::For stmt{nullptr, nullptr, nullptr, new Block()};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<For>(*ptr));
  auto* casted = llvm::dyn_cast<For>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, ifStatement) {
  node::If stmt{nullptr, nullptr, new Block()};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<If>(*ptr));
  auto* casted = llvm::dyn_cast<If>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, jump) {
  node::Jump stmt{Jump::continue_jump};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<Jump>(*ptr));
  auto* casted = llvm::dyn_cast<Jump>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, let) {
  node::Let stmt{"x", new PrimaryExpression(MaybeValue(0))};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<Let>(*ptr));
  auto* casted = llvm::dyn_cast<Let>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, transaction) {
  node::Transaction stmt{"isolation", {}};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<Transaction>(*ptr));
  auto* casted = llvm::dyn_cast<Transaction>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, insert) {
  node::Insert stmt{"table_name", new RowLiteral({new PrimaryExpression(MaybeValue(100))})};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<Insert>(*ptr));
  auto* casted = llvm::dyn_cast<Insert>(ptr);
  ASSERT_TRUE(casted != nullptr);
}

TEST_F(AstCastTest, scan) {
  node::Scan stmt{"table_name", "row_name", {}};
  Statement* ptr = &stmt;
  ASSERT_TRUE(llvm::isa<Scan>(*ptr));
  auto* casted = llvm::dyn_cast<Scan>(ptr);
  ASSERT_TRUE(casted != nullptr);
}
}  // namespace node
}  // namespace reir