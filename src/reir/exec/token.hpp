#ifndef REIR_TOKEN_HPP_
#define REIR_TOKEN_HPP_

#include <vector>
#include <string>
#include "util/slice.hpp"

namespace reir {

enum token_type : size_t {
  IDENTIFIER,
  STRING_LITERAL,
  RESERVED,
  NUMBER,
  FLOAT,

  // symbols
  OPEN_BRACE,
  CLOSE_BRACE,
  OPEN_PAREN,
  CLOSE_PAREN,
  OPEN_BRACKET,
  CLOSE_BRACKET,
  OPEN_ANGLE,
  CLOSE_ANGLE,
  COMMA,
  ASTERISK,
  SLASH,
  PLUS,
  MINUS,
  COLON,
  SEMICOLON,
  PERIOD,
  EQUAL,
  AND,
  PERCENT,

  EQUAL_EQUAL,
  EXCLAMATION_MARK_EQUAL,
  OPEN_ANGLE_EQUAL,
  CLOSE_ANGLE_EQUAL,
  PLUS_EQUAL,
  MINUS_EQUAL,
  ASTERISK_EQUAL,
  SLASH_EQUAL,
  PERCENT_EQUAL,
  AND_AND,
  OR_OR,

  // reserved_words
  reserved_splitter,  // Don't use it
  TYPE,
  INT,
  DEFINE,
  LET,
  TRANSACTION,
  SERIALIZABLE,
  IF,
  ELSE,
  FOR,
  EMIT,
  INSERT,
  SCAN,
  BREAK,
  CONTINUE,
  TUPLE,

  // invalid
  INVALID,
};

const static char* strs[] = {
  "IDENTIFIER",
  "STRING_LITERAL",
  "RESERVED",
  "NUMBER",
  "FLOAT",

  // symbols
  "OPEN_BRACE",
  "CLOSE_BRACE",
  "OPEN_PAREN",
  "CLOSE_PAREN",
  "OPEN_BRACKET",
  "CLOSE_BRACKET",
  "OPEN_ANGLE",
  "CLOSE_ANGLE",
  "COMMA",
  "ASTERISK",
  "SLASH",
  "PLUS",
  "MINUS",
  "COLON",
  "SEMICOLON",
  "PERIOD",
  "EQUAL",
  "AND",
  "PERCENT",

  // compound symbols
  "EQUAL_EQUAL",
  "EXCLAMATION_MARK_EQUAL",
  "OPEN_ANGLE_EQUAL",
  "CLOSE_ANGLE_EQUAL",
  "PLUS_EQUAL",
  "MINUS_EQUAL",
  "ASTERISK_EQUAL",
  "SLASH_EQUAL",
  "PERCENT_EQUAL",
  "AND_AND",
  "OR_OR",

  "reserved_splitter",  // Don't use it
  "TYPE",
  "INT",
  "DEFINE",
  "LET",
  "TRANSACTION",
  "SERIALIZABLE",
  "IF",
  "ELSE",
  "FOR",
  "EMIT",
  "INSERT",
  "SCAN",
  "BREAK",
  "CONTINUE",
  "TUPLE",

  "INVALID"
};


const size_t from = reserved_splitter + 1;
const size_t to = INVALID;

inline const char* token_type_str(enum token_type t) {
  return strs[t];
};

inline token_type as_reserved(const util::slice& word) {
  std::string w(word);
  for (size_t i = 0; i < w.size(); ++i) {
    w[i] &= ~0x20;  // capitalize
  }
  for (size_t i = from; i < to; ++i) {
    if (w == strs[i]) {
      return token_type(i);
    }
  }
  return INVALID;
}

static const std::string symbols = "{}()[],:;";
static const std::string alphabets =
  "abcdefghijklmnopqrstuvwxyz"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const std::string numbers = "1234567890";
static const std::string float_number = "1234567890.";

static const std::string operator_tokens = "+-*/%=!<>.&|";

inline token_type symbol_token(char c) {
  switch(c) {
  case '{':
    return OPEN_BRACE;
  case '}':
    return CLOSE_BRACE;
  case '(':
    return OPEN_PAREN;
  case ')':
    return CLOSE_PAREN;
  case '[':
    return OPEN_BRACKET;
  case ']':
    return CLOSE_BRACKET;
  case '<':
    return OPEN_ANGLE;
  case '>':
    return CLOSE_ANGLE;
  case ',':
    return COMMA;
  case '*':
    return ASTERISK;
  case '/':
    return SLASH;
  case '+':
    return PLUS;
  case '-':
    return MINUS;
  case ':':
    return COLON;
  case ';':
    return SEMICOLON;
  case '.':
    return PERIOD;
  case '=':
    return EQUAL;
  case '&':
    return AND;
  case '%':
    return PERCENT;
  default:
    abort();
  }
}

struct operator_token_def {
  const char* image;
  token_type type;
};

static const operator_token_def operator_token_list[] = {
  { "==", EQUAL_EQUAL },
  { "!=", EXCLAMATION_MARK_EQUAL },
  { "<=", OPEN_ANGLE_EQUAL },
  { ">=", CLOSE_ANGLE_EQUAL },
  { "+=", PLUS_EQUAL },
  { "-=", MINUS_EQUAL },
  { "*=", ASTERISK_EQUAL },
  { "/=", SLASH_EQUAL },
  { "%=", PERCENT_EQUAL },
  { "&&", AND_AND },
  { "||", OR_OR },
  { nullptr, INVALID },
};

inline token_type operator_token(const util::slice& slice) {
  if (slice.size() == 1) {
    return symbol_token(*slice.data());
  }
  std::string image { slice };
  for (size_t i = 0; operator_token_list[i].image != nullptr; ++i) {
    auto& def = operator_token_list[i];
    if (def.image == image) {
      return def.type;
    }
  }
  return INVALID;
}

struct Token {
  token_type type;
  util::slice text;
  int row;
  int col;
  friend std::ostream& operator<<(std::ostream& o, const Token& t) {
    o << "[" << token_type_str(t.type) << "]";
    switch (t.type) {
    case IDENTIFIER:
    case STRING_LITERAL:
    case RESERVED:
    case NUMBER:
    case FLOAT:
      o << "(" << t.text << ")";
      break;
    default:
      break; // do nothing
    }
    return o;
  }
};

class TokenStream {
public:
  TokenStream(std::vector<reir::Token>&& tokens) : tokens_(tokens), idx_(0) {}

  reir::Token&& get() {
    if (tokens_.size() <= idx_) {
      throw std::runtime_error("reached EOF of tokens");
    }
    return std::move(tokens_[idx_]);
  }

  void next() {
    ++idx_;
  }

  size_t pos() const {
    return idx_;
  }

  size_t size() const {
    return tokens_.size();
  }

  bool has_next() const {
    return idx_ != tokens_.size();
  }

private:
  std::vector<reir::Token> tokens_;
  size_t idx_;
};

}  // namespace reir

#endif  // REIR_TOKEN_HPP_
