//
// Created by kumagi on 18/05/16.
//

#include "reir/exec/tokenize.hpp"
#include <gtest/gtest.h>

namespace reir {

static std::string text(const Token& t) {
  return { t.text };
}

TEST(tokenizer, hello) {
  std::string s {"hello"};
  auto tokens = tokenize(s);
  ASSERT_EQ(1U, tokens.size()) << tokens;
  EXPECT_EQ(IDENTIFIER, tokens[0].type);
  EXPECT_EQ("hello", text(tokens[0]));
}

TEST(tokenizer, expr) {
  {
    std::string s {"x+2+d"};
    auto tokens = tokenize(s);
    ASSERT_EQ(5U, tokens.size()) << tokens;
    EXPECT_EQ(IDENTIFIER, tokens[0].type);
    EXPECT_EQ(PLUS, tokens[1].type);
    EXPECT_EQ(NUMBER, tokens[2].type);
    EXPECT_EQ(PLUS, tokens[3].type);
    EXPECT_EQ(IDENTIFIER, tokens[4].type);

    EXPECT_EQ("x", text(tokens[0]));
    EXPECT_EQ("2", text(tokens[2]));
    EXPECT_EQ("d", text(tokens[4]));
  }
  {
    std::string s {"(asd_+sa+d)"};
    auto tokens = tokenize(s);
    ASSERT_EQ(7U, tokens.size()) << tokens;
    EXPECT_EQ(OPEN_PAREN, tokens[0].type);
    EXPECT_EQ(IDENTIFIER, tokens[1].type);
    EXPECT_EQ(PLUS, tokens[2].type);
    EXPECT_EQ(IDENTIFIER, tokens[3].type);
    EXPECT_EQ(PLUS, tokens[4].type);
    EXPECT_EQ(IDENTIFIER, tokens[5].type);
    EXPECT_EQ(CLOSE_PAREN, tokens[6].type);

    EXPECT_EQ("asd_", text(tokens[1]));
    EXPECT_EQ("sa", text(tokens[3]));
    EXPECT_EQ("d", text(tokens[5]));
  }
  {
    std::string s {"1*2/3-4"};
    auto tokens = tokenize(s);
    ASSERT_EQ(7U, tokens.size()) << tokens;
    EXPECT_EQ(NUMBER, tokens[0].type);
    EXPECT_EQ(ASTERISK, tokens[1].type);
    EXPECT_EQ(NUMBER, tokens[2].type);
    EXPECT_EQ(SLASH, tokens[3].type);
    EXPECT_EQ(NUMBER, tokens[4].type);
    EXPECT_EQ(MINUS, tokens[5].type);
    EXPECT_EQ(NUMBER, tokens[6].type);

    EXPECT_EQ("1", text(tokens[0]));
    EXPECT_EQ("2", text(tokens[2]));
    EXPECT_EQ("3", text(tokens[4]));
    EXPECT_EQ("4", text(tokens[6]));
  }
}

TEST(tokenizer, string_literal) {
  {
    std::string s {R"("")"};
    auto tokens = tokenize(s);
    ASSERT_EQ(1U, tokens.size()) << tokens;
    EXPECT_EQ(STRING_LITERAL, tokens[0].type);
    EXPECT_EQ("", text(tokens[0]));
  }
  {
    std::string s {R"("hello")"};
    auto tokens = tokenize(s);
    ASSERT_EQ(1U, tokens.size()) << tokens;
    EXPECT_EQ(STRING_LITERAL, tokens[0].type);
    EXPECT_EQ("hello", text(tokens[0]));
  }
  {
    std::string s {R"(this is "h" a pen +"" 1)"};
    auto tokens = tokenize(s);
    ASSERT_EQ(8U, tokens.size()) << tokens;
    EXPECT_EQ(IDENTIFIER, tokens[0].type);
    EXPECT_EQ(IDENTIFIER, tokens[1].type);
    EXPECT_EQ(STRING_LITERAL, tokens[2].type);
    EXPECT_EQ(IDENTIFIER, tokens[3].type);
    EXPECT_EQ(IDENTIFIER, tokens[4].type);
    EXPECT_EQ(PLUS, tokens[5].type);
    EXPECT_EQ(STRING_LITERAL, tokens[6].type);
    EXPECT_EQ(NUMBER, tokens[7].type);

    EXPECT_EQ("this", text(tokens[0]));
    EXPECT_EQ("is", text(tokens[1]));
    EXPECT_EQ("h", text(tokens[2]));
    EXPECT_EQ("a", text(tokens[3]));
    EXPECT_EQ("pen", text(tokens[4]));
    EXPECT_EQ("", text(tokens[6]));
    EXPECT_EQ("1", text(tokens[7]));
  }
}

TEST(tokenizer, multibyte_symbols) {
  std::string s {"== != <= >= += -= *= /= %="};
  auto tokens = tokenize(s);
  ASSERT_EQ(9U, tokens.size()) << tokens;
  EXPECT_EQ(EQUAL_EQUAL, tokens[0].type) << tokens[0];
  EXPECT_EQ(EXCLAMATION_MARK_EQUAL, tokens[1].type);
  EXPECT_EQ(OPEN_ANGLE_EQUAL, tokens[2].type);
  EXPECT_EQ(CLOSE_ANGLE_EQUAL, tokens[3].type);
  EXPECT_EQ(PLUS_EQUAL, tokens[4].type);
  EXPECT_EQ(MINUS_EQUAL, tokens[5].type);
  EXPECT_EQ(ASTERISK_EQUAL, tokens[6].type);
  EXPECT_EQ(SLASH_EQUAL, tokens[7].type);
  EXPECT_EQ(PERCENT_EQUAL, tokens[8].type);
}

TEST(tokenizer, invalid_token) {
  ASSERT_THROW(tokenize("\"hello\"12dfs12"), std::runtime_error);{
  ASSERT_THROW(tokenize("1e12"), std::runtime_error);
}
}

}  // namespace reir