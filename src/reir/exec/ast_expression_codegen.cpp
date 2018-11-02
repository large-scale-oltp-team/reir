//
// Created by kumagi on 18/07/04.
//


#include <llvm/Support/raw_ostream.h>
#include <unordered_map>
#include "ast_node.hpp"
#include "ast_expression.hpp"
#include "compiler_context.hpp"

#include "debug.hpp"
#include "llvm_util.hpp"

namespace reir {

llvm::Value* value_emitter(CompilerContext& ctx, const MaybeValue& v) {
  if (v.exists()) {
    const Value& just_value = v.value();
    if (just_value.is_int()) {
      auto int_value = static_cast<uint64_t>(just_value.as_int());
      return ctx.builder_.getInt64(int_value);
    } else if (just_value.is_varchar()) {
      const auto& str = just_value.as_varchar();
      auto* string_type = ctx.type_table_["string"];
      auto* string_struct = reinterpret_cast<llvm::StructType*>(string_type);

      auto* head = llvm::dyn_cast<llvm::Constant>(ctx.builder_.CreateGlobalStringPtr(str));
      std::vector < llvm::Constant * > values;
      values.emplace_back(head);
      values.emplace_back(ctx.builder_.getInt64(str.length()));
      auto* init_struct = llvm::ConstantStruct::get(string_struct, values);
#ifndef NDEBUG
      std::cerr << "global struct:";
      LLVM_DUMP(init_struct);
#endif
      return init_struct;
    } else {
      throw std::runtime_error("unsupported value");
    }
  } else {
    return ctx.builder_.getInt8(0);
  }
}

namespace node {

llvm::Type* convert_type(node::Type* type, CompilerContext& c) {
  switch (type->type_) {
    case node::type_id::INTEGER:
      return c.builder_.getInt64Ty();
    case node::type_id::STRING:
      return c.type_table_["string"];
    case node::type_id::DOUBLE:
      return c.builder_.getDoubleTy();
    case node::type_id::NONE_TYPE:
      throw std::runtime_error("unknown type");
    default: {
      throw std::runtime_error("convert_type: unknown type: " + node::type_to_string(type->type_));
    }
  }
}

llvm::Type* PrimaryExpression::get_type(CompilerContext& c) const {
  switch (value_.which()) {
    case 0: { // MaybeValue
      const MaybeValue& v = boost::get<MaybeValue>(value_);
      if (v.exists()) {
        const Value& val = v.value();
        if (val.is_int()) {
          return c.builder_.getInt64Ty();
        } else if (val.is_varchar()) {
          return c.type_table_["string"];
        } else if (val.is_float()) {
          return c.builder_.getDoubleTy();
        } else {
          throw std::runtime_error("unknown type");
        }
      }
      return c.builder_.getInt32Ty();
    }
    case 1: { // Expression*
      Expression* n = boost::get<Expression*>(value_);
      return n->get_type(c);
    }
    default:
      throw std::runtime_error("get_type: unknown type");
  }
}

llvm::Value* PrimaryExpression::get_value(CompilerContext& ctx) const {
  switch (value_.which()) {
    case 0:  // MaybeValue
      return value_emitter(ctx, boost::get<MaybeValue>(value_));
    case 1:  // std::string
      return nullptr;
    case 2:  // Expression*
      return boost::get<Expression*>(value_)->get_value(ctx);
    default:
      throw std::runtime_error("get_type: unknown type");
  }
}

llvm::Type* FunctionCall::get_type(CompilerContext& c) const {
  auto* function_body = reinterpret_cast<VariableReference*>(parent_);
  const auto& name = function_body->name_;
  const auto& func = c.functions_table_.find(name);
  if (func == c.functions_table_.end()) {
    throw std::runtime_error("undefined function: " + name);
  } else {
    return func->second->getReturnType();
  }
}

std::string get_table_name(Expression* vref) {
  auto* name = reinterpret_cast<VariableReference*>(vref);
  return name->name_;
}

llvm::Value* FunctionCall::get_value(CompilerContext& c) const {
  auto* function_body = reinterpret_cast<VariableReference*>(parent_);
  assert(function_body);
  const auto& name = function_body->name_;
  const auto func = c.functions_table_.find(name);

  if (func == c.functions_table_.end()) {
    throw std::runtime_error("undefined function: " + name);
  } else {
    auto* function = const_cast<llvm::Function*>(func->second);  // function
    std::vector<llvm::Value*> args;
    args.reserve(args_.size());
    for (const auto& a : args_) {
      auto* value = a->get_value(c);
      if (value->getType()->isIntegerTy()) {
        args.emplace_back(value);
      } else if (value->getType()->isStructTy()) {
        auto* stack = c.stack_table_[a];
        c.builder_.CreateStore(value, stack);
        args.emplace_back(stack);
      } else if (value->getType()->isPointerTy()) {
        if (value->getType()->getPointerElementType()->isIntegerTy()) {
          args.emplace_back(c.builder_.CreateLoad(value));
        } else {
          args.emplace_back(value);
        }
      } else {
        std::string base;
        llvm::raw_string_ostream ss(base);
        ss << "unexpected argument type: ";
        value->getType()->print(ss);
        throw std::runtime_error(ss.str());
      }
    }
    return c.builder_.CreateCall(function, args);
  }
}

std::string get_type_signature(CompilerContext& c, const std::vector<Expression*>& types) {
  std::string type_signature;
  llvm::raw_string_ostream ss(type_signature);
  ss << "{";
  for (const auto& e : types) {
    llvm::Type* t = e->get_type(c);
    t->print(ss);
    ss<< ",";
  }
  ss << "}";
  return ss.str();
}

llvm::Type* RowLiteral::get_type(CompilerContext& c) const {
  auto sign = get_type_signature(c, elements_);
  auto it = c.type_table_.find(sign);
  if (it != c.type_table_.end()) {
    return it->second;
  } else {
    std::vector<llvm::Type*> types;
    types.reserve(elements_.size());
    for (const auto& e : elements_) {
      types.emplace_back(e->get_type(c));
    }
    auto* new_type = llvm::StructType::create(c.ctx_, types, sign);
    auto elms = new_type->element_begin();
    for (size_t i = 0; i < new_type->getNumElements(); ++i) {
    }
    c.type_table_[sign] = new_type;
    return new_type;
  }
}

llvm::Value* RowLiteral::get_value(CompilerContext& c) const {
  auto sign =  get_type_signature(c, elements_);
  auto* type = c.mod_->getTypeByName(get_type_signature(c, elements_));
  if (type == nullptr) {
    std::cerr << "not defined type: " << sign << "\n";
    assert(!"row type must be defined before run");
  }
 	llvm::Value* prev = llvm::UndefValue::get(type);
  for (unsigned int i = 0; i < elements_.size(); ++i) {
    std::vector<unsigned int> idx{i};
    if (elements_[i]->get_value(c)->getType()->isPointerTy()) {
      auto* val = c.builder_.CreateLoad(elements_[i]->get_value(c));
      prev = c.builder_.CreateInsertValue(prev, val, idx);
    } else {
      prev = c.builder_.CreateInsertValue(prev, elements_[i]->get_value(c), idx);
    }
  }
  return prev;
}

llvm::Type* ArrayLiteral::get_type(CompilerContext& c) const {
  if (elements_.empty()) {
    throw std::runtime_error("zero-length array-literal cannot be defined");
  }
  llvm::Type* first_elemenents_type = elements_[0]->get_type(c);
  return llvm::ArrayType::get(first_elemenents_type, elements_.size());
}

llvm::Value* ArrayLiteral::get_value(CompilerContext& c) const {
  auto* type = get_type(c);
  assert(type && "array type must be declared");

 	llvm::Value* prev = llvm::UndefValue::get(type);
  for (unsigned int i = 0; i < elements_.size(); ++i) {
    std::vector<unsigned int> idx{i};
    prev = c.builder_.CreateInsertValue(prev, elements_[i]->get_value(c), idx);
  }
  return prev;
}

llvm::Type* VariableReference::get_type(CompilerContext& c) const {
  auto it = c.variable_table_.find(name_);
  if (it != c.variable_table_.end()) {
    assert(it->second->getType()->isPointerTy());
    auto* element_ty = llvm::dyn_cast<llvm::PointerType>(it->second->getType())->getPointerElementType();
    return element_ty;
  } else {
    throw std::runtime_error("get_type() value: " + name_ + " not defined");
  }
}

llvm::Value* VariableReference::get_value(CompilerContext& c) const {
  return c.builder_.CreateLoad(get_ref(c));
}

llvm::Value* VariableReference::get_ref(CompilerContext& c) const {
  auto it = c.variable_table_.find(name_);
  if (it != c.variable_table_.end()) {
    return it->second;
  } else {
    throw std::runtime_error("value: " + name_ + " not defined");
  }
}

llvm::Type* PointerOf::get_type(CompilerContext& c) const {
  return target_->get_type(c)->getPointerTo();
}

llvm::Value* PointerOf::get_value(CompilerContext& c) const {
  auto* val = target_->get_value(c);
  llvm::LoadInst* inst = reinterpret_cast<llvm::LoadInst*>(val);
  return inst->getPointerOperand();
}

llvm::Type* ArrayReference::get_type(CompilerContext& c) const {
  return parent_->get_type(c)->getArrayElementType();
}

llvm::Value* ArrayReference::get_value(CompilerContext& c) const {
  return c.builder_.CreateLoad(get_ref(c));
}

llvm::Value* ArrayReference::get_ref(CompilerContext& c) const {
  auto* arr = parent_->get_value(c);
  auto* inst = llvm::dyn_cast<llvm::LoadInst>(arr);
  auto* idx = idx_->get_value(c);
  return c.builder_.CreateInBoundsGEP(inst->getPointerOperand(),
                                            {c.builder_.getInt64(0), idx},
                                            "arr_ref");
}


llvm::Type* MemberReference::get_type(CompilerContext& c) const {
  return convert_type(type_, c);
}

llvm::Value* MemberReference::get_value(CompilerContext& c) const {
  return c.builder_.CreateLoad(get_ref(c));
}

llvm::Value* MemberReference::get_ref(CompilerContext& c) const {
  auto* v = dynamic_cast<VariableReference*>(parent_);
  auto* ptr = c.variable_table_[v->name_];
  return c.builder_.CreateInBoundsGEP(ptr,
                                           {c.builder_.getInt32(0),
                                            c.builder_.getInt32(static_cast<uint32_t>(offset_))},
                                           name_);

}
llvm::Type* Assign::get_type(CompilerContext& c) const {
  return value_->get_type(c);
}

llvm::Value* Assign::get_value(CompilerContext& c) const {
  auto* target = target_->get_ref(c);
  c.builder_.CreateStore(value_->get_value(c), target);
  return c.builder_.CreateLoad(target);
}

llvm::Type* BinaryExpression::get_type(CompilerContext& c) const {
  if (rhs_->get_type(c) != lhs_->get_type(c)) {
    throw std::runtime_error("binary operation can't do with different types");
  }
  return rhs_->get_type(c);
}

llvm::Value* BinaryExpression::get_value(CompilerContext& c) const {
  llvm::Value* lh = nullptr;
  llvm::Value* rh = nullptr;
  // for conditional operators, delay rhs evaluation for short circuit
  if (op_ != CONDITIONAL_AND && op_ != CONDITIONAL_OR) {
    lh = lhs_->get_value(c);
    rh = rhs_->get_value(c);
  }
  switch (op_) {
    case PLUS:
      return c.builder_.CreateAdd(lh, rh, "add");
    case MINUS:
      return c.builder_.CreateSub(lh, rh, "sub");
    case MULTIPLE:
      return c.builder_.CreateMul(lh, rh, "mul");
    case DIVISION:
      return c.builder_.CreateSDiv(lh, rh, "div");
    case EQUAL:
      return c.builder_.CreateICmpEQ(lh, rh, "compare");
    case MODULO:
      return c.builder_.CreateSRem(lh, rh, "mod");
    case LESSTHAN:
      return c.builder_.CreateICmpSLT(lh, rh, "lt");
    case LESSEQUAL:
      return c.builder_.CreateICmpSLE(lh, rh, "le");
    case MORETHAN:
      return c.builder_.CreateICmpSGT(lh, rh, "gt");
    case MOREEQUAL:
      return c.builder_.CreateICmpSGE(lh, rh, "ge");
    case NOTEQUAL:
      return c.builder_.CreateICmpNE(lh, rh, "ne");
    case CONDITIONAL_AND:
      return get_conditional_value(c, true);
    case CONDITIONAL_OR:
      return get_conditional_value(c, false);
    default:
      throw std::runtime_error("unknown operator compiled");
  }
}

// for implicit conversion from expression to bool
llvm::Value* BinaryExpression::to_i1(CompilerContext& c, llvm::Value* lh) const {
  if (lh->getType() != llvm::Type::getInt1Ty(c.ctx_)) {
    return c.builder_.CreateICmpNE(lh, c.builder_.getInt64(0));
  }
  return lh;
}

llvm::Value* BinaryExpression::get_conditional_value(CompilerContext& c, bool forAND) const {
  auto next = llvm::BasicBlock::Create(c.ctx_, "bin_next", c.func_);
  auto fin = llvm::BasicBlock::Create(c.ctx_, "bin_fin", c.func_);
  llvm::Value* lh = to_i1(c, lhs_->get_value(c));
  // depending on first operand, skip evaluation for second (short circuit).
  auto& first = forAND ? next : fin;
  auto& second = forAND ? fin : next;
  c.builder_.CreateCondBr(lh, first, second);
  llvm::BasicBlock* current = c.builder_.GetInsertBlock();
  c.builder_.SetInsertPoint(next);
  llvm::Value* rh = to_i1(c, rhs_->get_value(c));
  c.builder_.CreateBr(fin);
  next = c.builder_.GetInsertBlock();
  c.builder_.SetInsertPoint(fin);
  auto phi_node = c.builder_.CreatePHI(c.builder_.getInt1Ty(), 2, "phi");
  phi_node->addIncoming(lh, current);
  phi_node->addIncoming(rh, next);
  return phi_node;
}

}  // namespace node
}  // namespace reir
