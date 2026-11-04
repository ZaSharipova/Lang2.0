#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace paracl {

class INode {
public:
    INode() = default;
    virtual ~INode() = default;
    INode(const INode&) = delete;
    INode& operator=(const INode&) = delete;
};

using NodePtr = std::unique_ptr<INode>;

class ValueNode : public INode {
public:
    explicit ValueNode(uint64_t value)
        : value_(value) { }
    uint64_t GetValue() const { return value_; }

private:
    uint64_t value_;
};

class VariableNode : public INode {
public:
    explicit VariableNode(std::string variable)
        : variable_(std::move(variable)) { }
    const std::string& GetVariable() const { return variable_; }

private:
    std::string variable_;
};

enum class OpType : std::uint8_t {
    kAdd,
    kSub,
    kMul,
    kDiv,
    kMod,

    kL,
    kG,
    kLE,
    kGE,
    kEq,
    kNe,
};

class OpNode : public INode {
public:
    OpNode(OpType type, NodePtr left, NodePtr right)
        : type_(type)
        , left_(std::move(left))
        , right_(std::move(right)) { }

    OpType GetType() const { return type_; }
    INode* GetLeft() const { return left_.get(); }
    INode* GetRight() const { return right_.get(); }

private:
    OpType type_;
    NodePtr left_;
    NodePtr right_;
};

class AssignNode : public INode {
public:
    AssignNode(std::string name, NodePtr expr)
        : name_(std::move(name))
        , expr_(std::move(expr)) { }

    const std::string& GetName() const { return name_; }
    INode* GetExpr() const { return expr_.get(); }

private:
    std::string name_;
    NodePtr expr_;
};

class PrintNode : public INode {
public:
    explicit PrintNode(NodePtr expr)
        : expr_(std::move(expr)) { }

    INode* GetExpr() const { return expr_.get(); }

private:
    NodePtr expr_;
};

class ReturnNode : public INode { // TODO return and print have to extend the same class
public:
    explicit ReturnNode(NodePtr expr)
        : expr_(std::move(expr)) { }

    INode* GetExpr() const { return expr_.get(); }

private:
    NodePtr expr_;
};

class ScopeNode : public INode {
public:
    void AddStatement(NodePtr stmt) { statements_.push_back(std::move(stmt)); }
    const std::vector<NodePtr>& GetStatements() const { return statements_; }

private:
    std::vector<NodePtr> statements_;
};

class IfNode : public INode {
public:
    IfNode(NodePtr cond, NodePtr body, NodePtr else_body = nullptr)
        : cond_(std::move(cond))
        , body_(std::move(body))
        , else_body_(std::move(else_body)) { }

    INode* GetCondition() const { return cond_.get(); }
    INode* GetBody() const { return body_.get(); }
    INode* GetElse() const { return else_body_.get(); }

private:
    NodePtr cond_;
    NodePtr body_;
    NodePtr else_body_;
};

class WhileNode : public INode {
public:
    WhileNode(NodePtr cond, NodePtr body)
        : cond_(std::move(cond))
        , body_(std::move(body)) { }

    INode* GetCondition() const { return cond_.get(); }
    INode* GetBody() const { return body_.get(); }

private:
    NodePtr cond_;
    NodePtr body_;
};

} // namespace paracl
