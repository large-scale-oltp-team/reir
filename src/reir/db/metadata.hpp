#ifndef REIR_DB_METADATA_HPP_
#define REIR_DB_METADATA_HPP_
#include <string>
#include <vector>
#include <unordered_map>
#include "schema.hpp"

namespace leveldb {
class DB;
}

namespace reir {

class Schema;

class MetaData {
 public:
  MetaData();
  ~MetaData();
  void show_tables() const;
  void create_table(const std::string& name, std::vector<Attribute>&& columns);
  void drop_table(const std::string& name);
  Schema get_schema(const std::string& name);
 private:
  std::unordered_map<std::string, Schema> tables_;
  leveldb::DB* db_;
};

}  // namespace reir

#endif  // REIR_DB_METADATA_HPP_

