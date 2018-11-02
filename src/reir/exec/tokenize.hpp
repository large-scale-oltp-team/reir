#ifndef REIR_TOKENIZE_HPP_
#define REIR_TOKENIZE_HPP_

#include <vector>
#include <string>
#include "util/slice.hpp"
#include "token.hpp"

namespace reir {

static const std::string delimiters = "\n \t";


inline bool in_reserverd_word(const util::slice& word) {
  std::string w(word);
  for (size_t i = 0; i < w.size(); ++i) {
    w[i] &= ~0x20;  // capitalize
  }
  for (size_t i = from; i < to; ++i) {
    if (w == strs[i]) {
      return true;
    }
  }
  return false;
}

inline void add_token(std::vector<Token>& ret, const util::slice& word, int row, int col) {
  if (in_reserverd_word(word)) {
    ret.emplace_back(Token{as_reserved(word), word, row});
  } else {
    ret.emplace_back(Token{IDENTIFIER, word, col});
  }
}

inline void add_operator(std::vector<Token>& ret, const util::slice& word, int row, int col) {
  auto type = operator_token(word);
  // TODO: for INVALID tokens
  ret.emplace_back(Token{type, word, row, col});
}

enum class lexer_state {
  INIT,
  NAME,
  NUMBER,
  STRING,
  OPERATOR,
};

inline void add_token(lexer_state state,
                      std::vector<Token>& ret,
                      const util::slice& word, int row, int col) {
  switch (state) {
    case lexer_state::NAME:
      add_token(ret, word, row, col);
      break;
    case lexer_state::NUMBER:
      ret.emplace_back(Token{NUMBER, word, row, col});
      break;
    case lexer_state::OPERATOR:
      add_operator(ret, word, row, col);
      break;
    default:
      abort();
  }
}

inline std::vector<Token> tokenize(const std::string& code) {
  std::vector<Token> ret;
  size_t from = 0;
  size_t len = 0;
  int row = 1;
  int col = 0;
  auto current_state = lexer_state::INIT;

  for (size_t i = 0; i < code.size(); ++i) {
    const char& c = code[i];
    if (current_state == lexer_state::STRING) {
      // TODO: escape sequences
      if (c == '"') {
        ret.emplace_back(Token{STRING_LITERAL, util::slice(&code[from + 1], len - 1), row, col});
        current_state = lexer_state::INIT; from = i; len = 0;
      } else {
        ++len;
      }
      ++col;
      continue;
    }
    if (c == '"') {
      if (current_state != lexer_state::INIT) {
        add_token(current_state, ret, util::slice(&code[from], len), row, col);
      }
      current_state = lexer_state::STRING; from = i; len = 0;
      ++len;
    } else if (delimiters.find(c) != std::string::npos) {
      if (current_state != lexer_state::INIT) {
        add_token(current_state, ret, util::slice(&code[from], len), row, col);
      }
      current_state = lexer_state::INIT; from = i; len = 0;
      if (c == '\n') {
        row++;
        col = -1;
      }
    } else if (symbols.find(c) != std::string::npos) {
      if (current_state != lexer_state::INIT) {
        add_token(current_state, ret, util::slice(&code[from], len), row, col);
      }
      ret.emplace_back(Token{symbol_token(c), util::slice(&code[i], 1), row, col});
      current_state = lexer_state::INIT; from = i; len = 0;
    } else if (operator_tokens.find(c) != std::string::npos) {
      switch (current_state) {
        case lexer_state::INIT:
          current_state = lexer_state::OPERATOR; from = i; len = 0;
          break;
        case lexer_state::NAME:
        case lexer_state::NUMBER:
          add_token(current_state, ret, util::slice(&code[from], len), row, col);
          current_state = lexer_state::OPERATOR; from = i; len = 0;
          break;
        case lexer_state::OPERATOR:
          break;
        default:
          abort();
      }
      ++len;
    } else if (numbers.find(c) != std::string::npos) {
      switch (current_state) {
        case lexer_state::INIT:
          current_state = lexer_state::NUMBER; from = i; len = 0;
          break;
        case lexer_state::NAME:
        case lexer_state::NUMBER:
          break;
        case lexer_state::OPERATOR:
          add_token(current_state, ret, util::slice(&code[from], len), row, col);
          current_state = lexer_state::NUMBER; from = i; len = 0;
          break;
        default:
          abort();
      }
      ++len;
    } else {
      switch (current_state) {
        case lexer_state::INIT:
          current_state = lexer_state::NAME; from = i; len = 0;
          break;
        case lexer_state::NAME:
          break;
        case lexer_state::NUMBER:
          throw std::runtime_error("don't use identifier " +
                                   std::string(&c, 1) + " inside of number: " +
                                   std::string(&code[from], len));
          break;
        case lexer_state::OPERATOR:
          add_token(current_state, ret, util::slice(&code[from], len), row, col);
          current_state = lexer_state::NAME; from = i; len = 0;
          break;
        default:
          abort();
      }
      ++len;
      ++col;
    }
  }
  switch (current_state) {
    case lexer_state::INIT:
      break;
    case lexer_state::NAME:
    case lexer_state::NUMBER:
    case lexer_state::OPERATOR:
      add_token(current_state, ret, util::slice(&code[from], len), row, col);
      break;
    case lexer_state::STRING:
      throw std::runtime_error("unexpected EOF in string literal");
    default:
      abort();
  }
  return ret;
}

}  // namespace reir

namespace std {
std::ostream& operator<<(ostream& o, const vector<reir::Token>& ts) {
  for (size_t i = 0; i < ts.size(); ++i) {
    if (0 < i) {
      o << ", ";
    }
    o << "{" << i << "}" << ts[i];
  }
  return o;
}
}  // namespace std


#endif  // REIR_TOKENIZE_HPP_
