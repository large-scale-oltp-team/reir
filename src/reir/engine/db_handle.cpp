#include <stdexcept>
#include "db_handle.hpp"

namespace reir {

DBHandle::DBHandle(const std::string& name) : name_(name) {
  if (name == "foedus") {
  } else {
    throw std::runtime_error("unknown db type: " + name);
  }
}

}  // namespace reir
