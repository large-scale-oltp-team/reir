#ifndef REIR_DB_VALUE_HPP_
#define REIR_DB_VALUE_HPP_
#include <assert.h>
#include <string>
#include <cstring>
#include <memory>
#include "util/slice.hpp"

namespace leveldb {
class DB;
}

namespace reir {
class Schema;
class ValueImpl;

class Value {
 public:
  enum ValType {
    INT,
    VARCHAR,
    FLOAT,
    UNKNWON,
  };
  /*
  template<typename T>
  Expression(T&& v) : type_(ValType::UNKNWON) {
    throw std::runtime_error("unknown ValType passed");
  }
  */
  explicit Value(const char* v);
  explicit Value(util::slice s);
  explicit Value(int64_t v);
  explicit Value(double v);
  Value();

  bool fixed_length() const {
    return true;
  }
  bool is_int() const;
  bool is_varchar() const;
  bool is_float() const;
  int64_t as_int() const;
  std::string as_varchar() const;
  double as_float() const;
  size_t serialized_length() const;

  void serialize(char* buffer) const;
  void deserialize(const char* buffer, ValType as);

  Value& operator=(const Value& rhs);
  Value& operator=(const int64_t& rhs);
  Value& operator=(const std::string& rhs);

  bool operator==(const Value& rhs) const;
  bool operator!=(const Value& rhs) const {
    return !this->operator==(rhs);
  }
  void dump(std::ostream& o) const;

  friend std::ostream& operator<<(std::ostream& o, const Value& v) {
    v.dump(o);
    return o;
  }

 private:
  std::shared_ptr<ValueImpl> pimpl_;
  friend class ValueImpl;
};

}  // namespace reir

#endif  // REIR_DB_VALUE_HPP_
