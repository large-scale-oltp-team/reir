#ifndef REIR_DB_HANDLE_HPP_
#define REIR_DB_HANDLE_HPP_
#include <string>
#include <assert.h>

namespace foedus {
namespace proc {
struct ProcArguments;
}
}
namespace leveldb {
class DB;
}

namespace reir {

/* it may hold FOEDUS or any other Transaction Engine */

class DBHandle {
 public:
  DBHandle(const std::string& name); // factory
  template<typename T>
  void set(T *p) {
    ptr_ = reinterpret_cast<void *>(p);
  }
  std::string get_name() const {
    return name_;
  }

  foedus::proc::ProcArguments* get_foedus_engine() {
    assert(name_ == "foedus");
    return reinterpret_cast<foedus::proc::ProcArguments*>(ptr_);
  }

  leveldb::DB *get_leveldb() {
    assert(name_ == "leveldb");
    return reinterpret_cast<leveldb::DB*>(ptr_);
  }

 private:
  std::string name_;
  void* ptr_;
};

}  // namespace reir

#endif  // REIR_DB_HANDLE_HPP_
