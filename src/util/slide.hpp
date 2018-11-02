#ifndef REIR_SLICE_HPP_
#define REIR_SLICE_HPP_

#include <assert.h>

#include <string>
#include <vector>
#include <map>
#include <memory.h>
#include <ostream>

#include "third_party/MurmurHash2_64.hpp"

namespace util {

class slice {
 public:
  slice() : ptr_(NULL), len_(0) {}
  slice(const char* d, size_t n) : ptr_(d), len_(n) { }
  slice(std::string& s) : ptr_(s.data()), len_(s.size()) { }
  slice(const std::string& s) : ptr_(s.data()), len_(s.size()) { }
  slice(std::vector<char>& s) : ptr_(s.data()), len_(s.size()) { }
  slice(const char* s) : ptr_(s), len_(::strlen(s)) { }

  template<size_t N>
  slice(const char (&d)[N]) : ptr_(d), len_(N) { }  // implicit!

  const char* get() const { return ptr_; }
  const char* data() const { return ptr_; }
  size_t size() const { return len_; }
  bool empty() const {
    return len_ == 0 || ptr_ == NULL || ptr_[0] == '\0';
  }
  const char& operator[](int idx) const {
    assert(idx < static_cast<int>(len_));
    return ptr_[idx];
  }

  size_t find(const slice& target) const {
    // more efficient algorithm should replace it
    for (size_t i = 0; i < len_; ++i) {
      if (::memcmp(&ptr_[i], target.ptr_, target.len_) == 0) {
        return i;
      }
    }
    return std::string::npos;
  }

  friend bool operator==(const slice& l, const slice& r) {
    if (l.len_ != r.len_) { return false; }
    return ::memcmp(l.ptr_, r.ptr_, l.len_) == 0;
  }

  friend bool operator!=(const slice& l, const slice& r) {
    return !(l == r);
  }

  std::vector<slice> split(const slice& delimiter) const {
    std::vector<slice> ret;
    size_t idx;
    slice p(ptr_, len_);

    while ((idx = p.find(delimiter)) != std::string::npos) {
      // new element
      ret.push_back(slice(p.ptr_, idx));
      // pointer goes ahead
      p.drop_head(idx + delimiter.len_);
    }
    ret.push_back(p);
    return ret;
  }

  void drop_head(size_t n) {
    assert(n <= len_);
    ptr_ += n;
    len_ -= n;
  }

  slice car(const slice& delimiter) const {
    const size_t pos = find(delimiter);
    if (pos == std::string::npos) {
      return *this;
    } else {
      return slice(ptr_, pos);
    }
  }

  slice cdr(const slice& delimiter) const {
    const size_t pos = find(delimiter);
    if (pos == std::string::npos) {
      return slice(NULL, 0);
    } else {
      size_t offset = pos + delimiter.len_;
      return slice(ptr_ + offset, len_ - offset);
    }
  }

  size_t count(const slice& target) const {
    const size_t count = 0;
    slice cursor = *this;
    for (;;) {
      const size_t offset = cursor.find(target);
      if (offset == std::string::npos) {
        return count;
      }
      cursor.ptr_ += offset + target.len_;
      cursor.len_ -= offset + target.len_;
    }
  }

  friend std::ostream& operator<<(std::ostream& o, const slice& s) {
    o << std::string(s.ptr_, s.len_);
    return o;
  }

  static std::string join(const std::vector<slice>& arr, const slice& glue) {
    if (arr.size() == 0) { return ""; }
    int len = 0;
    for (size_t i = 0; i < arr.size() - 1; ++i) {
      len += arr[i].size() + glue.len_;
    }
    len += arr[arr.size() - 1].len_;
    std::string joined;
    joined.reserve(len);
    for (size_t i = 0; i < arr.size() - 1; ++i) {
      joined += arr[i].ptr_;
      joined += glue.ptr_;
    }
    joined += arr[arr.size() - 1].ptr_;
    return joined;
  }

  slice suffix(size_t advance) const {
    if (len_ < advance) {
      return slice();
    }
    return slice(ptr_ + advance, len_ - advance);
  }

  operator std::string() const {
    return std::string(ptr_, len_);
  }

 private:
  friend bool operator<(const slice& l, const slice& r);

  // it does not have duty or right of free()
  const char* ptr_;
  size_t len_;
};

inline std::ostream& operator<<(std::ostream& o, const std::vector<slice>& v) {
  o << "[ ";
  for (size_t i = 0; i < v.size() - 1; ++i) {
    o << "\""<< v[i] << "\", ";
  }
  if (0 < v.size()) {o << v[v.size() - 1];}
  o << " ]";
  return o;
}

inline std::ostream& operator<<(std::ostream& o, const std::map<slice, slice>& v) {
  o << "{ ";
  for (std::map<slice, slice>::const_iterator it = v.begin();
       it != v.end();
       ++it) {
    o << "\"" << it->first << "\": "
      << "\"" << it->second << "\", ";
  }
  o << " }";
  return o;
}

inline bool operator<(const slice& l, const slice& r) {
  return ::strcmp(l.ptr_, r.ptr_) < 0;
}

}  // namespace util

namespace std {
template<>
struct hash<util::slice> {
public:
  inline size_t operator()(const util::slice &s) const {
    return MurmurHash64A (s.get(), s.size(), 0);
  }
};
};


#endif  // REIR_SLICE_HPP
