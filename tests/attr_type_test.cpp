#include <iostream>
#include <sstream>
#include <gtest/gtest.h>
#include "reir/db/attr_type.hpp"

namespace reir {

TEST(attr_type, construct) {
  AttrType a("int");
}

TEST(attr_type, serialize) {
  AttrType a("int");
  a.serialize(std::cout);
  std::cout << a;
}

TEST(attr_type, serdes) {
  AttrType a("int");
  std::stringstream ss;
  a.serialize(ss);
  AttrType b("ddd");
  ASSERT_NE(a, b);
  b.deserialize(ss);
  ASSERT_EQ(a, b);
  std::cout << b;
}


}  // namespace reir
