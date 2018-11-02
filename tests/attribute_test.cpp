#include <iostream>
#include <sstream>
#include <memory>
#include <gtest/gtest.h>
#include "reir/exec/ast_node.hpp"  // Column
#include "reir/db/attribute.hpp"

namespace reir {

TEST(attribute, construct) {
  Attribute a;
  a.name_ = "foo";
  a.type_ = AttrType("int");
  std::cout << a << std::endl;
}

TEST(attribute, codec_int) {
  Attribute a;
  a.name_ = "foo";
  a.type_ = AttrType("int");
  MaybeValue v1(static_cast<int64_t>(3));
  std::unique_ptr<char[]> buff(new char[10]);
  a.encode(v1, buff.get());
  MaybeValue v2;
  a.decode(buff.get(), v2);
  ASSERT_EQ(v1, v2);
}

}  // namespace reir
