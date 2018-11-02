//
// Created by kumagi on 18/02/20.
//

#ifndef PROJECT_LLVM_UTIL_HPP
#define PROJECT_LLVM_UTIL_HPP

namespace llvm {
struct Value;
}

namespace reir {
class ComplerContext;

template <typename T>
llvm::Value* get_ptr(const reir::CompilerContext& ctx, T* from){
  intptr_t addr = reinterpret_cast<intptr_t>(from);
  auto& c(const_cast<reir::CompilerContext&>(ctx));
  return
    llvm::ConstantExpr::getIntToPtr(llvm::ConstantInt::get(llvm::Type::getInt64Ty(c.ctx_), addr),
                                    llvm::PointerType::getUnqual(llvm::Type::getInt64Ty(c.ctx_)));
}
}  // namespace reir

#endif //PROJECT_LLVM_UTIL_HPP
