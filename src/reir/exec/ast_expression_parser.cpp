//
// Created by kumagi on 18/07/04.
//

#include "token.hpp"
#include "ast_expression.hpp"
#include "reir/db/maybe_value.hpp"

void expect_token(const reir::Token& token,
                  reir::token_type type);
namespace reir {
class TokenStream;
namespace node {

Expression* parse_expr(TokenStream& tokens);

Expression* parse_expr_postfix(Expression* ret, TokenStream& tokens) {
  if (!tokens.has_next()) {
    return ret;
  }
  switch (tokens.get().type) {
    case token_type::OPEN_PAREN: {
      // function call
      return parse_expr_postfix(new FunctionCall(ret, tokens), tokens);
    }
    case token_type::OPEN_BRACKET: {
      // array index access
      return parse_expr_postfix(new ArrayReference(dynamic_cast<VariableReference*>(ret), tokens), tokens);
    }
    case token_type::PERIOD: {
      // member access
      return parse_expr_postfix(new MemberReference(ret, tokens), tokens);
    }
      /*
    case token_type::OPEN_ANGLE: {
      return parse_expr_postfix(new TupleType(tokens), tokens);
    }
      */
    case token_type::EQUAL: {
      ret = parse_expr_postfix(new Assign(ret, tokens), tokens);
      break;
    }
    case token_type::PLUS:
    case token_type::MINUS:
    case token_type::ASTERISK:
    case token_type::SLASH:
    case token_type::PERCENT:
    case token_type::OPEN_ANGLE:
    case token_type::CLOSE_ANGLE:
    case token_type::EQUAL_EQUAL:
    case token_type::EXCLAMATION_MARK_EQUAL:
    case token_type::OPEN_ANGLE_EQUAL:
    case token_type::CLOSE_ANGLE_EQUAL:
    case token_type::AND_AND:
    case token_type::OR_OR: {
      auto op = tokens.get();
      tokens.next();
      auto* rhs = parse_expr(tokens);
      operators ope;
      switch (op.type) {
        case token_type::PLUS:
          ope = operators::PLUS; break;
        case token_type::MINUS:
          ope = operators::MINUS; break;
        case token_type::ASTERISK:
          ope = operators::MULTIPLE; break;
        case token_type::SLASH:
          ope = operators::DIVISION; break;
        case token_type::PERCENT:
          ope = operators::MODULO; break;
        case token_type::OPEN_ANGLE:
          ope = operators::LESSTHAN; break;
        case token_type::CLOSE_ANGLE:
          ope = operators::MORETHAN; break;
        case token_type::EQUAL_EQUAL:
          ope = operators::EQUAL; break;
        case token_type::EXCLAMATION_MARK_EQUAL:
          ope = operators::NOTEQUAL; break;
        case token_type::OPEN_ANGLE_EQUAL:
          ope = operators::LESSEQUAL; break;
        case token_type::CLOSE_ANGLE_EQUAL:
          ope = operators::MOREEQUAL; break;
        case token_type::AND_AND:
          ope = operators::CONDITIONAL_AND; break;
        case token_type::OR_OR:
          ope = operators::CONDITIONAL_OR; break;
        default:
          throw std::runtime_error(std::string("unknown binary operator: ") + std::string(tokens.get().text));
      }
      ret = new BinaryExpression(ret, ope, rhs);
      break;
    }
    default:
      return ret;
  };
  return ret;
}
/*
      expect_token(tokens.get(), token_type::IDENTIFIER);
      auto* ref = new VariableReference(tokens);
      if (!tokens.has_next()) return ref;
      switch (tokens.get().type) {
        case token_type::OPEN_BRACKET: {
          ret = parse_expr_postfix(new ArrayReference(ref, tokens), tokens);
          break;
        }
        case token_type::PERIOD: {
          ret = parse_expr_postfix(new MemberReference(ref, tokens), tokens);
          break;
        }
        case token_type::OPEN_PAREN: {
          // function call
          ret = parse_expr_postfix(new FunctionCall(ref, tokens), tokens);
          break;
        }
        case token_type::EQUAL: {
          ret = new Assign(ref, tokens);
          break;
        }
        default: {
          return ref;
        }
      }
      break;
      */

Expression* parse_expr(TokenStream& tokens) {
  Expression* ret;
  switch (tokens.get().type) {
    case token_type::AND: {
      tokens.next();
      ret = new PointerOf(parse_expr(tokens));
      break;
    }
    case token_type::OPEN_BRACE: {
      ret = new RowLiteral(tokens);
      break;
    }
    case token_type::OPEN_PAREN: {
      tokens.next();
      ret = parse_expr(tokens);
      expect_token(tokens.get(), token_type::CLOSE_PAREN);
      tokens.next();
      break;
    }
    case token_type::OPEN_BRACKET: {
      ret = new ArrayLiteral(tokens);
      break;
    }
    case token_type::IDENTIFIER: {
      ret = new VariableReference(tokens);
      break;
    }
    case token_type::STRING_LITERAL:
    case token_type::NUMBER: {
      ret = new PrimaryExpression(tokens);
      break;
    }
    default:
      throw std::runtime_error(std::string("unknown expr: ") + std::string(tokens.get().text));
  }
  if (!tokens.has_next()) {
    return ret;
  }
  return parse_expr_postfix(ret, tokens);
}

PrimaryExpression::PrimaryExpression(TokenStream& tokens)
    : Expression(ND_Primary) {
  switch (tokens.get().type) {
    case token_type::NUMBER: {
      value_ = MaybeValue((int64_t) std::stoi(tokens.get().text));
      tokens.next();  // number
      break;
    }
    case token_type::STRING_LITERAL: {
      value_ = MaybeValue(tokens.get().text);
      tokens.next();  // string_literal
      break;
    }
    default: {
      std::stringstream ss;
      type_ = new PrimaryType(type_id::NONE_TYPE);
      ss << "unsupported primary expression(" << tokens.get().text << ")" << "passed";
      throw std::runtime_error(ss.str());
    }
  }
}

VariableReference::VariableReference(TokenStream& tokens)
    : Expression(ND_Variable), name_(tokens.get().text) {
  expect_token(tokens.get(), token_type::IDENTIFIER);
  tokens.next();
}

ArrayReference::ArrayReference(Expression* parent, TokenStream& tokens)
    : Expression(ND_ArrayRef), parent_(parent) {
  expect_token(tokens.get(), token_type::OPEN_BRACKET);
  tokens.next();  // '['
  idx_ = parse_expr(tokens);
  expect_token(tokens.get(), token_type::CLOSE_BRACKET);
  tokens.next();  // ']'
}

ArrayReference::~ArrayReference() {
  delete parent_;
  delete idx_;
}

MemberReference::MemberReference(Expression* value, TokenStream& tokens)
    : Expression(ND_MemberRef), parent_(value) {
  expect_token(tokens.get(), token_type::PERIOD);
  tokens.next();
  name_ = tokens.get().text;
  tokens.next();
}

MemberReference::~MemberReference() {
  delete parent_;
}


FunctionCall::FunctionCall(Expression* parent, TokenStream& tokens)
    : Expression(ND_Func), parent_(parent) {
  expect_token(tokens.get(), token_type::OPEN_PAREN);
  tokens.next();
  while (tokens.get().type != token_type::CLOSE_PAREN) {
    auto* arg = parse_expr(tokens);
    args_.emplace_back(arg);
    if (tokens.get().type == token_type::CLOSE_PAREN) {
      break;
    }
    expect_token(tokens.get(), token_type::COMMA);
    tokens.next();  // ','
  }
  expect_token(tokens.get(), token_type::CLOSE_PAREN);
  tokens.next();  // ')'
}

RowLiteral::RowLiteral(TokenStream& tokens) : Expression(ND_Row) {
  expect_token(tokens.get(), token_type::OPEN_BRACE);
  tokens.next();  // '{'
  std::vector<Type*> row_types;
  while (tokens.get().type != token_type::CLOSE_BRACE) {
    auto* new_expr = parse_expr(tokens);
    row_types.push_back(new_expr->type_);
    elements_.emplace_back(new_expr);
    if (tokens.get().type == token_type::CLOSE_BRACE) {
      break;
    }
    expect_token(tokens.get(), token_type::COMMA);
    tokens.next();
  }
  // type_ = new TupleType(std::move(row_types));
  expect_token(tokens.get(), token_type::CLOSE_BRACE);
  tokens.next();  // '}'
}


ArrayLiteral::ArrayLiteral(TokenStream& tokens) : Expression(ND_Array) {
  expect_token(tokens.get(), token_type::OPEN_BRACKET);
  tokens.next();  // '['
  while (tokens.get().type != token_type::CLOSE_BRACKET) {
    auto* new_expr = parse_expr(tokens);
    elements_.emplace_back(new_expr);
    if (tokens.get().type == token_type::CLOSE_BRACKET) {
      break;
    }
    expect_token(tokens.get(), token_type::COMMA);
    tokens.next();
  }
  expect_token(tokens.get(), token_type::CLOSE_BRACKET);
  tokens.next();  // ']'
}

Assign::Assign(Expression* target, TokenStream& tokens) : Expression(ND_Assign){
  target_ = target;
  expect_token(tokens.get(), token_type::EQUAL);
  tokens.next();
  value_ = parse_expr(tokens);
}

}  // namespace node
}  // namespace reir
