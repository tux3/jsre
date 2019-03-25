#include "dot.hpp"
#include "ast/ast.hpp"
#include "analyze/astqueries.hpp"
#include "graph/graph.hpp"
#include <cstring>

using namespace std;

template <class T>
std::string valueStr(T v)
{
    return " [" + to_string(v) + "]";
}

template <>
std::string valueStr<string>(string v)
{
    /// FIXME: Truncating at byte boundaries is not UTF8 friendly!
    if (v.size() > 16)
        v = v.substr(0, 13) + "...";
    return " [\\\"" + v + "\\\"]";
}

template <>
std::string valueStr<bool>(bool v)
{
    return v ? " [true]" : " [false]";
}

template <>
std::string valueStr<double>(double v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), " [%g]", v);
    return buf;
}

template <>
std::string valueStr<BinaryExpression::Operator>(BinaryExpression::Operator v)
{
    switch (v) {
    case BinaryExpression::Operator::Equal:
        return "==";
    case BinaryExpression::Operator::NotEqual:
        return "!=";
    case BinaryExpression::Operator::StrictEqual:
        return "===";
    case BinaryExpression::Operator::StrictNotEqual:
        return "!==";
    case BinaryExpression::Operator::Lesser:
        return "<";
    case BinaryExpression::Operator::LesserOrEqual:
        return "<=";
    case BinaryExpression::Operator::Greater:
        return ">";
    case BinaryExpression::Operator::GreaterOrEqual:
        return ">=";
    case BinaryExpression::Operator::ShiftLeft:
        return "<<";
    case BinaryExpression::Operator::SignShiftRight:
        return ">>";
    case BinaryExpression::Operator::ZeroingShiftRight:
        return ">>>";
    case BinaryExpression::Operator::Plus:
        return "+";
    case BinaryExpression::Operator::Minus:
        return "-";
    case BinaryExpression::Operator::Times:
        return "*";
    case BinaryExpression::Operator::Division:
        return "/";
    case BinaryExpression::Operator::Modulo:
        return "%";
    case BinaryExpression::Operator::Exponentiation:
        return "**";
    case BinaryExpression::Operator::BitwiseOr:
        return "|";
    case BinaryExpression::Operator::BitwiseXor:
        return "^";
    case BinaryExpression::Operator::BitwiseAnd:
        return "&";
    case BinaryExpression::Operator::In:
        return "in";
    case BinaryExpression::Operator::Instanceof:
        return "instanceof";
    }
    throw runtime_error("Fell out of exhaustive switch for BinaryExpression::Operator");
}

template <>
std::string valueStr<UnaryExpression::Operator>(UnaryExpression::Operator v)
{
    switch (v) {
    case UnaryExpression::Operator::Minus:
        return "-";
    case UnaryExpression::Operator::Plus:
        return "+";
    case UnaryExpression::Operator::LogicalNot:
        return "!";
    case UnaryExpression::Operator::BitwiseNot:
        return "~";
    case UnaryExpression::Operator::Typeof:
        return "typeof";
    case UnaryExpression::Operator::Void:
        return "void";
    case UnaryExpression::Operator::Delete:
        return "delete";
    }

    throw runtime_error("Fell out of exhaustive switch for UnaryExpression::Operator");
}

template <>
std::string valueStr<LogicalExpression::Operator>(LogicalExpression::Operator v)
{
    switch (v) {
    case LogicalExpression::Operator::And:
        return "&&";
    case LogicalExpression::Operator::Or:
        return "||";
    }

    throw runtime_error("Fell out of exhaustive switch for LogicalExpression::Operator");
}

template <>
std::string valueStr<UpdateExpression::Operator>(UpdateExpression::Operator v)
{
    switch (v) {
    case UpdateExpression::Operator::Increment:
        return "++";
    case UpdateExpression::Operator::Decrement:
        return "--";
    }

    throw runtime_error("Fell out of exhaustive switch for UpdateExpression::Operator");
}

template <>
std::string valueStr<AssignmentExpression::Operator>(AssignmentExpression::Operator v)
{
    switch (v) {
    case AssignmentExpression::Operator::Equal:
        return "=";
    case AssignmentExpression::Operator::PlusEqual:
        return "+=";
    case AssignmentExpression::Operator::MinusEqual:
        return "-=";
    case AssignmentExpression::Operator::TimesEqual:
        return "*=";
    case AssignmentExpression::Operator::SlashEqual:
        return "/=";
    case AssignmentExpression::Operator::ModuloEqual:
        return "%=";
    case AssignmentExpression::Operator::ExponentiationEqual:
        return "**=";
    case AssignmentExpression::Operator::LeftShiftEqual:
        return "<<=";
    case AssignmentExpression::Operator::SignRightShiftEqual:
        return ">>=";
    case AssignmentExpression::Operator::ZeroingRightShiftEqual:
        return ">>>=";
    case AssignmentExpression::Operator::OrEqual:
        return "|=";
    case AssignmentExpression::Operator::AndEqual:
        return "&=";
    case AssignmentExpression::Operator::XorEqual:
        return "^=";
    }

    throw runtime_error("Fell out of exhaustive switch for LogicalExpression::Operator");
}

std::string makeLabel(const GraphNode& node)
{
    std::string base = node.getTypeName();
    if (node.getType() == GraphNodeType::Literal) {
        AstNode& literal = *node.getAstReference();
        base = literal.getTypeName();
        auto type = literal.getType();
        if (type == AstNodeType::NullLiteral)
            base += valueStr("null"s);
        else if (type == AstNodeType::NumericLiteral)
            base += valueStr(((NumericLiteral&)literal).getValue());
        else if (type == AstNodeType::BooleanLiteral)
            base += valueStr(((BooleanLiteral&)literal).getValue());
        else if (type == AstNodeType::StringLiteral)
            base += valueStr(((StringLiteral&)literal).getValue());
        else if (type == AstNodeType::RegExpLiteral)
            base += valueStr(((RegExpLiteral&)literal).getPattern());
    } else if (node.getType() == GraphNodeType::LoadValue
               || node.getType() == GraphNodeType::StoreValue
               || node.getType() == GraphNodeType::LoadParameter
               || node.getType() == GraphNodeType::StoreParameter
               || node.getType() == GraphNodeType::LoadNamedProperty
               || node.getType() == GraphNodeType::StoreNamedProperty) {
        base += " \\\""+((Identifier&)*node.getAstReference()).getName()+"\\\"";
    } else if (node.getType() == GraphNodeType::ObjectProperty) {
        if (node.inputCount() == 1) {
            AstNode* key = ((ObjectProperty&)*node.getAstReference()).getKey();
            if (key->getType() == AstNodeType::Identifier)
                base += valueStr(((Identifier*)key)->getName());
            else if (key->getType() == AstNodeType::StringLiteral)
                base += valueStr(((StringLiteral*)key)->getValue());
            else if (key->getType() == AstNodeType::NumericLiteral)
                base += valueStr(((NumericLiteral*)key)->getValue());
        }
    } else if (node.getType() == GraphNodeType::Function) {
        if (auto id = ((Function*)node.getAstReference())->getId())
            base += " \\\""+id->getName()+"\\\"";
    } else if (node.getType() == GraphNodeType::Case) {
        if (node.inputCount() == 0)
            base += " [Default]";
    } else if (node.getType() == GraphNodeType::BinaryOperator) {
        if (node.getAstReference()->getType() == AstNodeType::BinaryExpression)
            base += " "+valueStr(((BinaryExpression&)*node.getAstReference()).getOperator());
        else if (node.getAstReference()->getType() == AstNodeType::LogicalExpression)
            base += " "+valueStr(((LogicalExpression&)*node.getAstReference()).getOperator());
        else if (node.getAstReference()->getType() == AstNodeType::AssignmentExpression)
            base += " "+valueStr(((AssignmentExpression&)*node.getAstReference()).getOperator());
    } else if (node.getType() == GraphNodeType::UnaryOperator) {
        if (node.getAstReference()->getType() == AstNodeType::UnaryExpression)
            base += " "+valueStr(((UnaryExpression&)*node.getAstReference()).getOperator());
        else if (node.getAstReference()->getType() == AstNodeType::UpdateExpression)
            base += " "+valueStr(((UpdateExpression&)*node.getAstReference()).getOperator());
    }
    return base;
}

std::string makePrevLabel(GraphNodeType type, uint16_t j)
{
    if (type == GraphNodeType::Merge) {
        return "phi"+to_string(j);
    }

    return {};
}

std::string makeInputLabel(GraphNodeType type, uint16_t j)
{
    if (type == GraphNodeType::StoreProperty) {
        if (j == 0)
            return "obj";
        else if (j == 1)
            return "prop";
        else if (j == 2)
            return "val";
    } else if (type == GraphNodeType::LoadProperty) {
        if (j == 0)
            return "obj";
        else if (j == 1)
            return "prop";
    } else if (type == GraphNodeType::StoreNamedProperty) {
        if (j == 0)
            return "obj";
        else if (j == 1)
            return "val";
    } else if (type == GraphNodeType::ObjectProperty) {
        if (j == 0)
            return "val";
        else if (j == 1)
            return "key";
    } else if (type == GraphNodeType::LoadNamedProperty) {
        if (j == 0)
            return "obj";
    } else if (type == GraphNodeType::BinaryOperator) {
        if (j == 0)
            return "lhs";
        else if (j == 1)
            return "rhs";
    } else if (type == GraphNodeType::Call || type == GraphNodeType::NewCall) {
        if (j == 0)
            return "callee";
        else
            return "arg "+to_string(j);
    } else if (type == GraphNodeType::Phi) {
        return "phi"+to_string(j);
    } else if (type == GraphNodeType::ArrayLiteral || type == GraphNodeType::ObjectLiteral) {
        return to_string(j);
   }

    return {};
}

std::string graphToDOT(const Graph& graph)
{
    std::string text;
    if (auto* id = graph.getFun().getId())
        text += "// Function \""+id->getName()+"\"\n";
    text += "digraph ControlGraph {\n{ rank=source; 0; };\n";

    std::vector<bool> nodesUsed(graph.size());
    for (uint16_t i = 0; i < graph.size(); ++i)
        for (uint16_t input = 0; input < graph.getNode(i).inputCount(); ++input)
            nodesUsed[graph.getNode(i).getInput(input)] = true;

    for (uint16_t i = 0; i < graph.size(); ++i) {
        const GraphNode& node = graph.getNode(i);
        if (!node.prevCount() && !node.nextCount() && !nodesUsed[i])
            continue;
        text += to_string(i) + " [label=\"" + makeLabel(node) + "\"];\n";
        for (uint16_t j = 0; j < node.prevCount(); ++j)
            text += to_string(node.getPrev(j)) + " -> " + to_string(i) + " [color=red style=bold label=\""+makePrevLabel(node.getType(), j)+"\"];\n";
        for (uint16_t j = 0; j < node.inputCount(); ++j)
            text += to_string(node.getInput(j)) + " -> " + to_string(i) + " [color=blue label=\""+makeInputLabel(node.getType(), j)+"\"];\n";
    }

    text += "}\n";
    return text;
}
