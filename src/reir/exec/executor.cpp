#define NO_RTTI 1

#include "executor.hpp"
#include "compiler.hpp"
#include "ast_node.hpp"
#include "reir/exec/db_interface.hpp"
#include "reir/db/metadata.hpp"

#include <iostream>
#include <sstream>
#include "reir/engine/foedus_executor.hpp"

namespace {

struct encoded_kvp {
  std::unique_ptr<char[]> key;
  std::unique_ptr<char[]> val;
  encoded_kvp(std::unique_ptr<char[]>&& k, std::unique_ptr<char[]>&& v)
      : key(std::move(k)), val(std::move(v)) {}
};


void insert_into(reir::DBInterface& dbh, reir::MetaData& md, reir::node::Insert* ast) {
  // execution without JIT compilation
  /*
  const reir::Schema& sc(md.get_schema(ast->table_));
  size_t key_len = sc.key_length(ast->values_);
  size_t val_len = sc.val_length(ast->values_);
  std::string key;
  std::string value;
  key.resize(key_len);
  value.resize(val_len);
  sc.encode_key(ast->values_, &key[0]);
  sc.encode_value(ast->values_, &value[0]);

  if (dbh.get_name() == "foedus") {
    foedus::proc::ProcArguments* arg = dbh.get_foedus_engine();
    reir::foedus::insert(arg, md, ast, key, value);
  } else if (dbh.get_name() == "leveldb") {

  }
   */
}

}  // anonymous namespace

namespace reir {
class MetaData;

Executor::Executor() : jit_compiler_(new Compiler()){}

void Executor::execute(DBInterface& dbi, MetaData& md, node::Node* ast, bool jit) {
  if (jit) {
    jit_compiler_->compile_and_exec(dbi, md, ast);
  } else {
    // exec without jit
    std::stringstream ss;
    throw std::runtime_error("executor not implemented for " + ss.str());
  }
}

Executor::~Executor() {
}

}  // namespace reir
