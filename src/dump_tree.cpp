#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>

#include "../include/node.hpp"

namespace paracl {

namespace {

struct Style {
    const char* fill;
    const char* border;
};

constexpr Style g_kArith { .fill = "#d6e9ff", .border = "#3b82c4" };
constexpr Style g_kCompare { .fill = "#ffe6c2", .border = "#d9822b" };
constexpr Style g_kControl { .fill = "#e6dbff", .border = "#7c4dcc" };
constexpr Style g_kStmt { .fill = "#d8f5e0", .border = "#2e9a5b" };
constexpr Style g_kNumber { .fill = "#ffffff", .border = "#666666" };
constexpr Style g_kVar { .fill = "#fff4b8", .border = "#c4a000" };
constexpr Style g_kUnknown { .fill = "#ffb3b3", .border = "#cc0000" };

struct OpDescription {
    OpType type;
    const char* name;
    const Style& style;
};

constexpr OpDescription g_kOps[] = {
    { .type = OpType::kAdd, .name = "+", .style = g_kArith },
    { .type = OpType::kSub, .name = "-", .style = g_kArith },
    { .type = OpType::kMul, .name = "*", .style = g_kArith },
    { .type = OpType::kDiv, .name = "/", .style = g_kArith },
    { .type = OpType::kMod, .name = "%", .style = g_kArith },
    { .type = OpType::kL, .name = "<", .style = g_kCompare },
    { .type = OpType::kG, .name = ">", .style = g_kCompare },
    { .type = OpType::kLE, .name = "<=", .style = g_kCompare },
    { .type = OpType::kGE, .name = ">=", .style = g_kCompare },
    { .type = OpType::kEq, .name = "==", .style = g_kCompare },
    { .type = OpType::kNe, .name = "!=", .style = g_kCompare },
};

constexpr OpDescription g_kUnknownOp { .type = OpType { }, .name = "?", .style = g_kUnknown };

const OpDescription& OpInfo(OpType type) {
    const auto* iter = std::ranges::find_if(std::begin(g_kOps), std::end(g_kOps),
        [type](const OpDescription& op_desc) {
            return op_desc.type == type;
        });

    return iter != std::end(g_kOps) ? *iter : g_kUnknownOp;
}

std::string RecordEscape(const std::string& str) {
    constexpr std::string_view kSpecial = "{}|<>\"\\";

    std::string res;
    res.reserve(str.size());
    for (const char symb : str) {
        if (kSpecial.find(symb) != std::string_view::npos) {
            res += '\\';
        }

        res += symb;
    }

    return res;
}

std::string NodeId(const INode* node) {
    std::ostringstream sstream;
    sstream << 'n' << static_cast<const void*>(node);
    return sstream.str();
}

class DotDumper final : public IVisitor {
public:
    explicit DotDumper(std::ostream& out)
        : out_(out) { }

    void Run(const INode* root) {
        out_ << R"(digraph AST {
    rankdir=TB;
    bgcolor="#fafafa";
    nodesep=0.4;
    ranksep=0.5;
    node [fontname="Helvetica", fontsize=12, penwidth=1.5];
    edge [fontname="Helvetica", fontsize=10, color="#888888",
          fontcolor="#555555", arrowsize=0.7];

    )";

        if (root) {
            root->Accept(*this);
        }

        out_ << "}\n";
    }

    void Visit(const ValueNode& node) override {
        Emit(node, "num", std::to_string(node.GetValue()), g_kNumber);
    }

    void Visit(const VariableNode& node) override {
        Emit(node, "var", node.GetVariable(), g_kVar);
    }

    void Visit(const OpNode& node) override {
        const OpDescription& op_desc = OpInfo(node.GetType());
        Emit(node, "op", op_desc.name, op_desc.style);
        Link(node, node.GetLeft(), "L");
        Link(node, node.GetRight(), "R");
    }

    void Visit(const AssignNode& node) override {
        Emit(node, "assign", node.GetName() + " =", g_kStmt);
        Link(node, node.GetExpr());
    }

    void Visit(const PrintNode& node) override {
        Emit(node, "stmt", "print", g_kStmt);
        Link(node, node.GetExpr());
    }

    void Visit(const ReturnNode& node) override {
        Emit(node, "stmt", "return", g_kStmt);
        Link(node, node.GetExpr());
    }

    void Visit(const ProgramNode& node) override {
        Emit(node, "program", "", g_kControl);
        std::size_t index = 1;
        for (const auto& func : node.GetFunctions()) {
            Link(node, func.get(), std::to_string(index++));
        }
    }

    void Visit(const ScopeNode& node) override {
        Emit(node, "scope", "{ }", g_kStmt);
        std::size_t index = 1;
        for (const NodePtr& stmt : node.GetStatements()) {
            Link(node, stmt.get(), std::to_string(index++));
        }
    }

    void Visit(const FunctionNode& node) override {
        std::string signature = node.GetName() + '(';
        bool first = true;
        for (const std::string& param : node.GetParams()) {
            if (!first) {
                signature += ", ";
            }

            signature += param;
            first = false;
        }

        signature += ')';

        Emit(node, "function", signature, g_kControl);
        Link(node, node.GetBody(), "body");
    }

    void Visit(const CallNode& node) override {
        Emit(node, "call", node.GetName(), g_kStmt);
        std::size_t index = 1;
        for (const NodePtr& arg : node.GetArgs()) {
            Link(node, arg.get(), "arg " + std::to_string(index++));
        }
    }

    void Visit(const IfNode& node) override {
        Emit(node, "control", "if", g_kControl);
        Link(node, node.GetCondition(), "cond");
        Link(node, node.GetBody(), "then");
        Link(node, node.GetElse(), "else");
    }

    void Visit(const WhileNode& node) override {
        Emit(node, "control", "while", g_kControl);
        Link(node, node.GetCondition(), "cond");
        Link(node, node.GetBody(), "body");
    }

private:
    void Emit(const INode& node, const std::string& kind, const std::string& text,
        const Style& style) {
        out_ << "    " << NodeId(&node)
             << " [label=\"{" << RecordEscape(kind)
             << " | " << RecordEscape(text) << "}\""
             << ", shape=Mrecord, style=filled"
             << ", fillcolor=\"" << style.fill << '"'
             << ", color=\"" << style.border << "\"];\n";
    }

    void Link(const INode& parent, const INode* child,
        std::string_view label = { }) {
        if (!child) {
            return;
        }

        out_ << "    " << NodeId(&parent) << " -> " << NodeId(child);
        if (!label.empty()) {
            out_ << " [label=\" " << label << "\"]";
        }
        out_ << ";\n";

        child->Accept(*this);
    }

    std::ostream& out_;
};

void DumpTreeDot(const INode* root, std::ostream& out) {
    DotDumper(out).Run(root);
}

} // namespace

bool DumpTreeToImage(const INode* root, const std::string& dot_path,
    const std::string& image_path) {
    {
        std::ofstream dot(dot_path);
        if (!dot) {
            std::cerr << "Cannot open DOT file for writing: " << dot_path << '\n';
            return false;
        }

        DumpTreeDot(root, dot);
    }

    const std::string cmd = "dot \"" + dot_path + "\" -T png -o \"" + image_path + "\"";

    return std::system(cmd.c_str()) == 0;
}

} // namespace paracl
