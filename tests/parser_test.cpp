//
// Created by kumagi on 18/04/12.
//
#include <functional>
#include "gtest/gtest.h"
#include "reir/exec/parser.hpp"

namespace reir {

void try_code(const std::string& c) {
  std::cout << "---------------------------" << std::endl;
  SCOPED_TRACE(c);
  std::cout << c << std::endl;
  parse(c, [](node::Node* n){
    n->dump(std::cout, 0);
  });
  std::cout << "\n";
}

TEST(parser, empty) {
  try_code("");
  try_code("{}");
}

TEST(parser, table) {
  try_code("define<{int:hoge, int:fuga}> table_name");
  try_code("drop_table(table_name)");
}

TEST(parser, insert_simple) {
  try_code("insert table_name {foo, bar, sdfadsfads, dfdd}");
  try_code("insert table_name {1, 2, 3, 4}");
}

// tuples in array not supported yet
TEST(parser, DISABLED_insert) {
  try_code("insert table_name [{foo, bar, sdfadsfads, dfdd}, {ab,c,d,10}]");
  try_code("insert table_name [{1, 2, 3, 4}, {5,6,7,8}]");
}

TEST(parser, transaction) {
  try_code("transaction{}");
  try_code("transaction{"
            " insert hoge {fo, 1}"
           "}");
}

TEST(parser, if_stmt) {
  try_code("if 1 {"
           "  insert tbl {ho, 1}"
           "}");
}

TEST(parser, if_stmt_multi_condition) {
  try_code("if (1 < 2) || (3 <= 4) && (5 > 6) {"
           "  insert tbl {ho, 1}"
           "}");
}

TEST(parser, expr) {
  try_code("1+2");
  try_code("1+2*3/4-5");
  try_code("1*(2+3)/4*(5-6-7)-8");
  try_code("x + y - 4*(3-y)");
  try_code("y.hoge");
}

TEST(parser, let_assign) {
  try_code("let x = 10");
  try_code("let x = 10"
           " print_int(x)");
  try_code("y = 20");
  try_code("y = {20, 2, hoge}");
}

TEST(parser, empty_for_loop) {
  try_code("for let x=10; x = 1; x = x + 1 {}");
}

TEST(parser, for_loop) {
  try_code("for let x=10; x = 1; x = x + 1 {"
           " let y = 10"
           " print_str(&y)"
           "}");
}

}  // namespace reir

