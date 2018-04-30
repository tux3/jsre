#include "import.hpp"
#include "ast.hpp"
#include "location.hpp"
#include <cassert>
#include <iostream>
#include <unordered_map>

using namespace std;
using json = nlohmann::json;

class Module;

#define X(NODE) AstNode* import##NODE(const json&, AstSourceSpan);
IMPORTED_NODE_LIST(X)
#undef X

#define X(NODE) { #NODE, import##NODE },
static const unordered_map<string, AstNode* (*)(const json&, AstSourceSpan)> importFunctions = {
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
    auto loc = importLocation(program);

    return new AstRoot(loc, parentModule, bodyNodes);
}

AstNode* importNode(const json& node)
{
    auto typeIt = node.find("type");
    if (typeIt == node.end())
        throw runtime_error("Trying to import AST node with no type!");
    auto funIt = importFunctions.find(*typeIt);
    if (funIt == importFunctions.end())
        throw runtime_error("Unknown node of type " + typeIt->get<string>() + " in Babylon AST");
    auto loc = importLocation(node);
    return funIt->second(node, loc);
}

AstNode* importNodeOrNullptr(const json& node, const char* name)
{
    auto nodeIt = node.find(name);
    if (nodeIt == node.end())
        return nullptr;
    else if (nodeIt->type() == json::value_t::null)
        return nullptr;
    else
        return importNode(*nodeIt);
}

AstSourceSpan importLocation(const json& node)
{
    const json& nodeLoc = node["loc"];
    unsigned beginOff = node["start"];
    unsigned beginLine = nodeLoc["start"]["line"];
    unsigned beginCol = nodeLoc["start"]["column"];
    unsigned endOff = node["end"];
    unsigned endLine = nodeLoc["end"]["line"];
    unsigned endCol = nodeLoc["end"]["column"];
    return {
        {beginOff, beginLine, beginCol},
        {endOff, endLine, endCol}
    };
}

AstNode* importIdentifier(const json& node, AstSourceSpan loc)
{
    bool isOptional = false;
    if (auto it = node.find("optional"); it != node.end())
        isOptional = (bool)*it;

    return new Identifier(loc, node["name"].get<string>(), (TypeAnnotation*)importNodeOrNullptr(node, "typeAnnotation"), isOptional);
}

AstNode* importStringLiteral(const json& node, AstSourceSpan loc)
{
    return new StringLiteral(loc, node["value"].get<string>());
}

AstNode* importRegExpLiteral(const json& node, AstSourceSpan loc)
{
    return new RegExpLiteral(loc, node["pattern"].get<string>(), node["flags"].get<string>());
}

AstNode* importNullLiteral(const json& node, AstSourceSpan loc)
{
    return new NullLiteral(loc);
}

AstNode* importBooleanLiteral(const json& node, AstSourceSpan loc)
{
    return new BooleanLiteral(loc, (bool)node["value"]);
}

AstNode* importNumericLiteral(const json& node, AstSourceSpan loc)
{
    return new NumericLiteral(loc, (double)node["value"]);
}

AstNode* importTemplateLiteral(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> quasis;
    for (auto&& child : node["quasis"])
        quasis.push_back(importNode(child));
    vector<AstNode*> expressions;
    for (auto&& child : node["expressions"])
        expressions.push_back(importNode(child));
    return new TemplateLiteral(loc, quasis, expressions);
}

AstNode* importTemplateElement(const json& node, AstSourceSpan loc)
{
    return new TemplateElement(loc, node["value"]["raw"].get<string>(), (bool)node["tail"]);
}

AstNode* importTaggedTemplateExpression(const json& node, AstSourceSpan loc)
{
    return new TaggedTemplateExpression(loc, importNode(node["tag"]), importNode(node["quasi"]));
}

AstNode* importExpressionStatement(const json& node, AstSourceSpan loc)
{
    return new ExpressionStatement(loc, importNode(node["expression"]));
}

AstNode* importEmptyStatement(const json& node, AstSourceSpan loc)
{
    return new EmptyStatement(loc);
}

AstNode* importWithStatement(const json& node, AstSourceSpan loc)
{
    return new WithStatement(loc, importNode(node["object"]), importNode(node["body"]));
}

AstNode* importDebuggerStatement(const json& node, AstSourceSpan loc)
{
    return new DebuggerStatement(loc);
}

AstNode* importReturnStatement(const json& node, AstSourceSpan loc)
{
    AstNode* argument = node["argument"] != nullptr ? importNode(node["argument"]) : nullptr;
    return new ReturnStatement(loc, argument);
}

AstNode* importLabeledStatement(const json& node, AstSourceSpan loc)
{
    return new LabeledStatement(loc, importNode(node["label"]), importNode(node["body"]));
}

AstNode* importBreakStatement(const json& node, AstSourceSpan loc)
{
    AstNode* label = node["label"] != nullptr ? importNode(node["label"]) : nullptr;
    return new BreakStatement(loc, label);
}

AstNode* importContinueStatement(const json& node, AstSourceSpan loc)
{
    AstNode* label = node["label"] != nullptr ? importNode(node["label"]) : nullptr;
    return new ContinueStatement(loc, label);
}

AstNode* importIfStatement(const json& node, AstSourceSpan loc)
{
    AstNode* alternate = node["alternate"] != nullptr ? importNode(node["alternate"]) : nullptr;
    return new IfStatement(loc, importNode(node["test"]), importNode(node["consequent"]), alternate);
}

AstNode* importSwitchStatement(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> cases;
    for (auto&& child : node["cases"])
        cases.push_back(importNode(child));
    return new SwitchStatement(loc, importNode(node["discriminant"]), cases);
}

AstNode* importSwitchCase(const json& node, AstSourceSpan loc)
{
    AstNode* test = node["test"] != nullptr ? importNode(node["test"]) : nullptr;
    vector<AstNode*> consequent;
    for (auto&& child : node["consequent"])
        consequent.push_back(importNode(child));
    return new SwitchCase(loc, test, consequent);
}

AstNode* importThrowStatement(const json& node, AstSourceSpan loc)
{
    return new ThrowStatement(loc, importNode(node["argument"]));
}

AstNode* importTryStatement(const json& node, AstSourceSpan loc)
{
    AstNode* handler = node["handler"] != nullptr ? importNode(node["handler"]) : nullptr;
    AstNode* finalizer = node["finalizer"] != nullptr ? importNode(node["finalizer"]) : nullptr;
    return new TryStatement(loc, importNode(node["block"]), handler, finalizer);
}

AstNode* importCatchClause(const json& node, AstSourceSpan loc)
{
    AstNode* param = node["param"] != nullptr ? importNode(node["param"]) : nullptr;
    return new CatchClause(loc, param, importNode(node["body"]));
}

AstNode* importWhileStatement(const json& node, AstSourceSpan loc)
{
    return new WhileStatement(loc, importNode(node["test"]), importNode(node["body"]));
}

AstNode* importDoWhileStatement(const json& node, AstSourceSpan loc)
{
    return new DoWhileStatement(loc, importNode(node["test"]), importNode(node["body"]));
}

AstNode* importForStatement(const json& node, AstSourceSpan loc)
{
    AstNode* init = node["init"] != nullptr ? importNode(node["init"]) : nullptr;
    AstNode* test = node["test"] != nullptr ? importNode(node["test"]) : nullptr;
    AstNode* update = node["update"] != nullptr ? importNode(node["update"]) : nullptr;
    return new ForStatement(loc, init, test, update, importNode(node["body"]));
}

AstNode* importForInStatement(const json& node, AstSourceSpan loc)
{
    return new ForInStatement(loc, importNode(node["left"]), importNode(node["right"]), importNode(node["body"]));
}

AstNode* importForOfStatement(const json& node, AstSourceSpan loc)
{
    bool await = false;
    if (auto it = node.find("await"); it != node.end())
        await = (bool)*it;
    return new ForOfStatement(loc, importNode(node["left"]), importNode(node["right"]), importNode(node["body"]), await);
}

AstNode* importBlockStatement(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> body;
    for (auto&& child : node["body"])
        body.push_back(importNode(child));
    return new BlockStatement(loc, body);
}

AstNode* importSuper(const json&, AstSourceSpan loc)
{
    return new Super(loc);
}

AstNode* importImport(const json&, AstSourceSpan loc)
{
    return new Import(loc);
}

AstNode* importThisExpression(const json&, AstSourceSpan loc)
{
    return new ThisExpression(loc);
}

AstNode* importArrowFunctionExpression(const json& node, AstSourceSpan loc)
{
    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    bool isExpression;
    if (auto it = node.find("expression"); it != node.end())
        isExpression = (bool)*it;
    return new ArrowFunctionExpression(loc, id, params, importNode(node["body"]),
                                        (TypeParameterDeclaration*)importNodeOrNullptr(node, "typeParameters"),
                                        (TypeAnnotation*)importNodeOrNullptr(node, "returnType"),
                                        (bool)node["generator"], (bool)node["async"], isExpression);
}

AstNode* importYieldExpression(const json& node, AstSourceSpan loc)
{
    AstNode* argument = node["argument"] != nullptr ? importNode(node["argument"]) : nullptr;
    return new YieldExpression(loc, argument, (bool)node["delegate"]);
}

AstNode* importAwaitExpression(const json& node, AstSourceSpan loc)
{
    AstNode* argument = node["argument"] != nullptr ? importNode(node["argument"]) : nullptr;
    return new AwaitExpression(loc, argument);
}

AstNode* importArrayExpression(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> elements;
    for (auto&& child : node["elements"])
        elements.push_back(importNode(child));
    return new ArrayExpression(loc, elements);
}

AstNode* importObjectExpression(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> props;
    for (auto&& prop : node["properties"])
        props.push_back(importNode(prop));
    return new ObjectExpression(loc, props);
}

AstNode* importObjectProperty(const json& node, AstSourceSpan loc)
{
    return new ObjectProperty(loc, importNode(node["key"]),
        importNode(node["value"]),
        node["shorthand"],
        node["computed"]);
}

AstNode* importObjectMethod(const json& node, AstSourceSpan loc)
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
    return new ObjectMethod(loc, id, params, importNode(node["body"]),
                            (TypeParameterDeclaration*)importNodeOrNullptr(node, "typeParameters"),
                            (TypeAnnotation*)importNodeOrNullptr(node, "returnType"),
                            importNode(node["key"]), kind,(bool)node["generator"], (bool)node["async"], (bool)node["computed"]);
}

AstNode* importFunctionExpression(const json& node, AstSourceSpan loc)
{
    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    return new FunctionExpression(loc, id, params, importNode(node["body"]),
                                    (TypeParameterDeclaration*)importNodeOrNullptr(node, "typeParameters"),
                                    (TypeAnnotation*)importNodeOrNullptr(node, "returnType"),
                                    (bool)node["generator"], (bool)node["async"]);
}

AstNode* importUnaryExpression(const json& node, AstSourceSpan loc)
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
    return new UnaryExpression(loc, importNode(node["argument"]), op, (bool)node["prefix"]);
}

AstNode* importUpdateExpression(const json& node, AstSourceSpan loc)
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
    return new UpdateExpression(loc, importNode(node["argument"]), op, (bool)node["prefix"]);
}

AstNode* importBinaryExpression(const json& node, AstSourceSpan loc)
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
    return new BinaryExpression(loc, importNode(node["left"]), importNode(node["right"]), op);
}

AstNode* importAssignmentExpression(const json& node, AstSourceSpan loc)
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
    return new AssignmentExpression(loc, importNode(node["left"]), importNode(node["right"]), op);
}

AstNode* importLogicalExpression(const json& node, AstSourceSpan loc)
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
    return new LogicalExpression(loc, importNode(node["left"]), importNode(node["right"]), op);
}

AstNode* importMemberExpression(const json& node, AstSourceSpan loc)
{
    return new MemberExpression(loc, importNode(node["object"]), importNode(node["property"]), node["computed"]);
}

AstNode* importBindExpression(const json& node, AstSourceSpan loc)
{
    AstNode* object = node["object"] != nullptr ? importNode(node["object"]) : nullptr;
    return new BindExpression(loc, object, importNode(node["callee"]));
}

AstNode* importConditionalExpression(const json& node, AstSourceSpan loc)
{
    return new ConditionalExpression(loc, importNode(node["test"]), importNode(node["alternate"]), importNode(node["consequent"]));
}

AstNode* importCallExpression(const json& node, AstSourceSpan loc)
{
    auto callee = importNode(node["callee"]);
    vector<AstNode*> args;
    for (auto&& arg : node["arguments"])
        args.push_back(importNode(arg));
    return new CallExpression(loc, callee, args);
}

AstNode* importNewExpression(const json& node, AstSourceSpan loc)
{
    auto callee = importNode(node["callee"]);
    vector<AstNode*> args;
    for (auto&& arg : node["arguments"])
        args.push_back(importNode(arg));
    return new NewExpression(loc, callee, args);
}

AstNode* importSequenceExpression(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> expressions;
    for (auto&& child : node["expressions"])
        expressions.push_back(importNode(child));
    return new SequenceExpression(loc, expressions);
}

AstNode* importDoExpression(const json& node, AstSourceSpan loc)
{
    return new DoExpression(loc, importNode(node["body"]));
}

AstNode* importClassExpression(const json& node, AstSourceSpan loc)
{
    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    AstNode* superClass = node["superClass"] != nullptr ? importNode(node["superClass"]) : nullptr;
    vector<ClassImplements*> implements;
    if (node.find("implements") != node.end())
        for (auto&& child : node["implements"])
            implements.push_back((ClassImplements*)importNode(child));
    return new ClassExpression(loc, id, superClass, importNode(node["body"]),
                                (TypeParameterDeclaration*)importNodeOrNullptr(node, "typeParameters"),
                                (TypeParameterInstantiation*)importNodeOrNullptr(node, "superTypeParameters"), implements);
}

AstNode* importClassBody(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> body;
    for (auto&& child : node["body"])
        body.push_back(importNode(child));
    return new ClassBody(loc, body);
}

AstNode* importClassMethod(const json& node, AstSourceSpan loc)
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
    return new ClassMethod(loc, id, params, importNode(node["body"]), importNode(node["key"]),
            (TypeParameterDeclaration*)importNodeOrNullptr(node, "typeParameters"),
            (TypeAnnotation*)importNodeOrNullptr(node, "returnType"),
            kind, (bool)node["generator"], (bool)node["async"], (bool)node["computed"], (bool)node["static"]);
}

AstNode* importClassPrivateMethod(const json& node, AstSourceSpan loc)
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
    return new ClassPrivateMethod(loc, id, params, importNode(node["body"]), importNode(node["key"]),
            (TypeParameterDeclaration*)importNodeOrNullptr(node, "typeParameters"),
            (TypeAnnotation*)importNodeOrNullptr(node, "returnType"),
            kind, (bool)node["generator"], (bool)node["async"], (bool)node["static"]);
}

AstNode* importClassProperty(const json& node, AstSourceSpan loc)
{
    return new ClassProperty(loc, importNode(node["key"]), importNodeOrNullptr(node, "value"),
                            (TypeAnnotation*)importNodeOrNullptr(node, "typeAnnotation"), (bool)node["static"], (bool)node["computed"]);
}

AstNode* importClassPrivateProperty(const json& node, AstSourceSpan loc)
{
    return new ClassPrivateProperty(loc, importNode(node["key"]), importNode(node["value"]),
                                    (TypeAnnotation*)importNodeOrNullptr(node, "typeAnnotation"), (bool)node["static"]);
}

AstNode* importClassDeclaration(const json& node, AstSourceSpan loc)
{
    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    AstNode* superClass = node["superClass"] != nullptr ? importNode(node["superClass"]) : nullptr;
    vector<ClassImplements*> implements;
    if (node.find("implements") != node.end())
        for (auto&& child : node["implements"])
            implements.push_back((ClassImplements*)importNode(child));
    return new ClassDeclaration(loc, id, superClass, importNode(node["body"]),
                                (TypeParameterDeclaration*)importNodeOrNullptr(node, "typeParameters"),
                                (TypeParameterInstantiation*)importNodeOrNullptr(node, "superTypeParameters"), implements);
}

AstNode* importVariableDeclaration(const json& node, AstSourceSpan loc)
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
    return new VariableDeclaration(loc, declarators, kind);
}

AstNode* importVariableDeclarator(const json& node, AstSourceSpan loc)
{
    AstNode* init;
    if (auto jinit = node["init"]; jinit != nullptr)
        init = importNode(jinit);
    else
        init = nullptr;
    return new VariableDeclarator(loc, importNode(node["id"]), init);
}

AstNode* importFunctionDeclaration(const json& node, AstSourceSpan loc)
{
    AstNode* id = node["id"] != nullptr ? importNode(node["id"]) : nullptr;
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    return new FunctionDeclaration(loc, id, params, importNode(node["body"]),
                                    (TypeParameterDeclaration*)importNodeOrNullptr(node, "typeParameters"),
                                    (TypeAnnotation*)importNodeOrNullptr(node, "returnType"),
                                    (bool)node["generator"], (bool)node["async"]);
}

AstNode* importSpreadElement(const json& node, AstSourceSpan loc)
{
    return new SpreadElement(loc, importNode(node["argument"]));
}

AstNode* importObjectPattern(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> properties;
    for (auto&& child : node["properties"])
        properties.push_back(importNode(child));
    return new ObjectPattern(loc, properties, (TypeAnnotation*)importNodeOrNullptr(node, "typeAnnotation"));
}

AstNode* importArrayPattern(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> elements;
    for (auto&& child : node["elements"])
        elements.push_back(importNode(child));
    return new ArrayPattern(loc, elements);
}

AstNode* importAssignmentPattern(const json& node, AstSourceSpan loc)
{
    return new AssignmentPattern(loc, importNode(node["left"]), importNode(node["right"]));
}

AstNode* importRestElement(const json& node, AstSourceSpan loc)
{
    return new RestElement(loc, importNode(node["argument"]), (TypeAnnotation*)importNodeOrNullptr(node, "typeAnnotation"));
}

AstNode* importMetaProperty(const json& node, AstSourceSpan loc)
{
    return new MetaProperty(loc, importNode(node["meta"]), importNode(node["property"]));
}

AstNode* importImportDeclaration(const json& node, AstSourceSpan loc)
{
    using Kind = ImportDeclaration::Kind;
    Kind kind;
    string kindStr = node["importKind"];
    if (kindStr == "value")
        kind = Kind::Value;
    else if (kindStr == "type")
        kind = Kind::Type;
    else
        throw runtime_error("Unknown import declaration kind " + kindStr);

    vector<AstNode*> specifiers;
    for (auto&& child : node["specifiers"])
        specifiers.push_back(importNode(child));
    return new ImportDeclaration(loc, specifiers, importNode(node["source"]), kind);
}

AstNode* importImportSpecifier(const json& node, AstSourceSpan loc)
{
    bool isTypeImport = false;
    const json& kind = node["importKind"];
    if (kind.is_string() && kind == "type")
        isTypeImport = true;

    return new ImportSpecifier(loc, importNode(node["local"]), importNode(node["imported"]), isTypeImport);
}

AstNode* importImportDefaultSpecifier(const json& node, AstSourceSpan loc)
{
    return new ImportDefaultSpecifier(loc, importNode(node["local"]));
}

AstNode* importImportNamespaceSpecifier(const json& node, AstSourceSpan loc)
{
    return new ImportNamespaceSpecifier(loc, importNode(node["local"]));
}

AstNode* importExportNamedDeclaration(const json& node, AstSourceSpan loc)
{
    using Kind = ExportNamedDeclaration::Kind;
    Kind kind;
    string kindStr = node["exportKind"];
    if (kindStr == "value")
        kind = Kind::Value;
    else if (kindStr == "type")
        kind = Kind::Type;
    else
        throw runtime_error("Unknown export named declaration kind " + kindStr);

    AstNode* declaration = importNodeOrNullptr(node, "declaration");
    AstNode* source = importNodeOrNullptr(node, "source");
    vector<AstNode*> specifiers;
    for (auto&& child : node["specifiers"])
        specifiers.push_back(importNode(child));
    return new ExportNamedDeclaration(loc, declaration, source, specifiers, kind);
}

AstNode* importExportDefaultDeclaration(const json& node, AstSourceSpan loc)
{
    return new ExportDefaultDeclaration(loc, importNode(node["declaration"]));
}

AstNode* importExportAllDeclaration(const json& node, AstSourceSpan loc)
{
    return new ExportAllDeclaration(loc, importNode(node["source"]));
}

AstNode* importExportSpecifier(const json& node, AstSourceSpan loc)
{
    return new ExportSpecifier(loc, importNode(node["local"]), importNode(node["exported"]));
}

AstNode* importExportDefaultSpecifier(const json& node, AstSourceSpan loc)
{
    return new ExportDefaultSpecifier(loc, importNode(node["exported"]));
}

AstNode* importTypeAnnotation(const json& node, AstSourceSpan loc)
{
    return new TypeAnnotation(loc, importNode(node["typeAnnotation"]));
}

AstNode* importGenericTypeAnnotation(const json& node, AstSourceSpan loc)
{
    return new GenericTypeAnnotation(loc, importNode(node["id"]), importNodeOrNullptr(node, "typeParameters"));
}

AstNode* importObjectTypeAnnotation(const json& node, AstSourceSpan loc)
{
    vector<ObjectTypeProperty*> properties;
    for (auto&& child : node["properties"])
        properties.push_back((ObjectTypeProperty*)importNode(child));
    vector<ObjectTypeIndexer*> indexers;
    for (auto&& child : node["indexers"])
        indexers.push_back((ObjectTypeIndexer*)importNode(child));
    return new ObjectTypeAnnotation(loc, properties, indexers, node["exact"]);
}

AstNode* importObjectTypeProperty(const json& node, AstSourceSpan loc)
{
    return new ObjectTypeProperty(loc, (Identifier*)importNode(node["key"]), importNode(node["value"]), node["optional"]);
}

AstNode* importObjectTypeIndexer(const json& node, AstSourceSpan loc)
{
    return new ObjectTypeIndexer(loc, (Identifier*)importNodeOrNullptr(node, "id"), importNode(node["key"]), importNode(node["value"]));
}

AstNode* importStringTypeAnnotation(const json&, AstSourceSpan loc)
{
    return new StringTypeAnnotation(loc);
}

AstNode* importNumberTypeAnnotation(const json&, AstSourceSpan loc)
{
    return new NumberTypeAnnotation(loc);
}

AstNode* importBooleanTypeAnnotation(const json&, AstSourceSpan loc)
{
    return new BooleanTypeAnnotation(loc);
}

AstNode* importVoidTypeAnnotation(const json&, AstSourceSpan loc)
{
    return new VoidTypeAnnotation(loc);
}

AstNode* importAnyTypeAnnotation(const json&, AstSourceSpan loc)
{
    return new AnyTypeAnnotation(loc);
}

AstNode* importExistsTypeAnnotation(const json&, AstSourceSpan loc)
{
    return new ExistsTypeAnnotation(loc);
}

AstNode* importMixedTypeAnnotation(const json&, AstSourceSpan loc)
{
    return new MixedTypeAnnotation(loc);
}

AstNode* importNullableTypeAnnotation(const json& node, AstSourceSpan loc)
{
    return new NullableTypeAnnotation(loc, importNode(node["typeAnnotation"]));
}

AstNode* importTupleTypeAnnotation(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> types;
    for (auto&& child : node["types"])
        types.push_back(importNode(child));
    return new TupleTypeAnnotation(loc, types);
}

AstNode* importUnionTypeAnnotation(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> types;
    for (auto&& child : node["types"])
        types.push_back(importNode(child));
    return new UnionTypeAnnotation(loc, types);
}

AstNode* importNullLiteralTypeAnnotation(const json&, AstSourceSpan loc)
{
    return new NullLiteralTypeAnnotation(loc);
}

AstNode* importNumberLiteralTypeAnnotation(const json& node, AstSourceSpan loc)
{
    return new NumberLiteralTypeAnnotation(loc, node["value"]);
}

AstNode* importStringLiteralTypeAnnotation(const json& node, AstSourceSpan loc)
{
    return new StringLiteralTypeAnnotation(loc, node["value"]);
}

AstNode* importBooleanLiteralTypeAnnotation(const json& node, AstSourceSpan loc)
{
    return new BooleanLiteralTypeAnnotation(loc, node["value"]);
}


AstNode* importFunctionTypeAnnotation(const json& node, AstSourceSpan loc)
{
    vector<FunctionTypeParam*> params;
    for (auto&& child : node["params"])
        params.push_back((FunctionTypeParam*)importNode(child));
    return new FunctionTypeAnnotation(loc, params, (FunctionTypeParam*)importNodeOrNullptr(node, "rest"), importNode(node["returnType"]));
}

AstNode* importFunctionTypeParam(const json& node, AstSourceSpan loc)
{
    return new FunctionTypeParam(loc, (Identifier*)importNodeOrNullptr(node, "name"), importNode(node["typeAnnotation"]));
}

AstNode* importTypeParameterInstantiation(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    return new TypeParameterInstantiation(loc, params);
}

AstNode* importTypeParameterDeclaration(const json& node, AstSourceSpan loc)
{
    vector<AstNode*> params;
    for (auto&& child : node["params"])
        params.push_back(importNode(child));
    return new TypeParameterDeclaration(loc, params);
}

AstNode* importTypeParameter(const json& node, AstSourceSpan loc)
{
    return new TypeParameter(loc, node["name"]);
}

AstNode* importTypeAlias(const json& node, AstSourceSpan loc)
{
    return new TypeAlias(loc, (Identifier*)importNode(node["id"]), importNodeOrNullptr(node, "typeParameters"), importNode(node["right"]));
}

AstNode* importTypeCastExpression(const json& node, AstSourceSpan loc)
{
    return new TypeCastExpression(loc, importNode(node["expression"]), (TypeAnnotation*)importNode(node["typeAnnotation"]));
}

AstNode* importClassImplements(const json& node, AstSourceSpan loc)
{
    return new ClassImplements(loc, (Identifier*)importNode(node["id"]), (TypeParameterInstantiation*)importNodeOrNullptr(node, "typeParameters"));
}

AstNode* importQualifiedTypeIdentifier(const json& node, AstSourceSpan loc)
{
    return new QualifiedTypeIdentifier(loc, (Identifier*)importNode(node["qualification"]), (Identifier*)importNode(node["id"]));
}

AstNode* importTypeofTypeAnnotation(const json& node, AstSourceSpan loc)
{
    return new TypeofTypeAnnotation(loc, importNode(node["argument"]));
}

AstNode* importInterfaceDeclaration(const json& node, AstSourceSpan loc)
{
    vector<InterfaceExtends*> extends;
    for (auto&& child : node["extends"])
        extends.push_back((InterfaceExtends*)importNode(child));
    vector<InterfaceExtends*> mixins;
    for (auto&& child : node["mixins"])
        mixins.push_back((InterfaceExtends*)importNode(child));
    return new InterfaceDeclaration(loc, (Identifier*)importNode(node["id"]), (TypeParameterDeclaration*)importNodeOrNullptr(node, "typeParameters"),
                                    importNode(node["body"]), extends, mixins);
}

AstNode* importInterfaceExtends(const json& node, AstSourceSpan loc)
{
    return new InterfaceExtends(loc, (Identifier*)importNode(node["id"]), (TypeParameterInstantiation*)importNodeOrNullptr(node, "typeParameters"));
}
