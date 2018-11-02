//
// Created by kumagi on 18/04/12.
//

#include <iostream>
#include <sstream>
#include "parser.hpp"
#include "token.hpp"
#include "reir/db/maybe_value.hpp"
#include "ast_node.hpp"
#include "ast_expression.hpp"
#include "ast_statement.hpp"

#define OUTPUT std::cout << __LINE__ << ": " << tokens.get() << std::endl;

void expect_token(const reir::Token& token, reir::token_type type);

namespace reir {
namespace node {

Expression* parse_expr(TokenStream& tokens);

Block::Block(TokenStream& tokens) : Statement(ND_Block) {
  expect_token(tokens.get(), token_type::OPEN_BRACE);
  tokens.next();
  while (tokens.get().type != token_type::CLOSE_BRACE) {
    auto* stmt = parse_statement(tokens);
    if (stmt) { add(stmt); }
  }
  expect_token(tokens.get(), token_type::CLOSE_BRACE);
  tokens.next();
}

Transaction::Transaction(TokenStream& tokens) : Statement(ND_Transaction) {
  expect_token(tokens.get(), token_type::TRANSACTION);
  tokens.next();

  if (tokens.get().type != token_type::OPEN_BRACE) {
    std::cout << "to: " << tokens.get() << std::endl;
    isolation_ = tokens.get().text;
    tokens.next();
  } else {
    isolation_ = "SERIALIZABLE";
  }

  expect_token(tokens.get(), token_type::OPEN_BRACE);
  sequence_ = new Block(tokens);
}

Let::Let(TokenStream& tokens) : Statement(ND_Let) {
  expect_token(tokens.get(), token_type::LET);
  tokens.next();
  if (tokens.get().type == token_type::OPEN_ANGLE) {
    type_ = parse_row_type(tokens);
  } else {
    type_ = nullptr;
  }

  expect_token(tokens.get(), token_type::IDENTIFIER);
  name_ = tokens.get().text;
  tokens.next();
  expect_token(tokens.get(), token_type::EQUAL);
  tokens.next();
  expr_ = parse_expr(tokens);
}

DefineTuple::DefineTuple(reir::TokenStream& tokens) : Statement(ND_DefineTuple) {
  expect_token(tokens.get(), token_type::TUPLE);
  tokens.next();
  expect_token(tokens.get(), token_type::IDENTIFIER);
  name_ = tokens.get().text;
  tokens.next();
  auto* ptr = dynamic_cast<TupleType*>(parse_type(tokens));
  if (ptr  == nullptr) {
    throw std::runtime_error("invalid define tuple");
  }
  type_ = ptr;
}

If::If(TokenStream& tokens) : Statement(ND_If), false_block_(nullptr) {
  expect_token(tokens.get(), token_type::IF);
  tokens.next();
  cond_ = parse_expr(tokens);
  expect_token(tokens.get(), token_type::OPEN_BRACE);
  true_block_ = new Block(tokens);
  if (tokens.has_next() && tokens.get().type == token_type::ELSE) {
    tokens.next();
    expect_token(tokens.get(), token_type::OPEN_BRACE);
    false_block_ = new Block(tokens);
  }
}


ExprStatement::ExprStatement(reir::TokenStream& s) : Statement(ND_ExprStatement) {
  value_ = parse_expr(s);
}

Define::Define(TokenStream& tokens) : Statement(ND_Define) {
  expect_token(tokens.get(), token_type::DEFINE);
  tokens.next();  // 'DEFINE'
  schema_ = dynamic_cast<TupleType*>(parse_row_type(tokens));
  expect_token(tokens.get(), token_type::IDENTIFIER);
  name_ = tokens.get().text;
  tokens.next();  // '>'
}


For::For(TokenStream& tokens) : Statement(ND_For) {
  expect_token(tokens.get(), token_type::FOR);
  tokens.next();
  if (tokens.get().type != token_type::SEMICOLON) {
    init_ = parse_statement(tokens);
  }
  expect_token(tokens.get(), token_type::SEMICOLON);
  tokens.next();
  if (tokens.get().type != token_type::SEMICOLON) {
    cond_ = parse_expr(tokens);
  }
  expect_token(tokens.get(), token_type::SEMICOLON);
  tokens.next();
  if (tokens.get().type != token_type::SEMICOLON) {
    every_ = parse_expr(tokens);
  }
  expect_token(tokens.get(), token_type::OPEN_BRACE);
  blk_ = new Block(tokens);
};

Statement* parse_statement(TokenStream& tokens) {
  assert(tokens.has_next());
  if (tokens.get().type == token_type::CLOSE_BRACE) {
    return nullptr;
  }
  switch (tokens.get().type) {
    case token_type::OPEN_BRACE: {
      return new Block(tokens);
    }
    case token_type::CLOSE_BRACE: {
      return nullptr;
    }
    case token_type::TRANSACTION: {
      return new Transaction(tokens);
    }
    case token_type::LET: {
      return new Let(tokens);
    }
    case token_type::IF: {
      return new If(tokens);
    }
    case token_type::FOR: {
      return new For(tokens);
    }
    case token_type::DEFINE: {
      return new Define(tokens);
    }
    case token_type::EMIT: {
      return new Emit(tokens);
    }
    case token_type::INSERT: {
      return new Insert(tokens);
    }
    case token_type::SCAN: {
      return new Scan(tokens);
    }
    case token_type::BREAK: {
      tokens.next();
      return new Jump(Jump::break_jump);
    }
    case token_type::CONTINUE: {
      tokens.next();
      return new Jump(Jump::continue_jump);
    }
    case token_type::OPEN_BRACKET:  // array literal
    case token_type::OPEN_PAREN:
    case token_type::NUMBER:
    case token_type::STRING_LITERAL:
    case token_type::IDENTIFIER: {
      return new ExprStatement(tokens);
    }
    case token_type::TUPLE: {
      return new DefineTuple(tokens);
    }
    default: {
      std::stringstream ss;
      if (tokens.get().type == token_type::IDENTIFIER) {
        ss << "unexpected identifier: " << tokens.get().text;
      } else {
        ss << "un-parsable token: " << token_type_str(tokens.get().type);
      }
      throw std::runtime_error(ss.str());
    }
  }
}

Emit::Emit(TokenStream& tokens) : Statement(ND_Emit) {
  expect_token(tokens.get(), token_type::EMIT);
  tokens.next();
  value_ = parse_expr(tokens);
}

Insert::Insert(TokenStream& tokens)
     : Statement(ND_Insert), key_stack_(nullptr), value_stack_(nullptr), tuple_stack_(nullptr), prefix_(nullptr) {
  expect_token(tokens.get(), token_type::INSERT);
  tokens.next();
  expect_token(tokens.get(), token_type::IDENTIFIER);
  table_ = tokens.get().text;
  tokens.next();
  expect_token(tokens.get(), token_type::OPEN_BRACE);
  value_ = parse_expr(tokens);
}

Scan::Scan(TokenStream& tokens)
     : Statement(ND_Scan), key_stack_(nullptr), value_stack_(nullptr), tuple_stack_(nullptr), prefix_begin_(nullptr), prefix_end_(nullptr) {
  expect_token(tokens.get(), token_type::SCAN);
  tokens.next();
  expect_token(tokens.get(), token_type::IDENTIFIER);
  table_ = tokens.get().text;
  tokens.next();
  expect_token(tokens.get(), token_type::COMMA);
  tokens.next();
  row_name_ = tokens.get().text;
  tokens.next();
  expect_token(tokens.get(), token_type::OPEN_BRACE);
  blk_ = new Block(tokens);
}

}  // namespace node
}  // namespace reir
