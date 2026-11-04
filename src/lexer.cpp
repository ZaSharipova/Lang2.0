#include "lexer.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

namespace paracl::lexer {

static bool IsSpace(char c) {
    return (c == ' ' || c == '\n' || c == '\t');
}

static bool IsDigit(char c) {
    return ('0' <= c && c <= '9');
}

static bool IsAlpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

static bool IsAlnum(char c) {
    return IsAlpha(c) || IsDigit(c);
}

SourceBuffer SourceBuffer::FromFile(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("file didnt open");
    }

    std::stringstream stringBuffer;
    stringBuffer << file.rdbuf();
    std::string text = stringBuffer.str();

    return SourceBuffer::FromString(text);
}

SourceBuffer SourceBuffer::FromString(const std::string& str) {
    std::vector<char> buffer(str.begin(), str.end());
    buffer.push_back('\0');

    SourceBuffer sb(buffer);
    return sb;
}

int64_t Lexer::TokenToNum(Token token) {
    return std::stoll(TokenToStr(token));
}

std::string Lexer::TokenToStr(Token token) {
    std::string str(buffer_.data() + token.offset, token.len);
    return str;
}

void Lexer::SkipUselessText() {
    while (buffer_[cursor_] != '\0') {
        if (IsSpace(buffer_[cursor_])) {
            cursor_++;
            continue;
        }

        if (buffer_[cursor_] == '/' && buffer_[cursor_ + 1] == '/') {
            cursor_ += 2;
            while (buffer_[cursor_] != '\n' && buffer_[cursor_] != '\0') {
                cursor_++;
            }
            continue;
        }

        if (buffer_[cursor_] == '/' && buffer_[cursor_ + 1] == '*') {
            cursor_ += 2;
            while (buffer_[cursor_] != '\0') {
                if (buffer_[cursor_] == '*' && buffer_[cursor_ + 1] == '/') {
                    cursor_ += 2;
                    break;
                }
                cursor_++;
            }
            continue;
        }

        break;
    }
}

Token Lexer::NextToken() {
    SkipUselessText();
    
    if (buffer_[cursor_] == '\0') {
        return Token{TokenType::kEof, static_cast<uint32_t>(cursor_), 0};
    }

    if (IsAlpha(buffer_[cursor_])) {
        return LexIdentifierOrKeyword();
    }

    if (IsDigit(buffer_[cursor_])) {
        return LexNumber();
    }

    return LexPunctuation();
}

Token Lexer::LexIdentifierOrKeyword() {
    std::size_t start = cursor_;
    while (IsAlnum(buffer_[cursor_])) {
        cursor_++;
    }

    uint32_t offset = static_cast<uint32_t>(start);
    uint32_t len = static_cast<uint32_t>(cursor_ - start);

    std::string identifier(buffer_.data() + offset, len);
    TokenType type = TokenType::kIdentifier;
    if (identifier == "if") {
        type = TokenType::kKeyWordIf;
    } else if (identifier == "else") {
        type = TokenType::kKeyWordElse;
    } else if (identifier == "while") {
        type = TokenType::kKeyWordWhile;
    } else if (identifier == "return") {
        type = TokenType::kKeyWordReturn;
    } else if (identifier == "print") {
        type = TokenType::kKeyWordPrint;
    }
    
    return Token{type, offset, len};
}

Token Lexer::LexNumber() {
    std::size_t start = cursor_;
    while (IsDigit(buffer_[cursor_])) {
        cursor_++;
    }

    uint32_t offset = static_cast<uint32_t>(start);
    uint32_t len = static_cast<uint32_t>(cursor_ - start);

    return Token{TokenType::kIntLiteral, offset, len};
}

Token Lexer::LexPunctuation() {
    uint32_t offset = cursor_;
    uint32_t len = 1;
    TokenType type = TokenType::kError;

    char c = buffer_[cursor_++];
    switch (c) {
        case '?': type = TokenType::kQuestion; break;
        case '+': type = TokenType::kPlus; break;
        case '-': type = TokenType::kMinus; break;
        case '*': type = TokenType::kStar; break;
        case '/': type = TokenType::kSlash; break;
        case '%': type = TokenType::kPercent; break;
        case '(': type = TokenType::kLParen; break;
        case ')': type = TokenType::kRParen; break;
        case '{': type = TokenType::kLBrace; break;
        case '}': type = TokenType::kRBrace; break;
        case ';': type = TokenType::kSemicolon; break;

        case '=': 
            if (buffer_[cursor_] == '=') {
                type = TokenType::kEqualEqual;
                len = 2;
                cursor_++;
                break;
            }
            type = TokenType::kEqual;
            break;
        
        case '!': 
            if (buffer_[cursor_] == '=') {
                type = TokenType::kNotEqual;
                len = 2;
                cursor_++;
                break;
            }
            type = TokenType::kError;
            break;

        case '<':
            if (buffer_[cursor_] == '=') {
                type = TokenType::kLessEq;
                len = 2;
                cursor_++;
                break;
            }
            type = TokenType::kLess;
            break;

        case '>':
            if (buffer_[cursor_] == '=') {
                type = TokenType::kGreatEq;
                len = 2;
                cursor_++;
                break;
            }
            type = TokenType::kGreat;
            break;

        default: break;
    }

    return Token{type, offset, len};
}

std::vector<Token> Lexer::TokenizeAll() {
    std::vector<Token> res;
    while (true) {
        Token tok = NextToken();
        res.push_back(tok);
        if (tok.type == TokenType::kEof) {
            break;
        }
    }

    return res;
}

} // namespace paracl::lexer