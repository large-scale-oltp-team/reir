#ifndef REIR_SCHEMA_HPP_
#define REIR_SCHEMA_HPP_

#include <utility>
#include <vector>
#include <string>
#include <stdexcept>
#include <functional>

#include "attribute.hpp"
#include "maybe_value.hpp"

namespace reir {

class Schema {
public:
  Schema() {}

  Schema(std::string name, std::vector<Attribute> attrs)
      : name_(std::move(name)), attrs_(std::move(attrs)) {}
  Schema(const Schema& o) = default;

  ~Schema();

  size_t get_tuple_length(int idx) const {
    return attrs_[idx].default_size();
  }

  bool fixed_key_length() const {
    for (const auto& attr : attrs_) {
      if (attr.is_key() && !attr.fixed_length()) {
        return false;
      }
    }
    return true;
  }

  size_t get_fixed_key_length() const {
    if (!fixed_key_length()) {
      throw std::runtime_error("you cannot get fixed length of variable length key");
    }
    size_t ret = name_.size() + 1;
    for (const auto& attr : attrs_) {
      if (attr.is_key()) {
        ret += attr.default_size();
      }
    }
    return ret + 1;
  }

  size_t get_fixed_value_length() const {
    if (!fixed_value_length()) {
      throw std::runtime_error("you cannot get fixed length of variable length key");
    }
    size_t ret = 0;
    for (const auto& attr : attrs_) {
      if (!attr.is_key()) {
        ret += attr.default_size();
      }
    }
    return ret;
  }

  std::string get_key_prefix() const {
    std::string key;
    key.resize(name_.length() + 1);
    std::memcpy(&key[0], name_.data(), name_.length());
    key[name_.length()] = ':';
    return key;
  }

  bool is_key(int idx) const {
    return attrs_[idx].is_key();
  }

  void add_column(const Attribute& attr) {
    attrs_.emplace_back(attr);
  }

  bool fixed_value_length() const {
    for (const auto& attr : attrs_) {
      if (!attr.is_key()) {
        if (!attr.fixed_length()) {
          return false;
        }
      }
    }
    return true;
  }

  size_t encoded_length(const std::vector<MaybeValue>& tuple) const {
    check_tuple_size(tuple);
    size_t sum = name_.size() + 1;
    for (size_t i = 0; i < attrs_.size(); ++i) {
      sum += attrs_[i].encoded_length(tuple[i]);
    }
    return sum;
  }

  void encode(const std::vector<MaybeValue>& tuple, char* buffer) const {
    check_tuple_size(tuple);
    std::memcpy(buffer, name_.data(), name_.length());
    buffer[name_.length()] = ':';
    size_t offset = name_.length() + 1;
    for (size_t i = 0; i < attrs_.size(); ++i) {
      attrs_[i].encode(tuple[i], &buffer[offset]);
      offset += attrs_[i].encoded_length(tuple[i]);
    }
  }
  void decode(const char* buffer, std::vector<MaybeValue>& tuple) const {
    check_tuple_size(tuple);
    size_t offset = 0;
    for (size_t i = 0; i < attrs_.size(); ++i) {
      attrs_[i].decode(&buffer[offset], tuple[i]);
      offset += attrs_[i].encoded_length(tuple[i]);
    }
  }

  void serialize(std::string& buf) const;
  void deserialize(const std::string& buf);

  size_t key_length(const std::vector<MaybeValue>& tuple) const {
    std::vector<size_t> keys;
      for (size_t i = 0; i < attrs_.size(); ++i) {
      if (attrs_[i].is_key()) {
        keys.emplace_back(i);
      }
    }
    size_t len = name_.size() + 1;
    for (auto&& k : keys) {
      len += attrs_[k].encoded_length(tuple[k]);
    }
    return len;
  }

  size_t value_length(const std::vector<MaybeValue>& tuple) const {
    std::vector<size_t> keys;
    for (size_t i = 0; i < attrs_.size(); ++i) {
      if (!attrs_[i].is_key()) {
        keys.emplace_back(i);
      }
    }
    size_t len = 0;
    for (auto&& k : keys) {
      len += attrs_[k].encoded_length(tuple[k]);
    }
    return len;
  }

  std::string get_prefix() const {
    return name_ + ":";
  }

  void encode_key(const std::vector<MaybeValue>& tuple, char* buff) const {
    std::memcpy(buff, name_.data(), name_.length());
    buff[name_.length()] = ':';
    size_t offset = name_.length() + 1;
    for (size_t i = 0; i < attrs_.size(); ++i) {
      if (attrs_[i].is_key()) {
        const size_t advance = attrs_[i].encoded_length(tuple[i]);
        attrs_[i].encode(tuple[i], &buff[offset]);
        offset += advance;
      }
    }
  }

  void encode_value(const std::vector<MaybeValue>& tuple, char* buff) const {
    size_t idx = 0;
    for (size_t i = 0; i < attrs_.size(); ++i) {
      if (!attrs_[i].is_key()) {
        const size_t offset = attrs_[i].encoded_length(tuple[i]);
        attrs_[i].encode(tuple[i], &buff[idx]);
        idx += offset;
      }
    }
  }

  size_t val_length(const std::vector<MaybeValue>& tuple) const {
    std::vector<size_t> vals;
    for (size_t i = 0; i < attrs_.size(); ++i) {
      if (!attrs_[i].is_key()) {
        vals.emplace_back(i);
      }
    }
    size_t len = 0;
    for (auto&& v : vals) {
      len += attrs_[v].encoded_length(tuple[v]);
    }
    return len;
  }

  void check_tuple_size(const std::vector<MaybeValue>& tuple) const {
    if (tuple.size() != attrs_.size()) {
      throw std::runtime_error("size does not match");
    }
  }

  size_t columns() const {
    return attrs_.size();
  }

  friend std::ostream& operator<<(std::ostream& o, const Schema& s) {
    o << "(" << s.name_ << " ";
    for (size_t i = 0; i < s.attrs_.size(); ++i) {
      if (0 < i) {
        o << ", ";
      }
      o << s.attrs_[i];
    }
    o << ")";
    return o;
  }

  void each_attr(std::function<void(size_t,const Attribute&)> f) const {
    for (size_t i = 0; i < attrs_.size(); ++i) {
      f(i, attrs_[i]);
    }
  }

  bool operator==(const Schema& rhs) const {
    return name_ == rhs.name_ && attrs_ == rhs.attrs_;
  }
  bool operator!=(const Schema& rhs) const {
    return !this->operator==(rhs);
  }

 private:
  std::string name_;
  std::vector<Attribute> attrs_;
  std::vector<size_t> keys_;
};

}  // namespace reir

#endif  // REIR_SCHEMA_HPP_
