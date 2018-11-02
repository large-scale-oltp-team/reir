#include <iostream>
#include "schema.hpp"

namespace reir {

Schema::~Schema() {}

void Schema::serialize(std::string& buf) const {
  std::stringstream ss;
  ss << name_ << ":";
  for (const auto& a : attrs_) {
    a.serialize(ss);
  }
  buf = ss.str();
}

void Schema::deserialize(const std::string& buf) {
  attrs_.clear();
  std::stringstream ss;
  ss << buf;
  std::istreambuf_iterator<char> it(ss);
  std::stringstream name;
  while (*it != ':') name << *it++;
  name_ = name.str();
  ++it;
  for (;;) {
    std::istreambuf_iterator<char> i(ss), end;
    if (i == end) {
      break;
    }
    Attribute a;
    a.deserialize(ss);
    attrs_.emplace_back(std::move(a));
  }
}


}  // namespace reir
