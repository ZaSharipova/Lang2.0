#pragma once

#include <cstdint>
#include <vector>

namespace paracl::lexer {

enum class TokenType : uint32_t {
    kKeyWordIf = 0,
    kKeyWordElse,
    kKeyWordWhile,
    kKeyWordReturn,
    kKeyWordPrint,
    
    kEof,
    kError,

    kIdentifier,
    kIntLiteral,
    
    kQuestion,

    kPlus,
    kMinus,
    kStar,
    kSlash,
    kPercent,

    kEqual,
    kEqualEqual,
    kNotEqual,
    kLess,
    kGreat,
    kLessEq,
    kGreatEq,

    kLParen,
    kRParen,
    kLBrace,
    kRBrace,

    kSemicolon,
};

struct Token {
    TokenType type;
    uint32_t offset;
    uint32_t len;
};

class SourceBuffer {
public:
    SourceBuffer() = delete;

    static SourceBuffer FromFile(const char* filepath);
    static SourceBuffer FromString(std::string& str);
    
    const char* data() const { return buffer_.data(); }
    uint32_t size() const { return buffer_.size() - 1; }

private:
    std::vector<char> buffer_;
};

class Lexer {
 public:
  Lexer(const SourceBuffer& source)
   : source_{source}, 
     base_{source.data()}, 
     cursor_{source.data()} {}

  std::vector<Token> TokenizeAll();

 private:
  void SkipWhitespaceAndComments();
  Token LexIdentifierOrKeyword();
  Token LexNumber();
  Token LexPunctuation();

  const char* base_ = nullptr;
  const char* cursor_ = nullptr;
};

} // namespace paracl::lexer
