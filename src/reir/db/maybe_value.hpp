#ifndef REIR_MAYBE_VALUE_HPP_
#define REIR_MAYBE_VALUE_HPP_

#include <utility>
#include <assert.h>
#include "value.hpp"

namespace reir {

class MaybeValue {
 public:
  MaybeValue(): exists_(false) {}

  explicit MaybeValue(util::slice s) : exists_(true), value_(s) {}
  explicit MaybeValue(int64_t v) : exists_(true), value_(v) {}
  // explicit MaybeValue(double v) : exists_(true), value_(v) {}

  bool fixed_length() {
    return value_.fixed_length();
  }
  bool exists() const {
    return exists_;
  }
  Value& value() {
    assert(exists_);
    return value_;
  }
  const Value& value() const {
    assert(exists_);
    return value_;
  }
  size_t serialized_length() const {
    if (exists_) {
      return 1 + value_.serialized_length();
    } else {
      return 1;
    }
  }
  void make_null() {
    exists_ = false;
    value_ = reir::Value();
  }
  void serialize(char* buffer) const {
    if (exists_) {
      buffer[0] = 1;
      value_.serialize(&buffer[1]);
    } else {
      buffer[0] = 0;
    }
  }
  void deserialize(const char* buffer, Value::ValType as) {
    if (buffer[0] == 1) {
      exists_ = true;
      value_.deserialize(&buffer[1], as);
    } else {
      exists_ = false;
    }
  }
  MaybeValue& operator=(const Value& rhs) {
    exists_ = true;
    value_ = rhs;
    return *this;
  }
  bool operator==(const MaybeValue& rhs) const {
    if (exists_ != rhs.exists_) { return false; }
    if (!exists_) { return true; }
    assert(exists_);
    return value_ == rhs.value_;
  }
  bool operator!=(const MaybeValue& rhs) const {
    return !this->operator==(rhs);
  }
  friend std::ostream& operator<<(std::ostream& o, const MaybeValue& v) {
    if (!v.exists_) {
      o << "(null)";
    } else {
      o << v.value_;
    }
    return o;
  }

 private:
  bool exists_;
  reir::Value value_;
};

}  // namespace reir

#endif  // REIR_MAYBE_VALUE_HPP_
