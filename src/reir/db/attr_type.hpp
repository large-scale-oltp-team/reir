#ifndef REIR_DB_ATTRTYPE_HPP_
#define REIR_DB_ATTRTYPE_HPP_
#include <string>
#include <ostream>
#include <sstream>
#include <iostream>

namespace leveldb {
class DB;
}

namespace reir {
class Schema;
class MaybeValue;
class Value;

class AttrType {
 public:
  enum Type : size_t {
    INTEGER,
    DOUBLE,
    STRING,
    DATE,
    UNKNOWN,
  };
  explicit AttrType(const std::string& t);
  explicit AttrType(Type t, size_t param = 0) : type_(t), param_(param) {}
  ~AttrType();
  bool fixed_length() const {
    return type_ != Type::STRING || param_ != 0;
  }
  bool is_integer() const {
    return type_ == INTEGER;
  }

  size_t default_size() const {
    if (type_ == INTEGER) {
      return 8;
    } else if (type_ == STRING) {
      return param_;
    } else if (type_ == DATE) {
      return 8;
    } else if (type_ == DOUBLE) {
      return 8;
    } else {
      throw std::runtime_error("unknown type:" + std::to_string(type_));
    }
  }


  size_t encoded_length(const Value& tuple) const;
  void encode(const Value& tuple, char* buffer) const;
  void decode(const char* buffer, Value& tuple) const;

  void serialize(std::ostream& s) const;
  void deserialize(std::istream& s);

  friend std::ostream& operator<<(std::ostream& o, const AttrType& a) {
    if (a.type_ == Type::INTEGER) {
      o << "(int)";
    } else if (a.type_ == Type::STRING) {
      o << "(str(" << a.param_ << "))";
    } else {
      o << "(unknown)";
    }
    return o;
  }

  bool operator==(const AttrType& rhs) const {
    return type_ == rhs.type_ && param_ == rhs.param_;
  }
  bool operator!=(const AttrType& rhs) const {
    return !this->operator==(rhs);
  }
 private:
  AttrType::Type type_;
  size_t param_;
};


}  // namespace reir

#endif  // REIR_DB_ATTRTYPE_HPP_
