//
// Created by kumagi on 18/04/15.
//

#include <iostream>
#include <boost/variant.hpp>
#include "util/slice.hpp"
#include "value.hpp"

namespace reir {

class ValueImpl {
  Value::ValType type_;
  boost::variant<int64_t, std::string, double> value_;
 public:

  explicit ValueImpl(util::slice s) : type_(Value::ValType::VARCHAR), value_(std::string(s)) {}
  explicit ValueImpl(int64_t v) : type_(Value::ValType::INT), value_(v) {}
  explicit ValueImpl(double v) : type_(Value::ValType::FLOAT), value_(v) {}
  ValueImpl() : type_(Value::ValType::UNKNWON) {};

  bool is_int() const {
    return type_ == Value::ValType::INT;
  }
  bool is_varchar() const {
    return type_ == Value::ValType::VARCHAR;
  }
  bool is_float() const {
    return type_ == Value::ValType::FLOAT;
  }
  int64_t as_int() const {
    assert(is_int());
    return boost::get<int64_t>(value_);
  }
  std::string as_varchar() const {
    assert(is_varchar());
    return boost::get<std::string>(value_);
  }
  double as_float() const {
    assert(is_float());
    return boost::get<double>(value_);
  }
  size_t serialized_length() const {
    switch (type_) {
    case Value::ValType::INT:
      return sizeof(int64_t);
    case Value::ValType::VARCHAR: {
      const std::string& s = boost::get<const std::string>(value_);
      return s.size();
    }
    case Value::ValType::FLOAT:
      return sizeof(double);
    default:
      throw std::runtime_error("unknown valtype");
    };
  }

  void serialize(char* buffer) const {
    switch (type_) {
    case Value::ValType::INT: {
      int64_t* b = reinterpret_cast<int64_t*>(buffer);
      *b = boost::get<int64_t>(value_);
      return;
    }
    case Value::ValType::VARCHAR: {
      std::string str = boost::get<std::string>(value_);
      size_t* len = reinterpret_cast<size_t*>(buffer);
      *len = str.size();
      char* payload = &buffer[sizeof(size_t)];
      std::memcpy(payload, str.data(), str.size());
      return;
    }
    case Value::ValType::FLOAT: {
      double* b = reinterpret_cast<double*>(buffer);
      *b = boost::get<double>(value_);
      return;
    }
    default:
      throw std::runtime_error("unknown valtype");
    };
  }

  void deserialize(const char* buffer, Value::ValType as) {
    type_ = as;
    switch (as) {
    case Value::ValType::INT: {
      const int64_t* v = reinterpret_cast<const int64_t*>(buffer);
      value_ = *v;
      return;
    }
    case Value::ValType::VARCHAR: {
      const size_t* len = reinterpret_cast<const size_t*>(buffer);
      std::string str;
      str.resize(*len);
      const char* payload = &buffer[sizeof(size_t)];
      std::memcpy(&str[0], payload, *len);
      value_ = str;
      return;
    }
    case Value::ValType::FLOAT:
      value_ = *reinterpret_cast<const double*>(buffer);
      return;
    default:
      throw std::runtime_error("unknown valtype");
    }
  }

  ValueImpl& operator=(const Value& rhs) {
    type_ = rhs.pimpl_->type_;
    value_ = rhs.pimpl_->value_;
    return *this;
  }
  ValueImpl& operator=(const int64_t& rhs) {
    type_ = Value::ValType::INT;
    value_ = rhs;
    return *this;
  }
  ValueImpl& operator=(const std::string& rhs) {
    type_ = Value::ValType::VARCHAR;
    value_ = rhs;
    return *this;
  }
  bool operator==(const Value& rhs) const {
    return type_ == rhs.pimpl_->type_ && value_ == rhs.pimpl_->value_;
  }

  void dump(std::ostream& o) {
    if (value_.which() == 0) {  // int64_t
      o << value_;
    } else if (value_.which() == 1) {  // std::string
      o << "\"" << value_ << "\"";
    } else if (value_.which() == 2) {  // double
      o << value_;
    } else {
      throw std::runtime_error("unknown type assigned in MaybeValue");
    }
  }
};

Value::Value(const char* v) : pimpl_(new ValueImpl(v)) {}
Value::Value(util::slice s) : pimpl_(new ValueImpl(s)) {}
Value::Value(int64_t v) : pimpl_(new ValueImpl(v)) {}
Value::Value(double v) : pimpl_(new ValueImpl(v)) {};
Value::Value() : pimpl_(new ValueImpl()) {}

bool Value::is_int() const {
  return pimpl_->is_int();
}
bool Value::is_varchar() const {
  return pimpl_->is_varchar();
}
bool Value::is_float() const {
  return pimpl_->is_float();
}
int64_t Value::as_int() const {
  return pimpl_->as_int();
}
std::string Value::as_varchar() const {
  return pimpl_->as_varchar();
}
double Value::as_float() const {
  return pimpl_->as_float();
}
size_t Value::serialized_length() const {
  return pimpl_->serialized_length();
}

void Value::serialize(char* buffer) const {
  return pimpl_->serialize(buffer);
}
void Value::deserialize(const char* buffer, ValType as) {
  pimpl_->deserialize(buffer, as);
}

Value& Value::operator=(const Value& rhs) {
  pimpl_->operator=(rhs);
  return *this;
}
Value& Value::operator=(const int64_t& rhs) {
  pimpl_->operator=(rhs);
  return *this;
}
Value& Value::operator=(const std::string& rhs) {
  pimpl_->operator=(rhs);
  return *this;
}
bool Value::operator==(const Value& rhs) const {
  return pimpl_->operator==(rhs);
}

void Value::dump(std::ostream& o) const {
  pimpl_->dump(o);
}

}  // namespace reir
