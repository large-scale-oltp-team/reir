//
// Created by kumagi on 18/01/18.
//

#ifndef PROJECT_FOEDUS_COMPILER_HPP
#define PROJECT_FOEDUS_COMPILER_HPP

namespace foedus {
class Engine;
namespace proc {
class ProcArguments;
}
}

namespace llvm {
class Type;
class Value;
class AllocaInst;
}

namespace reir {
class MetaData;
class CompilerContext;
namespace node {
class Insert;
}  // namespace node

void foedus_insert_codegen(CompilerContext& ctx,
                           foedus::proc::ProcArguments *engine,
                           llvm::Type* keytype,
                           llvm::AllocaInst* key,
                           size_t key_len,
                           llvm::Type* valuetype,
                           llvm::AllocaInst* value,
                           size_t value_len);

}  // namespace reir
#endif //PROJECT_FOEDUS_COMPILER_HPP
