//
// Created by kumagi on 18/05/07.
//

#include <memory>
#include "ast_node.hpp"
#include "token.hpp"

void expect_token(const reir::Token& token,
                  reir::token_type type) {
  if (token.type != type) {
    std::stringstream ss;
    ss << "line:" << token.row << ":" << token.col << " "
        << "token: " << token << " is not expected "
       << reir::token_type_str(type) << " desired";
    throw std::runtime_error(ss.str());
  }
}

namespace reir {
namespace node {

PrimaryType::PrimaryType(TokenStream& tokens) {
  expect_token(tokens.get(), token_type::INT);
  type_ = type_parse(tokens.get().text);
  tokens.next();  // IDENTIFIER
  /*
  expect_token(tokens.get(), token_type::COLON);
  tokens.next();  // ':'
  expect_token(tokens.get(), token_type::IDENTIFIER);
  name_ = std::string(tokens.get().text);
  std::string value_name = tokens.get().text;
  tokens.next();  // IDENTIFIER

  uint32_t prop = Attribute::AttrProperty::NONE;
  while (tokens.get().type == token_type::IDENTIFIER) {
    if (tokens.get().type == token_type::IDENTIFIER) {
      if (tokens.get().text == "key") {
        prop |= (uint32_t) Attribute::AttrProperty::KEY;
      } else if (tokens.get().text == "nullable") {
        prop |= Attribute::AttrProperty::NULLABLE;
      } else if (tokens.get().text == "unique") {
        prop |= Attribute::AttrProperty::UNIQUE;
      } else {
        throw std::runtime_error(std::string("unknown property: ") + std::string(tokens.get().text));
      }
    }
    tokens.next();
  }
  prop_ = Attribute::AttrProperty(prop);
   */
}

ArrayType::ArrayType(reir::TokenStream& tokens)
    : Type(type_id::ARRAY) {
  // [integer] means integer array
  expect_token(tokens.get(), token_type::OPEN_BRACKET);
  tokens.next();
  type_ = parse_type(tokens)->type_;
  expect_token(tokens.get(), token_type::CLOSE_BRACKET);
  tokens.next();
}

TupleType::TupleType(reir::TokenStream& tokens)
    : Type(type_id::TUPLE) {
  expect_token(tokens.get(), token_type::OPEN_BRACE);
  tokens.next();
  for (;;) {
    // expect_token(tokens.get(), token_type::IDENTIFIER);
    std::string tn = tokens.get().text;
    tokens.next();
    bool parametized = false;
    if (tokens.get().type == token_type::OPEN_PAREN) {
      tokens.next();
      expect_token(tokens.get(), token_type::NUMBER);
      params_.push_back(std::make_shared<int>(std::stoi(tokens.get().text)));
      tokens.next();
      expect_token(tokens.get(), token_type::CLOSE_PAREN);
      tokens.next();
      parametized = true;
    }

    expect_token(tokens.get(), token_type::COLON);
    tokens.next();
    if (!parametized) {
      params_.push_back(nullptr);
    }

    expect_token(tokens.get(), token_type::IDENTIFIER);
    std::string nm = tokens.get().text;
    tokens.next();

    type_names_.push_back(tn);
    names_.push_back(nm);

    uint32_t prop = Attribute::AttrProperty::NONE;
    while (tokens.get().type == token_type::IDENTIFIER) {
      if (tokens.get().type == token_type::IDENTIFIER) {
        if (tokens.get().text == "key") {
          prop |= (uint32_t) Attribute::AttrProperty::KEY;
        } else if (tokens.get().text == "nullable") {
          prop |= Attribute::AttrProperty::NULLABLE;
        } else if (tokens.get().text == "unique") {
          prop |= Attribute::AttrProperty::UNIQUE;
        } else {
        throw std::runtime_error(std::string("unknown property: ") + std::string(tokens.get().text));
        }
      }
      tokens.next();
    }
    props_.push_back(Attribute::AttrProperty(prop));

    if (tokens.get().type != token_type::COMMA) break;
    expect_token(tokens.get(), token_type::COMMA);
    tokens.next();
  }
  expect_token(tokens.get(), token_type::CLOSE_BRACE);
  tokens.next();
}

Type* parse_type(TokenStream& tokens) {
  switch (tokens.get().type) {
    case token_type::OPEN_BRACKET:
      return new ArrayType(tokens);
    case token_type::OPEN_BRACE:
      return new TupleType(tokens);
    default:
      return new PrimaryType(tokens);
  }
}

Type* parse_row_type(TokenStream& tokens) {
  expect_token(tokens.get(), token_type::OPEN_ANGLE);
  tokens.next();  // '<'
  auto* t = parse_type(tokens);
  expect_token(tokens.get(), token_type::CLOSE_ANGLE);
  tokens.next();  // '>'
  return t;
}
}  // namespace node
}  // namespace reir