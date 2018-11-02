#include <assert.h>
#include <leveldb/db.h>

#include <iostream>
#include <sstream>
#include <memory>
#include <chrono>
#include <thread>

#include "metadata.hpp"
#include "reir/db/attribute.hpp"

namespace reir {
MetaData::MetaData() {
  leveldb::Options options;
  //options.IncreaseParallelism();
  //options.OptimizeLevelStyleCompaction();
  options.create_if_missing = true;
  options.error_if_exists = false;
  leveldb::Status s = leveldb::DB::Open(options, "/tmp/hoge", &db_);
  if (!s.ok()) {
    std::cerr << s.ToString() << std::endl;
  }
  assert(s.ok());
}

void MetaData::show_tables() const {
  std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions()));
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    // std::cout << it->key().ToString() << " => "  << it->value().ToString() << std::endl;
  }
  assert(it->status().ok());  // Check for any errors found during the scan

  for (auto it = tables_.begin();
       it != tables_.end();
       ++it) {
    std::cout << it->first << " -> " << it->second << std::endl;
  }
}

void MetaData::create_table(const std::string& name, std::vector<Attribute>&& attr) {
  std::stringstream key;
  key << "table:global_schema:" << name;
  Schema new_schema(name, std::move(attr));
  tables_.insert(std::make_pair(key.str(), new_schema));
  {
    std::string value;
    new_schema.serialize(value);
    db_->Put(leveldb::WriteOptions(), key.str(), value);
  }
}

void MetaData::drop_table(const std::string& name) {
}

Schema MetaData::get_schema(const std::string& name) {
  std::string value;
  std::stringstream key;
  key << "table:" << "global_schema:" << name;
  leveldb::Status s = db_->Get(leveldb::ReadOptions(), key.str(), &value);

  if(s.ok()) {
    Schema ret;
    ret.deserialize(value);
    return ret;
  } else {
    throw std::runtime_error("table " + name + " not found");
  }
}

MetaData::~MetaData() {
  delete db_;
}

}  // namespace reir
