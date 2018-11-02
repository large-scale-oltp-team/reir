//
// Created by kumagi on 18/01/18.
//

#include "leveldb_compiler.hpp"

namespace leveldb {
class DB;
}

namespace reir {
class MetaData;
namespace node {
class Insert;
}  // namespace node

void leveldb_insert_codegen(leveldb::DB *DB, MetaData& md, node::Insert *ast);

}  // namespace reir