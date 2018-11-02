//
// Created by kumagi on 18/01/18.
//

#ifndef PROJECT_LEVELDB_COMPILER_HPP
#define PROJECT_LEVELDB_COMPILER_HPP
namespace leveldb {
class DB;
}
namespace reir {

class MetaData;
namespace node {
class Insert;
}
void leveldb_insert_codegen(leveldb::DB* DB, MetaData& md, node::Insert* ast);

}

#endif //PROJECT_LEVELDB_COMPILER_HPP
