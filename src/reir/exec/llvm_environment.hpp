#ifndef REIR_LLVM_ENVIRONMENT_HPP_
#define REIR_LLVM_ENVIRONMENT_HPP_

#include <llvm/Support/TargetSelect.h>

namespace reir {

class llvm_environment {
 public:
  llvm_environment() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
  }
};

static llvm_environment env_;

}  // namespace reir

#endif  // REIR_LLVM_ENVIRONMENT_HPP_
