#include "attribute.hpp"
#include "reir/db/value.hpp"


namespace reir {

size_t Attribute::encoded_length(const MaybeValue& tuple) const {
  if (property_ == AttrProperty::NULLABLE) {
    if (!tuple.exists()) {
      return 1;
    } else {
      const reir::Value& v = tuple.value();
      return 1 + type_.encoded_length(v);
    }
  } else {
    if (!tuple.exists()) {
      throw std::runtime_error("NON-Nullable cell was null");
    } else {
      const reir::Value& v = tuple.value();
      return type_.encoded_length(v);
    }
  }
}

void Attribute::encode(const MaybeValue& tuple, char* buffer) const {
  if (property_ == AttrProperty::NULLABLE) {
    if (tuple.exists()) {
      buffer[0] = 1;
      const Value& v = tuple.value();
      type_.encode(v, &buffer[1]);
    }
  } else {
    if (tuple.exists()) {
      const Value& v = tuple.value();
      type_.encode(v, buffer);
    } else {
      throw std::runtime_error("tuple value must be set");
    }
  }
}

void Attribute::decode(const char* buffer, MaybeValue& tuple) const {
  if (property_ == AttrProperty::NULLABLE) {
    if (buffer[0] == 0) {
      tuple.make_null();
    } else {
      Value v;
      type_.decode(&buffer[1], v);
      tuple = v;
    }
  } else {
    Value v;
    type_.decode(buffer, v);
    tuple = v;
  }
}

void Attribute::serialize(std::ostream& s) const {
  s << "(" << name_ << " ";
  type_.serialize(s);
  s << " " << property_ << ")";
}

void Attribute::deserialize(std::istream& s) {
  std::istreambuf_iterator<char> it(s);
  while (*it != '(') {
    if (s.eof()) { return; }
    ++it;
  }
  ++it;
  std::stringstream n;
  while (*it != ' ') n << *it++;
  name_ = n.str();
  ++it;
  type_.deserialize(s);
  std::stringstream p;
  while (*it != ')') p << *it++;
  ++it;
  property_ = static_cast<AttrProperty>(std::stoll(p.str()));
}

Attribute::~Attribute() {}

}  // namespace reir
