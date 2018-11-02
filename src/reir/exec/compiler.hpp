#ifndef REIR_COMPILER_HPP_
#define REIR_COMPILER_HPP_

#include <cassert>

#include <unordered_map>
#include <iostream>

#include <llvm/ADT/STLExtras.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/RuntimeDyld.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/IR/IRBuilder.h>

#include "tuple.hpp"
#include "ast_node.hpp"

namespace llvm {
class TargetMachine;
class Function;
class Module;
}  // namespace llvm

namespace reir {
namespace node {
class Node;
}
class MetaData;
class DBInterface;
class CompilerContext;

class Compiler {
  void init_functions(CompilerContext& ctx);
  using OptimizeFunction =
      std::function<std::shared_ptr<llvm::Module>(std::shared_ptr<llvm::Module>)>;

  std::unique_ptr<llvm::TargetMachine> target_machine_;
  llvm::DataLayout data_layout_;
  llvm::orc::RTDyldObjectLinkingLayer obj_layer_;
  std::unique_ptr<llvm::orc::JITCompileCallbackManager> CompileCallbackManager;
  llvm::orc::IRCompileLayer<decltype(obj_layer_), llvm::orc::SimpleCompiler> compile_layer_;
  llvm::orc::IRTransformLayer<decltype(compile_layer_), OptimizeFunction> optimize_layer_;
  llvm::orc::CompileOnDemandLayer<decltype(optimize_layer_)> CODLayer;

 public:
  using ModuleHandle = decltype(CODLayer)::ModuleHandleT;

 public:
  Compiler();

  typedef bool(*exec_func)(node::Node*);

  void compile_and_exec(DBInterface& dbi, MetaData& md, node::Node* ast);
  void compile_and_exec(CompilerContext& ctx, DBInterface& dbi, MetaData& md, node::Node* ast);
  llvm::TargetMachine* get_target_machine() {
    return target_machine_.get();
  }

  ModuleHandle add_module(std::unique_ptr<llvm::Module> m);
  llvm::JITSymbol find_symbol(const std::string& name);

  ~Compiler() = default;

 private:
  void compile(CompilerContext& ctx, DBInterface& dbi, MetaData& md, node::Node* ast);
  std::shared_ptr<llvm::Module> optimize_module(std::shared_ptr<llvm::Module> M);

 public: // it should be private and friend classess

  // llvm members
};

}  // namespace reir

#endif  // REIR_COMPILER_HPP_
