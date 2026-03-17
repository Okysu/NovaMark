#include <gtest/gtest.h>
#include "nova/lexer/lexer.h"
#include "nova/core/string_utils.h"

using namespace nova;

// 测试 UString 基本功能
TEST(UStringTest, AsciiLength) {
    UString str("hello");
    EXPECT_EQ(str.length(), 5);
}

TEST(UStringTest, ChineseLength) {
    UString str("你好世界");
    EXPECT_EQ(str.length(), 4);
}

TEST(UStringTest, MixedLength) {
    UString str("hello你好");
    EXPECT_EQ(str.length(), 7);
}

TEST(UStringTest, IsChinese) {
    EXPECT_TRUE(UString::is_chinese(UString("中")[0]));
    EXPECT_FALSE(UString::is_chinese(UString("a")[0]));
}

TEST(UStringTest, IsIdentStart) {
    EXPECT_TRUE(UString::is_ident_start(UString("a")[0]));
    EXPECT_TRUE(UString::is_ident_start(UString("_")[0]));
    EXPECT_TRUE(UString::is_ident_start(UString("中")[0]));
    EXPECT_FALSE(UString::is_ident_start(UString("1")[0]));
}

// 测试 Lexer 基本功能
class LexerTest : public ::testing::Test {
protected:
    Result<std::vector<Token>> tokenize(const std::string& source) {
        return Lexer(source, "<test>").tokenize();
    }
};

TEST_F(LexerTest, EmptyInput) {
    auto result = tokenize("");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::Eof);
}

TEST_F(LexerTest, SingleIdentifier) {
    auto result = tokenize("hello");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 2);  // identifier + eof
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(tokens[0].value, "hello");
}

TEST_F(LexerTest, ChineseIdentifier) {
    auto result = tokenize("林晓");
    ASSERT_TRUE(result.is_ok()) << result.error().to_string();
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(tokens[0].value, "林晓");
}

TEST_F(LexerTest, NumberLiteral) {
    auto result = tokenize("123");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::NumberLiteral);
    EXPECT_DOUBLE_EQ(std::stod(tokens[0].value), 123.0);
}

TEST_F(LexerTest, FloatLiteral) {
    auto result = tokenize("3.14");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::NumberLiteral);
    EXPECT_DOUBLE_EQ(std::stod(tokens[0].value), 3.14);
}

TEST_F(LexerTest, StringLiteral) {
    auto result = tokenize("\"hello world\"");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].value, "hello world");
}

TEST_F(LexerTest, StringWithEscape) {
    auto result = tokenize("\"hello\\nworld\"");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].value, "hello\nworld");
}

TEST_F(LexerTest, Operators) {
    auto result = tokenize("+-*/==!=");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TokenType::Plus);
    EXPECT_EQ(tokens[1].type, TokenType::Minus);
    EXPECT_EQ(tokens[2].type, TokenType::Asterisk);
    EXPECT_EQ(tokens[3].type, TokenType::Slash);
    EXPECT_EQ(tokens[4].type, TokenType::EqualEqual);
    EXPECT_EQ(tokens[5].type, TokenType::BangEqual);
}

TEST_F(LexerTest, Arrow) {
    auto result = tokenize("->");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::Arrow);
}

TEST_F(LexerTest, AtSign) {
    auto result = tokenize("@var");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::AtSign);
    // @var 解析为 @ 后跟 identifier
    EXPECT_EQ(tokens[1].type, TokenType::Identifier);
    EXPECT_EQ(tokens[1].value, "var");
}

TEST_F(LexerTest, Hash) {
    auto result = tokenize("#scene_forest");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::Hash);
    // #scene_forest 解析为 # 后跟 identifier
    EXPECT_EQ(tokens[1].type, TokenType::Identifier);
    EXPECT_EQ(tokens[1].value, "scene_forest");
}

TEST_F(LexerTest, Keywords) {
    auto result = tokenize("if endif else and or not");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_EQ(tokens[0].type, TokenType::KwIf);
    EXPECT_EQ(tokens[1].type, TokenType::KwEndif);
    EXPECT_EQ(tokens[2].type, TokenType::KwElse);
    EXPECT_EQ(tokens[3].type, TokenType::KwAnd);
    EXPECT_EQ(tokens[4].type, TokenType::KwOr);
    EXPECT_EQ(tokens[5].type, TokenType::KwNot);
}

TEST_F(LexerTest, UnterminatedString) {
    auto result = tokenize("\"unclosed");
    EXPECT_TRUE(result.is_err());
}

TEST_F(LexerTest, Colon) {
    auto result = tokenize(":");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::Colon);
}

TEST_F(LexerTest, Newline) {
    auto result = tokenize("a\nb");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    // a, newline, b, eof
    EXPECT_GE(tokens.size(), 4);
    EXPECT_EQ(tokens[1].type, TokenType::Newline);
}

TEST_F(LexerTest, SkipComment) {
    auto result = tokenize("a // comment\nb");
    ASSERT_TRUE(result.is_ok());
    auto tokens = result.unwrap();
    // a, newline, b, eof
    EXPECT_GE(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(tokens[2].type, TokenType::Identifier);
}

// 参数化测试：单字符 Token
class SingleTokenTest : public ::testing::TestWithParam<std::pair<std::string, TokenType>> {};

TEST_P(SingleTokenTest, RecognizeToken) {
    auto [input, expected] = GetParam();
    auto result = Lexer(input, "<test>").tokenize();
    ASSERT_TRUE(result.is_ok()) << "Lexer failed on: " << input;
    auto tokens = result.unwrap();
    EXPECT_GE(tokens.size(), 1) << "No tokens for: " << input;
    EXPECT_EQ(tokens[0].type, expected) << "Expected " << token_type_str(expected) 
                                        << " but got " << token_type_str(tokens[0].type);
}

INSTANTIATE_TEST_SUITE_P(
    LexerTokens,
    SingleTokenTest,
    ::testing::Values(
        std::make_pair("(", TokenType::LeftParen),
        std::make_pair(")", TokenType::RightParen),
        std::make_pair("[", TokenType::LeftBracket),
        std::make_pair("]", TokenType::RightBracket),
        std::make_pair("{", TokenType::LeftBrace),
        std::make_pair("}", TokenType::RightBrace),
        std::make_pair(",", TokenType::Comma),
        std::make_pair(".", TokenType::Dot),
        std::make_pair("+", TokenType::Plus),
        std::make_pair("-", TokenType::Minus),
        std::make_pair("*", TokenType::Asterisk),
        std::make_pair("/", TokenType::Slash),
        std::make_pair("%", TokenType::Percent),
        std::make_pair("=", TokenType::Equals),
        std::make_pair("<", TokenType::Less),
        std::make_pair(">", TokenType::Greater),
        std::make_pair("==", TokenType::EqualEqual),
        std::make_pair("!=", TokenType::BangEqual),
        std::make_pair("<=", TokenType::LessEqual),
        std::make_pair(">=", TokenType::GreaterEqual),
        std::make_pair("->", TokenType::Arrow),
        std::make_pair(":", TokenType::Colon)
    )
);
