#include "../include/parser.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include "../include/lexer.hpp"
#include "../include/node.hpp"

/* G          ::= FUNCTION_D+
   FUNCTION_D ::= "declare" V "(" [ E {"," E}* ] ")" "{" OP* "}" TODO пока нет этого, непонятна точка входа

   OP         ::= RETURN | PRINTF | SCANF | WHILE | IF

   RETURN     ::= "return" E ";" +
   PRINTF     ::= ("write" | "write_char") "(" (V | N) ")" ";" +
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

NodePtr Parser::Parse() {
    auto root = std::make_unique<ScopeNode>();

    while (GetToken().type != lexer::TokenType::kEof) {
        NodePtr op = GetOp();
        if (!op) {
            return nullptr;
        }

        root->AddStatement(std::move(op));
    }

    return root;
}

NodePtr Parser::GetWhile() { // NOLINT(misc-no-recursion)
    if (!IsThatTokenType(lexer::TokenType::kKeyWordWhile)) {
        return nullptr;
    }

    NodePtr condition = GetCondition();
    if (!condition) {
        return nullptr;
    }

    NodePtr body = GetBody();
    if (!body) {
        return nullptr;
    }

    return std::make_unique<WhileNode>(std::move(condition), std::move(body));
}

NodePtr Parser::GetIf() { // NOLINT(misc-no-recursion)
    if (!IsThatTokenType(lexer::TokenType::kKeyWordIf)) {
        return nullptr;
    }

    NodePtr condition = GetCondition();
    if (!condition) {
        return nullptr;
    }

    NodePtr body = GetBody();
    if (!body) {
        return nullptr;
    }

    NodePtr else_body = GetElse();
    if (!else_body) { // std::optional is needed here
        return std::make_unique<IfNode>(std::move(condition), std::move(body));
    }

    return std::make_unique<IfNode>(std::move(condition), std::move(body), std::move(else_body));
}

NodePtr Parser::GetElse() { // NOLINT(misc-no-recursion)
    if (!IsThatTokenType(lexer::TokenType::kKeyWordElse)) {
        return nullptr;
    }

    NodePtr body = GetBody();
    if (!body) {
        return nullptr;
    }

    return body;
}

NodePtr Parser::GetCondition() {
    if (!IsThatTokenType(lexer::TokenType::kLParen)) {
        std::cerr << "Error while parsing if, as there's no '(' in condition\n";
        return nullptr;
    }

    NodePtr left = GetExpression();
    if (!left) {
        std::cerr << "Error while parsing if, as there's no correct condition\n";
        return nullptr;
    }

    const lexer::Token sign = GetToken();
    if (IsComparisonSign(sign)) {
        IncreaseTokensPos();
        NodePtr right = GetExpression();
        if (!right) {
            std::cerr << "Error while parsing if, as there's no correct condition after comparison sign\n";
            return nullptr;
        }

        left = std::make_unique<OpNode>(ToOpType(sign), std::move(left), std::move(right));
    }

    if (!IsThatTokenType(lexer::TokenType::kRParen)) {
        std::cerr << "Error while parsing if, as there's no ')' in condition\n";
        return nullptr;
    }

    return left;
}

NodePtr Parser::GetBody() { // NOLINT(misc-no-recursion)
    if (!IsThatTokenType(lexer::TokenType::kLBrace)) {
        std::cerr << "Error while parsing if, as there's no '(' in condition\n";
        return nullptr;
    }

    auto scope = std::make_unique<ScopeNode>();
    while (!IsThatTokenType(lexer::TokenType::kRBrace)) {
        NodePtr body = GetOp();
        if (!body) {
            // std::cerr << "Error while parsing, body of if or while is incorrect\n";
            return nullptr;
        }

        scope->AddStatement(std::move(body));
    }

    return scope;
}

NodePtr Parser::GetExpression() { // NOLINT(misc-no-recursion)
    NodePtr left = GetTerm();
    if (!left) {
        return nullptr;
    }

    lexer::Token op_token = GetToken();
    while (op_token.type == lexer::TokenType::kPlus || op_token.type == lexer::TokenType::kMinus) {
        const OpType op_node_type = ToOpType(op_token);
        IncreaseTokensPos();
        NodePtr right = GetTerm();
        if (!right) {
            std::cerr << "There is no right part in arithmetical expression\n"; // TODO like there's a mistake in tokens_.pos_
            return nullptr;
        }

        left = std::make_unique<OpNode>(op_node_type, std::move(left), std::move(right));
        op_token = GetToken();
    }

    return left;
}

NodePtr Parser::GetTerm() { // NOLINT(misc-no-recursion)
    NodePtr left = GetPrimary();
    if (!left) {
        return nullptr;
    }

    lexer::Token op_token = GetToken();
    while (op_token.type == lexer::TokenType::kStar || op_token.type == lexer::TokenType::kSlash) {
        const OpType op_node_type = ToOpType(op_token);
        IncreaseTokensPos();
        NodePtr right = GetPrimary();
        if (!right) {
            std::cerr << "There is no right part in arithmetical expression\n"; // TODO like there's a mistake in tokens_.pos_
            return nullptr;
        }

        left = std::make_unique<OpNode>(op_node_type, std::move(left), std::move(right));
        op_token = GetToken();
    }

    return left;
}

NodePtr Parser::GetPrimary() { // NOLINT(misc-no-recursion)
    if (IsThatTokenType(lexer::TokenType::kLParen)) {
        NodePtr expr = GetExpression();
        if (!expr) {
            std::cerr << "Error while getting primary expression: there is no appropriate expression\n";
            return nullptr;
        }

        if (IsThatTokenType(lexer::TokenType::kRParen)) {
            return expr;
        }

        std::cerr << "Error while parsing expr in parens, there is no ')'\n";
        return nullptr;
    }

    NodePtr variable = GetIdentifier();
    if (variable) {
        return variable;
    }

    NodePtr intliteral = GetIntLiteral();
    return intliteral;
}

NodePtr Parser::GetIdentifier() {
    const lexer::Token token = GetToken();
    if (IsThatTokenType(lexer::TokenType::kIdentifier)) {
        return std::make_unique<VariableNode>(lexer_->TokenToStr(token));
    }

    return nullptr;
}

NodePtr Parser::GetIntLiteral() {
    const lexer::Token token = GetToken();
    if (IsThatTokenType(lexer::TokenType::kIntLiteral)) {
        return std::make_unique<ValueNode>(lexer_->TokenToNum(token));
    }

    return nullptr;
}

NodePtr Parser::GetReturn() {
    if (!IsThatTokenType(lexer::TokenType::kKeyWordReturn)) {
        return nullptr;
    }

    NodePtr expr = GetExpression();
    if (!expr) {
        return nullptr;
    }

    if (!IsThatTokenType(lexer::TokenType::kSemicolon)) {
        std::cerr << "Error: there is no ';'\n";
        return nullptr;
    }

    return std::make_unique<ReturnNode>(std::move(expr));
}

NodePtr Parser::GetPrint() {
    if (!IsThatTokenType(lexer::TokenType::kKeyWordPrint)) {
        return nullptr;
    }

    if (!IsThatTokenType(lexer::TokenType::kLParen)) {
        std::cerr << "Error while parsing print: there is no '('\n"; // TODO
        return nullptr;
    }

    NodePtr expr = GetExpression();
    if (!expr) {
        std::cerr << "Error while parsing expression in print\n";
        return nullptr;
    }

    if (!IsThatTokenType(lexer::TokenType::kRParen)) {
        std::cerr << "Error while parsing print: there is no ')'\n";
        return nullptr;
    }

    if (!IsThatTokenType(lexer::TokenType::kSemicolon)) {
        std::cerr << "Error: there is no ';'\n";
        return nullptr;
    }

    return std::make_unique<PrintNode>(std::move(expr));
}

NodePtr Parser::GetOp() { // NOLINT(misc-no-recursion)
    switch (GetToken().type) {
    case lexer::TokenType::kKeyWordReturn:
        return GetReturn();

    case lexer::TokenType::kKeyWordPrint:
        return GetPrint();

    case lexer::TokenType::kKeyWordWhile:
        return GetWhile();

    case lexer::TokenType::kKeyWordIf:
        return GetIf();

    case lexer::TokenType::kIdentifier:
        return GetAssign();

        // case lexer::TokenType::kKeyWordScan: // TODO пока скип
        //     return GetScan();

    default:
        std::cerr << "Error while parsing operation: unexpected token\n";
        return nullptr;
    }
}

NodePtr Parser::GetAssign() {
    const lexer::Token name = GetToken();
    if (!IsThatTokenType(lexer::TokenType::kIdentifier)) {
        return nullptr;
    }
    if (!IsThatTokenType(lexer::TokenType::kEqual)) {
        std::cerr << "Error while parsing assignment: there is no '='\n";
        return nullptr;
    }
    NodePtr expr = GetExpression();
    if (!expr) {
        return nullptr;
    }
    if (!IsThatTokenType(lexer::TokenType::kSemicolon)) {
        std::cerr << "Error while parsing assignment: there is no ';'\n";
        return nullptr;
    }
    return std::make_unique<AssignNode>(lexer_->TokenToStr(name), std::move(expr));
}

bool Parser::IsComparisonSign(const lexer::Token token) {
    return (token.type == lexer::TokenType::kEqualEqual
        || token.type == lexer::TokenType::kNotEqual
        || token.type == lexer::TokenType::kLess
        || token.type == lexer::TokenType::kGreat
        || token.type == lexer::TokenType::kLessEq
        || token.type == lexer::TokenType::kGreatEq);
}

lexer::Token Parser::GetToken() const {
    return tokens_[pos_];
}

lexer::Token Parser::GetNextToken() {
    return tokens_[pos_++];
}

bool Parser::IsThatTokenType(lexer::TokenType type) {
    if (!(GetToken().type == type)) {
        return false;
    }

    pos_++;
    return true;
}

void Parser::IncreaseTokensPos() {
    assert(pos_ <= tokens_.size());
    pos_++;
}

OpType Parser::ToOpType(lexer::Token token) {
    switch (token.type) {
    case lexer::TokenType::kPlus:
        return OpType::kAdd;
    case lexer::TokenType::kMinus:
        return OpType::kSub;
    case lexer::TokenType::kStar:
        return OpType::kMul;
    case lexer::TokenType::kSlash:
        return OpType::kDiv;
    case lexer::TokenType::kPercent:
        return OpType::kMod;

    case lexer::TokenType::kEqualEqual:
        return OpType::kEq;
    case lexer::TokenType::kNotEqual:
        return OpType::kNe;
    case lexer::TokenType::kLess:
        return OpType::kL;
    case lexer::TokenType::kLessEq:
        return OpType::kLE;
    case lexer::TokenType::kGreat:
        return OpType::kG;
    case lexer::TokenType::kGreatEq:
        return OpType::kGE;

    default:
        throw std::runtime_error("Error while getting OpType\n");
    }
}

} // namespace paracl::parser
