#include "import.hpp"
#include "ast.hpp"
#include <cassert>
#include <iostream>
#include <unordered_map>

using namespace std;
using json = nlohmann::json;

class Module;

#define X(NODE) AstNode* import##NODE(const json&);
IMPORTED_NODE_LIST(X)
#undef X

#define X(NODE) { #NODE, import##NODE },
static const unordered_map<string, AstNode* (*)(const json&)> importFunctions = {
    IMPORTED_NODE_LIST(X)
};
#undef X

AstRoot* importBabylonAst(Module& parentModule, const json& jast)
{
    json program = jast["program"];
    assert(program["type"] == "Program");

    vector<AstNode*> bodyNodes;
    for (auto&& jnode : program["body"])
        bodyNodes.push_back(importNode(jnode));

    return new AstRoot(parentModule, bodyNodes);
}

AstNode* importNode(const json& node)
{
    auto typeIt = node.find("type");
    if (typeIt == node.end())
        throw runtime_error("Trying to import AST node with no type!");
    auto funIt = importFunctions.find(*typeIt);
    if (funIt == importFunctions.end())
        throw runtime_error("Unknown node of type " + typeIt->get<string>() + " in Babylon AST");
    return funIt->second(node);
}

AstNode* importNodeOrNullptr(const nlohmann::json& node, const char* name)
{
    auto nodeIt = node.find(name);
    if (nodeIt == node.end())
        return nullptr;
    else if (nodeIt->type() == json::value_t::null)
        return nullptr;
    else
        return importNode(*nodeIt);
}

AstNode* importIdentifier(const json& node)
{
    return new Identifier(node["name"].get<string>());
}

AstNode* importStringLiteral(const json& node)
{
    return new StringLiteral(node["value"].get<string>());
}

AstNode* importRegExpLiteral(const json& node)
{
    return new RegExpLiteral(node["pattern"].get<string>(), node["flags"].get<string>());
}

AstNode* importNullLiteral(const json&)
{
    return new NullLiteral();
}

AstNode* importBooleanLiteral(const json& node)
{
    return new BooleanLiteral((bool)node["value"]);
}

AstNode* importNumericLiteral(const json& node)
{
    return new NumericLiteral((double)node["value"]);
}

AstNode* importTemplateLiteral(const json& node)
{
    vector<AstNode*> quasis;
    for (auto&& child : node["quasis"])
        quasis.push_back(importNode(child));
    vector<AstNode*> expressions;
    for (auto&& child : node["expressions"])
        expressions.push_back(importNode(child));
    return new TemplateLiteral(quasis, expressions);
}

AstNode* importTemplateElement(const json& node)
{
    return new TemplateElement(node["value"]["raw"].get<string>(), (bool)node["tail"]);
}

AstNode* importTaggedTemplateExpression(const json& node)
{
    return new TaggedTemplateExpression(importNode(node["tag"]), importNode(node["quasi"]));
}

AstNode* importExpressionStatement(const json& node)
{
    return new ExpressionStatement(importNode(node["expression"]));
}

AstNode* importEmptyStatement(const json& node)
{
    return new EmptyStatement();
}

AstNode* importWithStatement(const json& node)
{
    return new WithStatement(importNode(node["object"]), importNode(node["body"]));
}

AstNode* importDebuggerStatement(const json& node)
{
    return new DebuggerStatement();
}

AstNode* importReturnStatement(const json& node)
{
    AstNode* argument = node["argument"] != nullptr ? importNode(node["argument"]) : nullptr;
    return new ReturnStatement(argument);
}

AstNode* importLabeledStatement(const json& node)
{
    return new LabeledStatement(importNode(node["label"]), importNode(node["body"]));
}

AstNode* importBreakStatement(const json& node)
{
    AstNode* label = node["label"] != nullptr ? importNode(node["label"]) : nullptr;
    return new BreakStatement(label);
}

AstNode* importContinueStatement(const json& node)
{
    AstNode* label = node["label"] != nullptr ? importNode(node["label"]) : nullptr;
    return new ContinueStatement(label);
}

AstNode* importIfStatement(const json& node)
{
    AstNode* alternate = node["alternate"] != nullptr ? importNode(node["alternate"]) : nullptr;
    return new IfStatement(importNode(node["test"]), importNode(node["consequent"]), alternate);
}

AstNode* importSwitchStatement(const json& node)
{
    vector<AstNode*> cases;
    for (auto&& child : node["cases"])
        cases.push_back(importNode(child));
    return new SwitchStatement(importNode(node["discriminant"]), cases);
}

AstNode* importSwitchCase(const json& node)
{
    AstNode* test = node["test"] != nullptr ? importNode(node["test"]) : nullptr;
    vector<AstNode*> consequent;
    for (auto&& child : node["consequent"])
        consequent.push_back(importNode(child));
    return new SwitchCase(test, consequent);
}

AstNode* importThrowStatement(const json& node)
{
    return new ThrowStatement(importNode(node["argument"]));
}

AstNode* importTryStatement(const json& node)
{
    AstNode* handler = node["handler"] != nullptr ? importNode(node["handler"]) : nullptr;
    AstNode* finalizer = node["finalizer"] != nullptr ? importNode(node["finalizer"]) : nullptr;
    return new TryStatement(importNode(node["block"]), handler, finalizer);
}

AstNode* importCatchClause(const json& node)
{
    AstNode* param = node["param"] != nullptr ? importNode(node["param"]) : nullptr;
    return new CatchClause(param, importNode(node["body"]));
}

AstNode* importWhileStatement(const json& node)
{
    return new WhileStatement(importNode(node["test"]), importNode(node["body"]));
}

AstNode* importDoWhileStatement(const json& node)
{
    return new DoWhileStatement(importNode(node["test"]), importNode(node["body"]));
}

AstNode* importForStatement(const json& node)
{
    AstNode* init = node["init"] != nullptr ? importNode(node["init"]) : nullptr;
    AstNode* test = node["test"] != nullptr ? importNode(node["test"]) : nullptr;
    AstNode* update = node["update"] != nullptr ? importNode(node["update"]) : nullptr;
    return new ForStatement(init, test, update, importNode(node["body"]));
}

AstNode* importForInStatement(const json& node)
{
    return new ForInStatement(importNode(node["left"]), importNode(node["right"]), importNode(node["body"]));
}

AstNode* importForOfStatement(const json& node)
{
    bool await = false;
    if (auto it = node.find("await"); it != node.end())
        await = (bool)*it;
    return new ForOfStatement(importNode(node["left"]), importNode(node["right"]), importNode(node["body"]), await);
}

AstNode* importBlockStatement(const json& node)
{
    vector<AstNode*> body;
    for (auto&& child : node["body"])
        body.push_back(importNode(child));
    return new BlockStatement(body);
}

AstNode* importSuper(const json&)
{
    return new Super();
}

AstNode* importImport(const json&)
{
    return new Import();
}

AstNode* importThisExpression(const json&)
{
    return new ThisExpression();
}

AstNode* importArrowFunctionExpression(const json& node)
{
    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    return new ArrowFunctionExpression(id, params, importNode(node["body"]), (bool)node["generator"], (bool)node["async"], (bool)node["expression"]);
}

AstNode* importYieldExpression(const json& node)
{
    AstNode* argument = node["argument"] != nullptr ? importNode(node["argument"]) : nullptr;
    return new YieldExpression(argument, (bool)node["delegate"]);
}

AstNode* importAwaitExpression(const json& node)
{
    AstNode* argument = node["argument"] != nullptr ? importNode(node["argument"]) : nullptr;
    return new AwaitExpression(argument);
}

AstNode* importArrayExpression(const json& node)
{
    vector<AstNode*> elements;
    for (auto&& child : node["elements"])
        elements.push_back(importNode(child));
    return new ArrayExpression(elements);
}

AstNode* importObjectExpression(const json& node)
{
    vector<AstNode*> props;
    for (auto&& prop : node["properties"])
        props.push_back(importNode(prop));
    return new ObjectExpression(props);
}

AstNode* importObjectProperty(const json& node)
{
    return new ObjectProperty(importNode(node["key"]),
        importNode(node["value"]),
        node["shorthand"],
        node["computed"]);
}

AstNode* importObjectMethod(const json& node)
{
    using Kind = ObjectMethod::Kind;
    Kind kind;
    string kindStr = node["kind"];
    if (kindStr == "method")
        kind = Kind::Method;
    else if (kindStr == "get")
        kind = Kind::Get;
    else if (kindStr == "set")
        kind = Kind::Set;
    else
        throw runtime_error("Unknown private class method declaration kind " + kindStr);

    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    return new ObjectMethod(id, params, importNode(node["body"]), importNode(node["key"]), kind,
        (bool)node["generator"], (bool)node["async"], (bool)node["computed"]);
}

AstNode* importFunctionExpression(const json& node)
{
    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    return new FunctionExpression(id, params, importNode(node["body"]), (bool)node["generator"], (bool)node["async"]);
}

AstNode* importUnaryExpression(const json& node)
{
    using Operator = UnaryExpression::Operator;
    Operator op;
    string opStr = node["operator"];
    if (opStr == "-")
        op = Operator::Minus;
    else if (opStr == "+")
        op = Operator::Plus;
    else if (opStr == "!")
        op = Operator::LogicalNot;
    else if (opStr == "~")
        op = Operator::BitwiseNot;
    else if (opStr == "typeof")
        op = Operator::Typeof;
    else if (opStr == "void")
        op = Operator::Void;
    else if (opStr == "delete")
        op = Operator::Delete;
    else if (opStr == "throw")
        op = Operator::Throw;
    else
        throw runtime_error("Unknown unary operator " + opStr);
    return new UnaryExpression(importNode(node["argument"]), op, (bool)node["prefix"]);
}

AstNode* importUpdateExpression(const json& node)
{
    using Operator = UpdateExpression::Operator;
    Operator op;
    string opStr = node["operator"];
    if (opStr == "++")
        op = Operator::Increment;
    else if (opStr == "--")
        op = Operator::Decrement;
    else
        throw runtime_error("Unknown update operator " + opStr);
    return new UpdateExpression(importNode(node["argument"]), op, (bool)node["prefix"]);
}

AstNode* importBinaryExpression(const json& node)
{
    using Operator = BinaryExpression::Operator;
    Operator op;
    string opStr = node["operator"];
    if (opStr == "==")
        op = Operator::Equal;
    else if (opStr == "!=")
        op = Operator::NotEqual;
    else if (opStr == "===")
        op = Operator::StrictEqual;
    else if (opStr == "!==")
        op = Operator::StrictNotEqual;
    else if (opStr == "<")
        op = Operator::Lesser;
    else if (opStr == "<=")
        op = Operator::LesserOrEqual;
    else if (opStr == ">")
        op = Operator::Greater;
    else if (opStr == ">=")
        op = Operator::GreaterOrEqual;
    else if (opStr == "<<")
        op = Operator::ShiftLeft;
    else if (opStr == ">>")
        op = Operator::SignShiftRight;
    else if (opStr == ">>>")
        op = Operator::ZeroingShiftRight;
    else if (opStr == "+")
        op = Operator::Plus;
    else if (opStr == "-")
        op = Operator::Minus;
    else if (opStr == "*")
        op = Operator::Times;
    else if (opStr == "/")
        op = Operator::Division;
    else if (opStr == "%")
        op = Operator::Modulo;
    else if (opStr == "|")
        op = Operator::BitwiseOr;
    else if (opStr == "^")
        op = Operator::BitwiseXor;
    else if (opStr == "&")
        op = Operator::BitwiseAnd;
    else if (opStr == "in")
        op = Operator::In;
    else if (opStr == "instanceof")
        op = Operator::Instanceof;
    else
        throw runtime_error("Unknown binary operator " + opStr);
    return new BinaryExpression(importNode(node["left"]), importNode(node["right"]), op);
}

AstNode* importAssignmentExpression(const json& node)
{
    using Operator = AssignmentExpression::Operator;
    Operator op;
    string opStr = node["operator"];
    if (opStr == "=")
        op = Operator::Equal;
    else if (opStr == "+=")
        op = Operator::PlusEqual;
    else if (opStr == "-=")
        op = Operator::MinusEqual;
    else if (opStr == "*=")
        op = Operator::TimesEqual;
    else if (opStr == "/=")
        op = Operator::SlashEqual;
    else if (opStr == "%=")
        op = Operator::ModuloEqual;
    else if (opStr == "<<=")
        op = Operator::LeftShiftEqual;
    else if (opStr == ">>=")
        op = Operator::SignRightShiftEqual;
    else if (opStr == ">>>=")
        op = Operator::ZeroingRightShiftEqual;
    else if (opStr == "|=")
        op = Operator::OrEqual;
    else if (opStr == "^=")
        op = Operator::XorEqual;
    else if (opStr == "&=")
        op = Operator::AndEqual;
    else
        throw runtime_error("Unknown assigment operator " + opStr);
    return new AssignmentExpression(importNode(node["left"]), importNode(node["right"]), op);
}

AstNode* importLogicalExpression(const json& node)
{
    using Operator = LogicalExpression::Operator;
    Operator op;
    string opStr = node["operator"];
    if (opStr == "||")
        op = Operator::Or;
    else if (opStr == "&&")
        op = Operator::And;
    else
        throw runtime_error("Unknown logical operator " + opStr);
    return new LogicalExpression(importNode(node["left"]), importNode(node["right"]), op);
}

AstNode* importMemberExpression(const json& node)
{
    return new MemberExpression(importNode(node["object"]), importNode(node["property"]), node["computed"]);
}

AstNode* importBindExpression(const json& node)
{
    AstNode* object = node["object"] != nullptr ? importNode(node["object"]) : nullptr;
    return new BindExpression(object, importNode(node["callee"]));
}

AstNode* importConditionalExpression(const json& node)
{
    return new ConditionalExpression(importNode(node["test"]), importNode(node["alternate"]), importNode(node["consequent"]));
}

AstNode* importCallExpression(const json& node)
{
    auto callee = importNode(node["callee"]);
    vector<AstNode*> args;
    for (auto&& arg : node["arguments"])
        args.push_back(importNode(arg));
    return new CallExpression(callee, args);
}

AstNode* importNewExpression(const json& node)
{
    auto callee = importNode(node["callee"]);
    vector<AstNode*> args;
    for (auto&& arg : node["arguments"])
        args.push_back(importNode(arg));
    return new NewExpression(callee, args);
}

AstNode* importSequenceExpression(const json& node)
{
    vector<AstNode*> expressions;
    for (auto&& child : node["expressions"])
        expressions.push_back(importNode(child));
    return new SequenceExpression(expressions);
}

AstNode* importDoExpression(const json& node)
{
    return new DoExpression(importNode(node["body"]));
}

AstNode* importClassExpression(const json& node)
{
    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    AstNode* superClass = node["superClass"] != nullptr ? importNode(node["superClass"]) : nullptr;
    return new ClassExpression(id, superClass, importNode(node["body"]));
}

AstNode* importClassBody(const json& node)
{
    vector<AstNode*> body;
    for (auto&& child : node["body"])
        body.push_back(importNode(child));
    return new ClassBody(body);
}

AstNode* importClassMethod(const json& node)
{
    using Kind = ClassMethod::Kind;
    Kind kind;
    string kindStr = node["kind"];
    if (kindStr == "constructor")
        kind = Kind::Constructor;
    else if (kindStr == "method")
        kind = Kind::Method;
    else if (kindStr == "get")
        kind = Kind::Get;
    else if (kindStr == "set")
        kind = Kind::Set;
    else
        throw runtime_error("Unknown class method declaration kind " + kindStr);

    AstNode* id = importNodeOrNullptr(node, "id");
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    return new ClassMethod(id, params, importNode(node["body"]), importNode(node["key"]), kind,
        (bool)node["generator"], (bool)node["async"], (bool)node["computed"], (bool)node["static"]);
}

AstNode* importClassPrivateMethod(const json& node)
{
    using Kind = ClassPrivateMethod::Kind;
    Kind kind;
    string kindStr = node["kind"];
    if (kindStr == "method")
        kind = Kind::Method;
    else if (kindStr == "get")
        kind = Kind::Get;
    else if (kindStr == "set")
        kind = Kind::Set;
    else
        throw runtime_error("Unknown private class method declaration kind " + kindStr);

    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    return new ClassPrivateMethod(id, params, importNode(node["body"]), importNode(node["key"]), kind,
        (bool)node["generator"], (bool)node["async"], (bool)node["static"]);
}

AstNode* importClassProperty(const json& node)
{
    return new ClassProperty(importNode(node["key"]), importNode(node["value"]), (bool)node["static"], (bool)node["computed"]);
}

AstNode* importClassPrivateProperty(const json& node)
{
    return new ClassPrivateProperty(importNode(node["key"]), importNode(node["value"]), (bool)node["static"]);
}

AstNode* importClassDeclaration(const json& node)
{
    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    AstNode* superClass = node["superClass"] != nullptr ? importNode(node["superClass"]) : nullptr;
    return new ClassDeclaration(id, superClass, importNode(node["body"]));
}

AstNode* importVariableDeclaration(const json& node)
{
    using Kind = VariableDeclaration::Kind;
    Kind kind;
    string kindStr = node["kind"];
    if (kindStr == "var")
        kind = Kind::Var;
    else if (kindStr == "let")
        kind = Kind::Let;
    else if (kindStr == "const")
        kind = Kind::Const;
    else
        throw runtime_error("Unknown variable declaration kind " + kindStr);

    vector<AstNode*> declarators;
    for (auto&& child : node["declarations"])
        declarators.push_back(importNode(child));
    return new VariableDeclaration(declarators, kind);
}

AstNode* importVariableDeclarator(const json& node)
{
    AstNode* init;
    if (auto jinit = node["init"]; jinit != nullptr)
        init = importNode(jinit);
    else
        init = nullptr;
    return new VariableDeclarator(importNode(node["id"]), init);
}

AstNode* importFunctionDeclaration(const json& node)
{
    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    return new FunctionDeclaration(id, params, importNode(node["body"]), (bool)node["generator"], (bool)node["async"]);
}

AstNode* importSpreadElement(const json& node)
{
    return new SpreadElement(importNode(node["argument"]));
}

AstNode* importObjectPattern(const json& node)
{
    vector<AstNode*> properties;
    for (auto&& child : node["properties"])
        properties.push_back(importNode(child));
    return new ObjectPattern(properties);
}

AstNode* importArrayPattern(const json& node)
{
    vector<AstNode*> elements;
    for (auto&& child : node["elements"])
        elements.push_back(importNode(child));
    return new ArrayPattern(elements);
}

AstNode* importAssignmentPattern(const json& node)
{
    return new AssignmentPattern(importNode(node["left"]), importNode(node["right"]));
}

AstNode* importRestElement(const json& node)
{
    return new RestElement(importNode(node["argument"]));
}

AstNode* importMetaProperty(const json& node)
{
    return new MetaProperty(importNode(node["meta"]), importNode(node["property"]));
}

AstNode* importImportDeclaration(const json& node)
{
    vector<AstNode*> specifiers;
    for (auto&& child : node["specifiers"])
        specifiers.push_back(importNode(child));
    return new ImportDeclaration(specifiers, importNode(node["source"]));
}

AstNode* importImportSpecifier(const json& node)
{
    return new ImportSpecifier(importNode(node["local"]), importNode(node["imported"]));
}

AstNode* importImportDefaultSpecifier(const json& node)
{
    return new ImportDefaultSpecifier(importNode(node["local"]));
}

AstNode* importImportNamespaceSpecifier(const json& node)
{
    return new ImportNamespaceSpecifier(importNode(node["local"]));
}

AstNode* importExportNamedDeclaration(const json& node)
{
    AstNode* declaration = importNodeOrNullptr(node, "declaration");
    AstNode* source = importNodeOrNullptr(node, "source");
    vector<AstNode*> specifiers;
    for (auto&& child : node["specifiers"])
        specifiers.push_back(importNode(child));
    return new ExportNamedDeclaration(declaration, source, specifiers);
}

AstNode* importExportDefaultDeclaration(const json& node)
{
    return new ExportDefaultDeclaration(importNode(node["declaration"]));
}

AstNode* importExportAllDeclaration(const json& node)
{
    return new ExportAllDeclaration(importNode(node["source"]));
}

AstNode* importExportSpecifier(const json& node)
{
    return new ExportSpecifier(importNode(node["local"]), importNode(node["exported"]));
}

AstNode* importExportDefaultSpecifier(const json& node)
{
    return new ExportDefaultSpecifier(importNode(node["exported"]));
}
