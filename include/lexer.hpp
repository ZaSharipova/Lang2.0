#pragma once

#include <cstdint>
#include <vector>
#include <string>

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
    static SourceBuffer FromString(const std::string& str);
    
    const char* data() const { return buffer_.data(); }
    uint32_t size() const { return buffer_.size() - 1; }
    
    const char& operator[](std::size_t index) const { return buffer_[index]; }
    char& operator[](std::size_t index) { return buffer_[index]; }

    std::pair<std::size_t, std::size_t> GetLocation(uint32_t offset) const {
        if (newLines_.empty()) {
            return {1, 1};
        }

        auto it = std::upper_bound(newLines_.begin(), newLines_.end(), offset);
        std::size_t lineIdx = std::distance(newLines_.begin(), it);
        
        return {lineIdx, offset - newLines_[lineIdx - 1] + 1};
    }
    
private:
    SourceBuffer(const std::vector<char>& buffer) : buffer_{buffer} {}

    void BuildNewLines() {
        newLines_.clear();
        newLines_.push_back(0);

        for (std::size_t i = 0; i < buffer_.size(); ++i) {
            if (buffer_[i] == '\n') {
                newLines_.push_back(i + 1);
            }
        }
    }

    std::vector<char> buffer_;
    std::vector<uint32_t> newLines_;
};

class Lexer {
 public:
    int64_t TokenToNum(Token token);
    std::string TokenToStr(Token token);
 
 Lexer(const SourceBuffer& source)
    : buffer_{source} {}

  std::vector<Token> TokenizeAll();

private:
    void SkipUselessText();
    Token NextToken();
    Token LexIdentifierOrKeyword();
    Token LexNumber();
    Token LexPunctuation();

  const SourceBuffer& buffer_;
  std::size_t cursor_ = 0;
};

} // namespace paracl::lexer
