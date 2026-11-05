#pragma once

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include "lexer.hpp"
#include "node.hpp"

/* G          ::= FUNCTION_D+
   FUNCTION_D ::= V "(" [ E {"," E}* ] ")" "{" OP* "}"

   OP         ::= RETURN | PRINTF | SCANF | WHILE | IF | CALL;

   RETURN     ::= "return" E ";"
   PRINTF     ::= ("write" | "write_char") "(" (V | N) ")" ";"
   CALL       ::= V "(" E [ E {"," E}* ] ")" ";"
   SCANF      ::= "read" "(" S ")" ";" пока скип
   WHILE      ::= "while" "(" CONDITION ")" BODY +
   IF         ::= "if" "(" CONDITION ")" BODY [ "else" BODY ] +
   CONDITION  ::= E [ ("<=" | "<" | ">=" | ">" | "==" | "!=") E ]
   BODY       ::= "{" OP* "}"

   E          ::= T ([+-] T)* +
   T          ::= P ([*:/] P)* +
   P          ::= IntLiteral | Identifier | '(' E ')' +

   V          ::= Variable +
   N          ::= Number +
*/


namespace paracl::parser {

using NodePtr = std::unique_ptr<INode>;

class Parser {
public:
    Parser(const lexer::Lexer* lexer, std::vector<lexer::Token> tokens)
        : lexer_(lexer)
        , tokens_(std::move(tokens)) { }

    NodePtr Parse();

private:
    NodePtr GetFunctionDeclare();
    NodePtr GetFunctionCall();
    NodePtr GetWhile();
    NodePtr GetIf();
    NodePtr GetElse();
    NodePtr GetCondition();
    NodePtr GetBody();
    NodePtr GetExpression();
    NodePtr GetTerm();
    NodePtr GetPrimary();
    NodePtr GetIdentifier();
    NodePtr GetIntLiteral();
    NodePtr GetReturn();
    NodePtr GetPrint();
    NodePtr GetOp();
    NodePtr GetAssign();

    static bool IsComparisonSign(lexer::Token token);
    static OpType ToOpType(lexer::Token token);

    lexer::Token GetToken() const;
    lexer::Token SeeNextToken() const;
    lexer::Token GetNextToken();
    bool IsThatTokenType(lexer::TokenType type);
    void IncreaseTokensPos();

    const lexer::Lexer* lexer_ = nullptr;
    std::vector<lexer::Token> tokens_;
    std::size_t pos_ = 0;
};

} // namespace paracl::parser
