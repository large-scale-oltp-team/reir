#include <stdexcept>
#include "attr_type.hpp"
#include "maybe_value.hpp"

#include <iostream>

namespace reir {

AttrType::AttrType(const std::string& t) {
  if (t == "int") {
    type_ = Type::INTEGER;
    param_ = 0;
  } else if (t == "string") {
    type_ = Type::STRING;
    param_ = 16;
  } else {
    type_ = Type::UNKNOWN;
    param_ = 0;
  }
}

AttrType::~AttrType() {}

size_t AttrType::encoded_length(const Value&tuple) const {
  if (type_ == Type::INTEGER) {
    return sizeof(int64_t);
  } else if (type_ == Type::STRING) {
    if (tuple.is_varchar()) {
      std::string str(tuple.as_varchar());  // FIXME: avoid deep copy
      return str.length();
    } else {
      throw std::runtime_error("tuple is not string type");
    }
  } else {
    throw std::runtime_error("UNKNOWN Type cant be encoded");
  }
}

void AttrType::encode(const Value& value, char* buffer) const {
  switch (type_) {
  case Type::INTEGER: {
    if (!value.is_int()) {
      throw std::runtime_error("tuple type unmatch, int is expected");
    }
    int64_t data = value.as_int();
    std::memcpy(buffer, &data, sizeof(int64_t));
    break;
  }
  case Type::STRING: {
    if (!value.is_varchar()) {
      throw std::runtime_error("tuple type unmatch, varchar is expected");
    }
    const std::string& data = value.as_varchar();
    size_t& len = *reinterpret_cast<size_t*>(buffer);
    len = data.length();
    std::memcpy(&buffer[sizeof(size_t)], &data[0], data.length());
    break;
  }
  default: {
    throw std::runtime_error("encoding unimplementd yet");
  }
  }
}
void AttrType::decode(const char* buffer, Value& tuple) const {
  switch(type_) {
  case Type::INTEGER: {
    const int64_t* data = reinterpret_cast<const int64_t*>(buffer);
    tuple = *data;
    break;
  }
  case Type::STRING: {
    size_t len = *reinterpret_cast<const size_t*>(buffer);
    std::string str;
    str.resize(len);
    std::memcpy(&str[0], &buffer[sizeof(size_t)], len);
    tuple = str;
    break;
  }
  default: {
    throw std::runtime_error("decoding unimplementd yet");
  }
  }
}

void AttrType::serialize(std::ostream& s) const {
  s << "(" << type_ << " " << param_ << ")";
}

void AttrType::deserialize(std::istream& s) {
  std::istreambuf_iterator<char> it(s);
  while (*it != '(') ++it;
  ++it;
  std::stringstream t;
  while (*it != ' ') t << *it++;
  type_ = static_cast<Type>(std::stoll(t.str()));
  ++it;
  std::stringstream p;
  while (*it != ')') p << *it++;
  param_ = static_cast<size_t>(std::stoll(p.str()));
  ++it;
}

}  // namespace reir

