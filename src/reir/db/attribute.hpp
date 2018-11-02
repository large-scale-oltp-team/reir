#ifndef REIR_ATTRIBUTE_HPP_
#define REIR_ATTRIBUTE_HPP_

#include <vector>
#include "maybe_value.hpp"
#include "attr_type.hpp"

namespace reir {

struct Attribute {
 public:
  enum AttrProperty : uint32_t {
    NONE     = 0x00000000,
    NULLABLE = 0x00000001,
    KEY      = 0x00000002,
    UNIQUE   = 0x00000004,
  };
  Attribute() : type_("unknown"), property_(NONE) {}
  Attribute(std::string name, AttrType type, AttrProperty prop = NONE)
      : name_(std::move(name)), type_(std::move(type)), property_(prop) {}
  Attribute(const Attribute&) = default;
  ~Attribute();
  bool fixed_length() const {
    std::cerr << "name:" << name_ << " " << type_ << " pa" << type_ << "\n";
    return type_.fixed_length();
  }
  size_t encoded_length(const MaybeValue& tuple) const;

  bool is_key() const {
    return property_ & AttrProperty::KEY;
  }
  const AttrType& type() const {
    return type_;
  }

  size_t default_size() const {
    if (property_ & NULLABLE) {
      return type_.default_size() + 1;
    } else {
      return type_.default_size();
    }
  }

  void encode(const MaybeValue& tuple, char* buffer) const;
  void decode(const char* buffer, MaybeValue& tuple) const;
  void serialize(std::ostream& s) const;
  void deserialize(std::istream& s);
  bool operator==(const Attribute& rhs) const {
    return name_ == rhs.name_ && type_ == rhs.type_ && property_ == rhs.property_;
  }
  bool operator!=(const Attribute& rhs) const {
    return !this->operator==(rhs);
  }

  friend std::ostream& operator<<(std::ostream& o, const Attribute& s) {
    o << s.type_ << s.name_;
    if (s.property_ & AttrProperty::NONE) {
      o << " NONE";
    }
    if (s.property_ & AttrProperty::NULLABLE) {
      o << " NULLABLE";
    }
    if (s.property_ & AttrProperty::KEY) {
      o << " KEY";
    }
    if (s.property_ & AttrProperty::UNIQUE) {
      o << " UNIQUE";
    }
    return o;
  }

  std::string name_;
  AttrType type_;
  AttrProperty property_;
};

}  // namespace reir

#endif  // REIR_ATTRIBUTE_HPP_
