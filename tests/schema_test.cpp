#include <iostream>
#include <sstream>
#include <memory>
#include <gtest/gtest.h>
#include "reir/exec/ast_node.hpp"  // Column
#include "reir/db/schema.hpp"

namespace reir {

TEST(schema, construct) {
  Schema a("s", {
      Attribute("foo", AttrType("int"), Attribute::AttrProperty::KEY),
      Attribute("bar", AttrType("double"), Attribute::AttrProperty::NONE)
  });
  std::cout << a << std::endl;
}

TEST(schema, neq) {
  Schema a("a", {
      Attribute("foo", AttrType("int"), Attribute::AttrProperty::KEY),
      Attribute("bar", AttrType("double"), Attribute::AttrProperty::NONE)
  });
  Schema b("s", {
      Attribute("foe", AttrType("int"), Attribute::AttrProperty::KEY),
      Attribute("bar", AttrType("double"), Attribute::AttrProperty::NONE)
  });

  ASSERT_NE(a, b);
  std::cout << a << std::endl;
}

TEST(schema, serdes) {
  Schema a("a", {
      Attribute("foo", AttrType("int"), Attribute::AttrProperty::KEY),
  });
  std::string buf;
  a.serialize(buf);
  Schema b("a", {
      Attribute("foo", AttrType("int"), Attribute::AttrProperty::KEY),
  });
  b.deserialize(buf);

  ASSERT_EQ(a, b);
  std::cout << a << std::endl;
}


}  // namespace reir
