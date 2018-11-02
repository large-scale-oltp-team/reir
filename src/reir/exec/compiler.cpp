
#include <chrono>
#include <random>
#include <llvm/Support/DynamicLibrary.h>

#include <llvm/ADT/STLExtras.h>

#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Constants.h>

#include "compiler.hpp"
#include "tuple.hpp"
#include "reir/db/metadata.hpp"
#include "reir/engine/db_handle.hpp"

#include "compiler_context.hpp"

#include "leveldb_compiler.hpp"
#include "db_interface.hpp"
#include "llvm_util.hpp"
#include "ast_statement.hpp"
#include "ast_expression.hpp"

#define OFFSET_OF(struct_t, member) (reinterpret_cast<std::uint64_t>(&reinterpret_cast<struct_t*>(0)->member) / sizeof(void*))

namespace {
extern "C" {

size_t get_length(const std::string& s) {
  return s.length();
}
const char* get_data(const std::string& s) {
  std::cout << "getting data(" << s << ")" << std::endl;
  return s.data();
}

struct string_struct {
  char* data;
  uint64_t length;
};

void print_string(const string_struct& s) {
  // std::cout << "data: " << (void*)s.data << " len: " << s.length << std::endl;
  std::cout << std::string(s.data, s.length);
}

std::random_device rd;
int64_t rand_int(const int64_t from, const int64_t to) {
  std::uniform_int_distribution<int64_t> dist(from, to);
  return dist(rd);
}

void print_int(int64_t s) {
  std::cout << s << std::endl;
}

void print_value(const std::vector<reir::MaybeValue>& s) {
  for (size_t i = 0; i < s.size(); ++i) {
    std::cout << i << "->" << s[i] << std::endl;
  }
}


void emit(reir::CompilerContext* ctx, char* buff, size_t len) {
  ctx->get_output(buff, len);
}

void tmp(reir::node::Node* a) {
  std::cout << "a " << &a << ")" << std::endl;
}

void debug_out() {
  std::cout << "you came here!" << std::endl;
}

const reir::MaybeValue* value_at(const std::vector<reir::MaybeValue>& v, size_t idx) {
  std::cout << "key may be: " << v[idx] << std::endl;
  return &v[idx];
}

int64_t get_int_value(const reir::MaybeValue& v) {
  if (!v.exists()) {
    throw std::runtime_error("key is null");
  }
  const reir::Value& val = v.value();
  if (!val.is_int()) {
    throw std::runtime_error("key is not integer");
  }
  return val.as_int();
}

int64_t debug_string(const char* v) {
  for (size_t i = 0; i < 13; ++i) {
    std::cout << (int)v[i] << " " << std::flush;
  }
  std::cout << std::endl;
  return 0;
}

}  // extern C
}  // anonymous namespace

static const char toplevel[] = "__top_level";

namespace reir {

llvm::Function* get_table_name_check_function(CompilerContext& ctx, const std::string name) {
  auto it = ctx.functions_table_.find("table_name_check");
  if (it != ctx.functions_table_.end()) {
    return it->second;
  }
  std::vector<llvm::Type*> args;
  args.emplace_back(llvm::Type::getInt8PtrTy(ctx.ctx_));
  llvm::FunctionType* signature =
    llvm::FunctionType::get(llvm::Type::getInt1Ty(ctx.ctx_), args, false);
  llvm::Function* ret =
    llvm::Function::Create(signature,
                           llvm::Function::ExternalLinkage,
                           "table_name_check",
                           ctx.mod_.get());

  llvm::BasicBlock *table_name_check =
    llvm::BasicBlock::Create(ctx.ctx_, "table_name_check", ret);

  llvm::BasicBlock *return_false =
    llvm::BasicBlock::Create(ctx.ctx_, "return_false", ret);

  for (size_t i = 0; i < name.size() + 1; ++i) {
    auto* value = ctx.builder_.CreateLoad(
      ctx.builder_.CreateInBoundsGEP(llvm::Type::getInt8Ty(ctx.ctx_),
                                      ret->arg_begin(),
                                      ctx.builder_.getInt8(static_cast<uint8_t>(i))));
    auto* compared =
            ctx.builder_.CreateICmpEQ(value,
                                       ctx.builder_.getInt8(static_cast<uint8_t>(name[i])));

    std::stringstream ss;
    ss << "label" << std::to_string(i);
    std::string next_block_name(ss.str());
    llvm::BasicBlock* next_br =
      llvm::BasicBlock::Create(ctx.ctx_, next_block_name, ret);

    ctx.builder_.CreateCondBr(compared, next_br, return_false);
    ctx.builder_.SetInsertPoint(next_br);
  }
  ctx.builder_.CreateRet(ctx.builder_.getInt1(true));
  ctx.builder_.SetInsertPoint(return_false);
  ctx.builder_.CreateRet(ctx.builder_.getInt1(false));

  ctx.functions_table_.insert(std::make_pair("table_name_check", ret));
  return ret;
}

llvm::Function* get_value_at_function(CompilerContext& ctx) {
  // std::vector<MaybeValue>* -> int -> MaybeValue
  std::vector<llvm::Type*> args;
  args.emplace_back(llvm::Type::getInt64PtrTy(ctx.ctx_));
  args.emplace_back(llvm::Type::getInt64Ty(ctx.ctx_));
  llvm::FunctionType* signature =
      llvm::FunctionType::get(llvm::Type::getInt64PtrTy(ctx.ctx_), args, false);
  llvm::Function* ret =
    llvm::Function::Create(signature,
                           llvm::Function::ExternalLinkage,
                           "value_at",
                           ctx.mod_.get());
  return ret;
}

llvm::Function* get_int_value_function(CompilerContext& ctx) {
  // MaybeValue* -> int
  std::vector<llvm::Type*> args;
  args.emplace_back(llvm::Type::getInt64PtrTy(ctx.ctx_));  // MaybeValue*
  llvm::FunctionType* signature =
      llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx.ctx_), args, false);
  llvm::Function* ret =
    llvm::Function::Create(signature,
                           llvm::Function::ExternalLinkage,
                           "get_int_value",
                           ctx.mod_.get());
  return ret;
}
llvm::Function* get_debug_string(CompilerContext& ctx) {
  // char* -> int
  std::vector<llvm::Type*> args;
  args.emplace_back(llvm::Type::getInt8PtrTy(ctx.ctx_));  // char*
  llvm::FunctionType* signature =
      llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx.ctx_), args, false);
  llvm::Function* ret =
    llvm::Function::Create(signature,
                           llvm::Function::ExternalLinkage,
                           "debug_string",
                           ctx.mod_.get());
  return ret;
}

void emit_value_cell_serializer(CompilerContext& ctx, llvm::Type* buffer_type, llvm::Value* buffer, llvm::Value* v, size_t offset){
  // 8 bytes integer
  llvm::ArrayRef<llvm::Value*> ref({ctx.builder_.getInt64(0), ctx.builder_.getInt64(offset)});
  auto* dst_i8 = ctx.builder_.CreateInBoundsGEP(buffer_type,
                                                 buffer,
                                                 ref);
  auto* dst =
    ctx.builder_.CreateBitCast(dst_i8,
                                llvm::Type::getInt64PtrTy(ctx.ctx_));
  auto* store = ctx.builder_.CreateStore(v, dst);
  store->setAlignment(8);

  llvm::ArrayRef<llvm::Value*> head({ctx.builder_.getInt64(0), ctx.builder_.getInt64(0)});
  auto* as_i8 = ctx.builder_.CreateInBoundsGEP(buffer_type,
                                                buffer,
                                                head);
  // ast->values_[i]'s type must be Integer
  // then copy ast->value_[i].value() for 8 bytes
}

void Compiler::init_functions(CompilerContext& ctx) {
  {  // init malloc
    std::vector<llvm::Type*> args{
        llvm::Type::getInt64Ty(ctx.ctx_)
    };
    llvm::FunctionType* malloc_signature =
        llvm::FunctionType::get(llvm::Type::getInt8PtrTy(ctx.ctx_), args, false);

    llvm::Function* malloc_func =
        llvm::Function::Create(malloc_signature,
                               llvm::Function::ExternalLinkage,
                               "malloc",
                               ctx.mod_.get());

    std::vector<std::string> arg_names = {"size"};
    int idx = 0;
    for (auto& arg : malloc_func->args()) {
      malloc_func->arg_begin()[idx].setName(arg_names[idx]);
      ++idx;
    }
    ctx.functions_table_["malloc"] = malloc_func;
  }

  {
    std::vector<llvm::Type*> args{
        llvm::Type::getInt64PtrTy(ctx.ctx_),
        llvm::Type::getInt64PtrTy(ctx.ctx_)
    };

    llvm::FunctionType* drop_table_signature =
        llvm::FunctionType::get(ctx.builder_.getVoidTy(), args, false);
    auto* drop_table = llvm::Function::Create(drop_table_signature,
                               llvm::Function::ExternalLinkage,
                               "drop_table_func",
                               ctx.mod_.get());
    std::vector<std::string> arg_names = {"metadata", "ast"};
    int idx = 0;
    for (auto& arg : drop_table->args()) {
      drop_table->arg_begin()[idx].setName(arg_names[idx]);
      ++idx;
    }

    ctx.functions_table_["drop_table"] = drop_table;
  }
  {  // print_int
    llvm::Function* print_int =
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getVoidTy(ctx.ctx_),
                                    {llvm::Type::getInt64Ty(ctx.ctx_)},
                                    false),
            llvm::Function::ExternalLinkage,
            "print_int",
            ctx.mod_.get());
    std::vector<std::string> arg_names = {"value"};
    int idx = 0;
    for (auto& arg : print_int->args()) {
      arg.setName(arg_names[idx]);
      ++idx;
    }
    print_int->arg_begin()->setName("foobar");

    ctx.functions_table_["print_int"] = print_int;
  }
  {  // print_string
    llvm::Function* print_string =
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getVoidTy(ctx.ctx_),
                                    {ctx.type_table_["string"]->getPointerTo()},
                                    false),
            llvm::Function::ExternalLinkage,
            "print_string",
            ctx.mod_.get());
    std::vector<std::string> arg_names = {"value"};
    int idx = 0;
    for (auto& arg : print_string->args()) {
      arg.setName(arg_names[idx]);
      ++idx;
    }
    print_string->arg_begin()->setName("str");

    ctx.functions_table_["print_string"] = print_string;
  }
  {  // rand
    llvm::Function* rand_int =
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx.ctx_),
                                    {llvm::Type::getInt64Ty(ctx.ctx_),
                                     llvm::Type::getInt64Ty(ctx.ctx_)},
                                    false),
            llvm::Function::ExternalLinkage,
            "rand_int",
            ctx.mod_.get());
    std::vector<std::string> arg_names = {"min", "max"};
    int idx = 0;
    for (auto& arg : rand_int->args()) {
      arg.setName(arg_names[idx]);
      ++idx;
    }
    ctx.functions_table_["rand"] = rand_int;
  }
  {  // emit
    llvm::Function* emit_func =
        llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getVoidTy(ctx.ctx_),
                                    {llvm::Type::getInt64PtrTy(ctx.ctx_),
                                     llvm::Type::getInt8PtrTy(ctx.ctx_),
                                     llvm::Type::getInt64Ty(ctx.ctx_)},
                                    false),
            llvm::Function::ExternalLinkage,
            "__emit_func",
            ctx.mod_.get());
    std::vector<std::string> arg_names = {"ctx", "ptr", "length"};
    int idx = 0;
    for (auto& arg : emit_func->args()) {
      arg.setName(arg_names[idx]);
      ++idx;
    }
    ctx.functions_table_["__emit_func"] = emit_func;
  }
}

Compiler::Compiler()
  : target_machine_(llvm::EngineBuilder().selectTarget()),
    data_layout_(target_machine_->createDataLayout()),
    obj_layer_([]() { return std::make_shared<llvm::SectionMemoryManager>(); }),
    compile_layer_(obj_layer_, llvm::orc::SimpleCompiler(*target_machine_)),
    optimize_layer_(compile_layer_,
                    [this](std::shared_ptr<llvm::Module> M) {
                      return optimize_module(std::move(M));
                    }),
    CompileCallbackManager(
            llvm::orc::createLocalCompileCallbackManager(target_machine_->getTargetTriple(), 0)),
    CODLayer(optimize_layer_,
             [](llvm::Function &F) { return std::set<llvm::Function*>({&F}); },
             *CompileCallbackManager,
             llvm::orc::createLocalIndirectStubsManagerBuilder(
                 target_machine_->getTargetTriple())) {

  llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
}

std::shared_ptr<llvm::Module> Compiler::optimize_module(std::shared_ptr<llvm::Module> M) {
  // Create a function pass manager.
  auto FPM = llvm::make_unique<llvm::legacy::FunctionPassManager>(M.get());

  // Add some optimizations.
  FPM->add(llvm::createInstructionCombiningPass());
  FPM->add(llvm::createReassociatePass());
  FPM->add(llvm::createGVNPass());
  FPM->add(llvm::createCFGSimplificationPass());
  FPM->add(llvm::createMemCpyOptPass());
  FPM->add(llvm::createPromoteMemoryToRegisterPass());
  FPM->doInitialization();

  // Run the optimizations over all functions in the module being added to
  // the JIT.
  for (auto &F : *M)
    FPM->run(F);
  // llvm::outs() << *M << "\n";  // see optimized IR
  return M;
}

Compiler::ModuleHandle Compiler::add_module(std::unique_ptr<llvm::Module> M) {
  // Build our symbol resolver:
  // Lambda 1: Look back into the JIT itself to find symbols that are part of
  //           the same "logical dylib".
  // Lambda 2: Search for external symbols in the host process.
  auto resolver = llvm::orc::createLambdaResolver(
      [&](const std::string &Name) {
        if (auto Sym = CODLayer.findSymbol(Name, false))
          return Sym;
        return llvm::JITSymbol(nullptr);
      },
      [](const std::string &Name) {
        if (auto SymAddr =
            llvm::RTDyldMemoryManager::getSymbolAddressInProcess(Name))
          return llvm::JITSymbol(SymAddr, llvm::JITSymbolFlags::Exported);
        return llvm::JITSymbol(nullptr);
      });

  // Add the set to the JIT with the resolver we created above and a newly
  // created SectionMemoryManager.
  return cantFail(CODLayer.addModule(std::move(M), std::move(resolver)));
}

void insert_codegen(CompilerContext& ctx, DBInterface& dbi, MetaData& md, node::Insert* ast) {
  /*
  auto* table_name_check_func = get_table_name_check_function(ctx, ast->table_);
  auto* value_at_func = get_value_at_function(ctx);
  auto* value_int_func = get_int_value_function(ctx);
  auto* debug_string_func = get_debug_string(ctx);

  std::vector<llvm::Type*> ext_arg;
  ext_arg.emplace_back(llvm::Type::getInt64PtrTy(ctx.ctx_));  // ast

  std::vector<llvm::Type*> noarg;
  llvm::FunctionType* nosignature =
    llvm::FunctionType::get(ctx.builder_.getVoidTy(), noarg, false);
  llvm::Function* debug =
    llvm::Function::Create(nosignature,
                           llvm::Function::ExternalLinkage,
                           "debug_out",
                           ctx.mod_);

  std::vector<llvm::Type*> args;
  args.emplace_back(llvm::Type::getInt64PtrTy(ctx.ctx_));  // ast
  llvm::FunctionType* ft =
    llvm::FunctionType::get(llvm::Type::getInt64PtrTy(ctx.ctx_), args, false);
  llvm::Function* print_string =
    llvm::Function::Create(ft,
                           llvm::Function::ExternalLinkage,
                           "print_string",
                           ctx.mod_);


  llvm::BasicBlock *bb = llvm::BasicBlock::Create(ctx.ctx_, "insert", ctx.func_);
  ctx.builder_.SetInsertPoint(bb);

  std::vector<llvm::Expression*> argsv;
  auto* first_arg = &ctx.func_->arg_begin()[0];
  argsv.emplace_back(first_arg);
  */
  return;
  /*
  auto* table_name =
    ctx.builder_.CreateInBoundsGEP(llvm::Type::getInt64Ty(ctx.ctx_),
                                    first_arg,
                                    ctx.builder_.getInt64(OFFSET_OF(node::Insert, table_)));
  auto* values =
    ctx.builder_.CreateInBoundsGEP(llvm::Type::getInt64Ty(ctx.ctx_),
                                    first_arg,
                                    ctx.builder_.getInt64(OFFSET_OF(node::Insert, values_)));

  std::vector<llvm::Type*> get_data_args{llvm::Type::getInt64PtrTy(ctx.ctx_)};
  llvm::FunctionType* get_data_func_proto =
    llvm::FunctionType::get(llvm::Type::getInt8PtrTy(ctx.ctx_), get_data_args, false);
  llvm::Function* get_data =
    llvm::Function::Create(get_data_func_proto,
                           llvm::Function::ExternalLinkage,
                           "get_data",
                           ctx.mod_);

  std::vector<llvm::Expression*> table_name_args{table_name};
  auto* table_name_ptr =
    ctx.builder_.CreateCall(get_data, table_name_args);

  std::vector<llvm::Expression*> check_table_name_args{table_name_ptr};
  auto *name_equal =
      ctx.builder_.CreateCall(table_name_check_func, check_table_name_args);

  llvm::BasicBlock* invalid =
    llvm::BasicBlock::Create(ctx.ctx_, "invalid_table_name", ctx.func_);
  llvm::BasicBlock* valid_table_name =
    llvm::BasicBlock::Create(ctx.ctx_, "valid_table_name", ctx.func_);

  auto *compared =
      ctx.builder_.CreateICmpEQ(name_equal, ctx.builder_.getInt1(true));
  ctx.builder_.CreateCondBr(compared, valid_table_name, invalid);
  ctx.builder_.SetInsertPoint(invalid);
  ctx.builder_.CreateRet(ctx.builder_.getInt32(0));  // table name mismatch, finish

  ctx.builder_.SetInsertPoint(valid_table_name);

  // serialize key

  llvm::Type *keytype, *valuetype;
  llvm::AllocaInst *key, *value;
  size_t key_len, value_len;
  auto tmp_schema = ctx.local_schema_table_.find(ast->table_);

  const reir::Schema& sc = tmp_schema != ctx.local_schema_table_.end() ?
                          tmp_schema->second :
                          md.get_schema(ast->table_);

  if (sc.fixed_key_length()) {
    key_len = sc.key_length(ast->values_);
    keytype = llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx.ctx_), key_len);
    key = ctx.builder_.CreateAlloca(keytype);
    key->setAlignment(1);
    std::string prefix = sc.get_prefix();

    for (size_t i = 0; i < prefix.size(); ++i) {
      llvm::ArrayRef<llvm::Expression*> ref({ctx.builder_.getInt64(0), ctx.builder_.getInt64(i)});
      auto* tmp = ctx.builder_.CreateInBoundsGEP(keytype,
                                                  key,
                                                  ref);
      auto* store = ctx.builder_.CreateStore(ctx.builder_.getInt8(prefix[i]),
                                              tmp);
      store->setAlignment(1);
    }

    // serialize key
    size_t offset = prefix.size();
    sc.each_attr([&](size_t i, const Attribute& attr) {
        if (!attr.is_key()) { return; }
        if (attr.type().is_integer()) {
          // 8 bytes integer
          std::vector<llvm::Expression*> args{values, ctx.builder_.getInt64(i)};
          std::vector<llvm::Expression*> maybe_value{ctx.builder_.CreateCall(value_at_func, args)};
          auto* v = ctx.builder_.CreateCall(value_int_func, maybe_value);
          emit_value_cell_serializer(ctx, keytype, key, v, offset);
          offset += sizeof(int64_t);
        } else {
          // other type key
          throw std::runtime_error("not implemented yet");
        }
      });
    //std::vector<llvm::Expression*> arg{key};
    //ctx.builder_.CreateCall(debug_string_func, arg);
  } else {
    throw std::runtime_error("variable length key is not supported yet");
  }

  // serialize value
  size_t value_offset = 0;
  if (sc.fixed_value_length()) {
    value_len = sc.value_length(ast->values_);
    valuetype = llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx.ctx_), value_len);
    value = ctx.builder_.CreateAlloca(valuetype);
    value->setAlignment(1);

    // serialize value
    size_t offset = 0;
    sc.each_attr([&](size_t i, const Attribute& attr) {
        if (attr.is_key()) { return; }
        if (attr.type().is_integer()) {
          // 8 bytes integer
          std::vector<llvm::Expression*> args{values, ctx.builder_.getInt64(i)};
          std::vector<llvm::Expression*> maybe_value{ctx.builder_.CreateCall(value_at_func, args)};
          auto* v = ctx.builder_.CreateCall(value_int_func, maybe_value);
          emit_value_cell_serializer(ctx, valuetype, value, v, offset);
          offset += sizeof(int64_t);
        } else {
          // other type value
          throw std::runtime_error("not implemented yet");
        }
      });
    //std::vector<llvm::Expression*> arg{value};
    //ctx.builder_.CreateCall(debug_string_func, arg);
  } else {
    throw std::runtime_error("variable length value is not supported yet");
  }


  llvm::ArrayRef<llvm::Expression*> ref({ctx.builder_.getInt64(0), ctx.builder_.getInt64(0)});
  auto* key_i8 =
      ctx.builder_.CreateInBoundsGEP(keytype,
                                      key,
                                      ref);
  auto* value_i8 =
      ctx.builder_.CreateInBoundsGEP(valuetype,
                                      value,
                                     ref);
  dbi.emit_insert(ctx,
                  key_i8, ctx.builder_.getInt64(key_len),
                  value_i8, ctx.builder_.getInt64(value_len));

  llvm::Function* link_test =
      llvm::Function::Create(nosignature,
                             llvm::Function::ExternalLinkage,
                             "link_test",
                             ctx.mod_);
  ctx.builder_.CreateCall(link_test);
  ctx.builder_.CreateRet(ctx.builder_.getInt32(1));
   */
}



void create_table_func(MetaData* md, std::string name, std::vector<Attribute>&& columns) {
  md->create_table(name, std::move(columns));
}

/*
void drop_table_func(MetaData* md, node::DropTable* dt) {
md->drop_table(dt);
std::cout << "call drop table with[";
dt->dump(std::cout);
std::cout << "]" << std::endl;
}
*/


/*
void Compiler::create_table_call_codegen(CompilerContext &ctx, DBInterface &dbh,
                                         MetaData &md, node::CreateTable *crt) {
  reir::Schema s(crt->name_, crt->columns_);
  ctx.local_schema_table_.emplace(crt->name_, s);
  std::vector<llvm::Expression*> ct_args({get_ptr(ctx, &md), get_ptr(ctx, crt)});
  crt->dump(std::cout);
  std::cout << &md << std::endl;
  ctx.builder_.CreateCall(embedded_functions_["create_table"], ct_args);
}

void Compiler::drop_table_call_codegen(CompilerContext& ctx,
                                       DBInterface& dbi,
                                       MetaData& md,
                                       node::DropTable* drt) {
  ctx.local_schema_table_.erase(drt->name_);
  std::vector<llvm::Expression*> dt_args({get_ptr(ctx, &md), get_ptr(ctx, drt)});
  ctx.builder_.CreateCall(embedded_functions_["drop_table"], dt_args, "hoge");
}

void Compiler::transaction_codegen(CompilerContext &ctx, DBInterface &dbi,
                                   MetaData &md, node::Transaction *txn) {
  dbi.emit_begin_txn(ctx);
  ctx.builder_.CreateRet(ctx.builder_.getInt32(1));
}

void Compiler::let_codegen(CompilerContext &ctx,
                           DBInterface &dbi,
                           MetaData &md,
                           node::Let *let) {
  switch (let->expr_.type_) {
  case node::Node::NodeType::IntegerExprT : {
    auto *val = expr_codegen(ctx, dbi, md, let->expr_);
    auto *int_expr = reinterpret_cast<node::IntegerExpr>(let->expr_);
    auto *stk = ctx.builder_.CreateAlloca(llvm::Type::getInt64Ty(ctx.ctx_),
                                           nullptr,
                                           let->name_);
    stk->setAlignment(8);
    ctx.builder_.CreateStore(
        expr_codegen(ctx, dbi, md, let->expr_), stk)->setAlignment(8);
    ctx.variable_table_.emplace(let->name_, stk);
    // ctx.builder_.CreateLoad(stk, let->)->setAlignment(8);;
    break;
  }
  default:
    throw std::runtime_error("unknown expr type");
  }
}

void Compiler::if_codegen(CompilerContext &ctx,
                          DBInterface &dbi,
                          MetaData &md,
                          node::If *if_ast) {
  auto* cond = if_ast->cond_;
  llvm::BasicBlock *true_blk =
      llvm::BasicBlock::Create(ctx.ctx_, "true_blk", ctx.func_);
  llvm::BasicBlock *false_blk =
      llvm::BasicBlock::Create(ctx.ctx_, "false_blk", ctx.func_);
  llvm::BasicBlock *finish_blk =
      llvm::BasicBlock::Create(ctx.ctx_, "finish_blk", ctx.func_);
  auto *result = ctx.builder_.CreateICmpNE(expr_codegen(ctx, dbi, md, cond),
                                            ctx.builder_.getInt64(0));
  ctx.builder_.CreateCondBr(result, true_blk, false_blk);

  // true block
  ctx.builder_.SetInsertPoint(true_blk);
  get_value(ctx, dbi, md, if_ast->true_block_);
  ctx.builder_.CreateBr(finish_blk);

  // false block
  ctx.builder_.SetInsertPoint(false_blk);
  if (if_ast->false_block_) {
    assign(ctx, dbi, md, if_ast->false_block_);
  }
  ctx.builder_.CreateBr(finish_blk);

  ctx.builder_.SetInsertPoint(finish_blk);
}

llvm::Expression* Compiler::expr_codegen(CompilerContext& ctx,
                                    DBInterface& dbi,
                                    MetaData& md,
                                    node::Expr* expr) {
  switch (expr->type_) {
  case node::Node::NodeType::IntegerExprT : {;
    auto *int_expr = reinterpret_cast<node::IntegerExpr*>(expr);
    return ctx.builder_.getInt64(int_expr->value_);
  }
  default:
    throw std::runtime_error("unsupported expr type");
  }
  return nullptr;
}

void Compiler::print_codegen(CompilerContext& ctx, DBInterface& dbi, MetaData& md, node::Print* prt) {
  assert(prt->target_->type_ == node::Node::NodeType::IntegerExprT);
  std::cout << "print is ";
  prt->target_->dump(std::cout);
  auto *int_expr = reinterpret_cast<node::IntegerExpr*>(prt->target_);
  auto* value = expr_codegen(ctx, dbi,md, int_expr);
  std::vector<llvm::Expression*> ct_args({value});
  ctx.builder_.CreateCall(embedded_functions_["print_int"], ct_args);
}
*/

void init_types(CompilerContext& ctx) {
  auto* maybe_struct_type = ctx.mod_->getTypeByName("string_type");
  if (maybe_struct_type != nullptr) {
    ctx.type_table_["string"] = maybe_struct_type;
  } else if (ctx.type_table_.find("string") == ctx.type_table_.end()) {
    auto* strtype = llvm::StructType::create(ctx.ctx_,
                                             {ctx.builder_.getInt8PtrTy(),
                                              ctx.builder_.getInt64Ty()},
                                             "string_type");
    ctx.type_table_["string"] = strtype;
  }
}

void Compiler::compile(CompilerContext& ctx, DBInterface& dbi, MetaData& md, node::Node* ast) {
  init_types(ctx);
  init_functions(ctx);
  ast->analyze(ctx);

  llvm::BasicBlock *bb = llvm::BasicBlock::Create(ctx.ctx_, "entry_jit", ctx.func_);
  ctx.builder_.SetInsertPoint(bb);

  llvm::sys::DynamicLibrary::AddSymbol("print_int", (void*)&print_int);
  llvm::sys::DynamicLibrary::AddSymbol("print_string", (void*)&print_string);
  llvm::sys::DynamicLibrary::AddSymbol("rand_int", (int64_t*)&rand_int);
  llvm::sys::DynamicLibrary::AddSymbol("__emit_func", (void*)&emit);

  auto* whole_block = reinterpret_cast<node::Block*>(ast);
  whole_block->each_statement([&](const node::Statement* n) -> void {
    n->alloca_stack(ctx);
  });

  whole_block->codegen(ctx);
  ctx.builder_.CreateRet(ctx.builder_.getInt1(true));
#ifndef NDEBUG
  ctx.dump();
#endif
}

void Compiler::compile_and_exec(DBInterface& dbi, MetaData& md, node::Node* ast) {
  CompilerContext ctx(*this, &dbi, &md);
  compile_and_exec(ctx, dbi, md, ast);
}

void Compiler::compile_and_exec(CompilerContext& ctx, DBInterface& dbi, MetaData& md, node::Node* ast) {
  if (ast == nullptr) {
    return;
  }
#ifndef NDEBUG
  ast->dump(std::cout, 0);
  std::cout << std::endl;
#endif

  auto start_time = std::chrono::steady_clock::now();
  compile(ctx, dbi, md, ast);

  auto handle = add_module(std::move(ctx.mod_));
  ctx.init();

  auto ExprSymbol = this->find_symbol(ctx.get_name());
  assert(ExprSymbol && "Function not found");

#ifndef NDEBUG
  auto compiled_time = std::chrono::steady_clock::now();
  auto compile_duration = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(compiled_time - start_time).count()) / 1000000;
  std::cout << "compiled in " << compile_duration
            << " sec.\n";
#endif

  auto ret = (exec_func)llvm::cantFail(ExprSymbol.getAddress());
  if (ret == nullptr) {
    std::cout << "failed to compile " << std::endl;
    cantFail(CODLayer.removeModule(handle));
    return;
  }
  bool a = ret(ast);

#ifndef NDEBUG
  auto executed_time = std::chrono::steady_clock::now();
  auto executed_duration = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(executed_time - compiled_time).count()) / 1000000;
  std::cout << "executed in " << executed_duration << " sec." << std::endl;
  std::cout << "returns: " << a << std::endl;
#endif
  std::cout << "emitted values: ";
  std::vector<RawRow>& outputs = ctx.outputs_;
  for (int i = 0; i < outputs.size(); ++i) {
    if (0 < i) std::cout << "\n";
    std::cout << outputs[i];
  }
  std::cout << "\n";

  cantFail(CODLayer.removeModule(handle));
}

llvm::JITSymbol Compiler::find_symbol(const std::string& name) {
  std::string MangledName;
  llvm::raw_string_ostream MangledNameStream(MangledName);
  llvm::Mangler::getNameWithPrefix(MangledNameStream, name, data_layout_);
  return CODLayer.findSymbol(MangledNameStream.str(), true);
}

}  // namespace reir
