#include "ast/import.hpp"
#include "ast/ast.hpp"
#include <cassert>
#include <unordered_map>

using namespace std;
using namespace v8;

class Module;

#define X(NODE) AstNode* import##NODE(const Local<Object>&, AstSourceSpan&);
IMPORTED_NODE_LIST(X)
#undef X

#define X(NODE) { #NODE, import##NODE },
static const unordered_map<string, AstNode* (*)(const Local<Object>&, AstSourceSpan&)> importFunctions = {
    IMPORTED_NODE_LIST(X)
};
#undef X

static bool hasKey(const Local<Object>& node, const char* key)
{
    Isolate* isolate = node->GetIsolate();
    auto keyStr = v8::String::NewFromOneByte(isolate, (uint8_t*)key, NewStringType::kInternalized).ToLocalChecked();
    return node->HasRealNamedProperty(isolate->GetCurrentContext(), keyStr).FromJust();
}

static Local<Value> getVal(const Local<Object>& node, const char* key)
{
    // TODO: We may want to create a string from OneByte instead of Utf8, if it's any faster. We can guarantee that AST node keys are ASCII anyways
    Isolate* isolate = node->GetIsolate();
    auto keyStr = v8::String::NewFromOneByte(isolate, (uint8_t*)key, NewStringType::kInternalized).ToLocalChecked();
    auto val = node->GetRealNamedProperty(isolate->GetCurrentContext(), keyStr).ToLocalChecked();
    assert(!val->IsUndefined());
    return val;
}

static optional<Local<Value>> tryGetVal(const Local<Object>& node, const char* key)
{
    Isolate* isolate = node->GetIsolate();
    auto keyStr = v8::String::NewFromOneByte(isolate, (uint8_t*)key, NewStringType::kInternalized).ToLocalChecked();
    auto val = node->GetRealNamedProperty(isolate->GetCurrentContext(), keyStr);
    if (val.IsEmpty())
        return nullopt;
    return val.ToLocalChecked();
}

static Local<Object> getObj(const Local<Object>& node, const char* key)
{
    auto val = getVal(node, key);
    assert(val->IsObject());
    return val.As<Object>();
}

static string getStr(const Local<Object>& node, const char* key)
{
    Isolate* isolate = node->GetIsolate();
    auto val = getVal(node, key);
    assert(val->IsString());
    String::Utf8Value strVal(isolate, val.As<String>());
    return string{*strVal};
}

static double getNumber(const Local<Object>& node, const char* key)
{
    Isolate* isolate = node->GetIsolate();
    auto val = getVal(node, key);
    assert(val->IsNumber());
    return val->NumberValue(isolate->GetCurrentContext()).FromJust();
}

static optional<string> tryGetStr(const Local<Object>& node, const char* key)
{
    Isolate* isolate = node->GetIsolate();
    auto val = getVal(node, key);
    if (val->IsNullOrUndefined())
        return nullopt;
    assert(val->IsString());
    String::Utf8Value strVal(isolate, val.As<String>());
    return string{*strVal};
}

static bool getBool(const Local<Object>& node, const char* key)
{
    Isolate* isolate = node->GetIsolate();
    auto val = getVal(node, key);
    assert(val->IsBoolean());
    return val->BooleanValue(isolate->GetCurrentContext()).FromJust();
}

static optional<bool> tryGetBool(const Local<Object>& node, const char* key)
{
    Isolate* isolate = node->GetIsolate();
    auto maybeVal = tryGetVal(node, key);
    if (!maybeVal.has_value())
        return nullopt;
    const auto& val = maybeVal.value();
    assert(val->IsBoolean());
    return val->BooleanValue(isolate->GetCurrentContext()).FromJust();
}

AstNode* importNode(const Local<Object>& node)
{
    auto val = getVal(node, "type");
    assert(val->IsString());
    String::Utf8Value type(node->GetIsolate(), val.As<String>());
    auto funIt = importFunctions.find(*type);
    if (funIt == importFunctions.end())
        throw runtime_error("Unknown node of type "s + *type + " in Babylon AST");
    auto loc = importLocation(node);
    return funIt->second(node, loc);
}

AstNode* importChild(const Local<Object>& node, const char* name)
{
    return importNode(getObj(node, name));
}

AstNode* importChildOrNullptr(const Local<Object>& node, const char* name)
{
    auto childNode = tryGetVal(node, name);
    if (!childNode.has_value())
        return nullptr;
    else if (childNode.value()->IsNull())
        return nullptr;
    else
        return importNode(childNode->As<Object>());
}

template <class T = AstNode>
vector<T*> importChildArray(const Local<Object>& node, const char* name)
{
    auto arrVal = getVal(node, name);
    assert(arrVal->IsArray());
    auto arr = arrVal.As<Array>();
    auto len = arr->Length();

    vector<T*> result;
    result.reserve(len);
    for (int i=0; i<len; ++i) {
        auto elem = arr->Get(i).As<Object>();
        if (elem->IsNull()) {
            result.push_back(nullptr);
        } else {
            result.push_back(reinterpret_cast<T*>(importNode(elem)));
        }
    }

    return result;
}

AstRoot* importBabylonAst(::Module& parentModule, v8::Local<v8::Object> astObj)
{
    auto program = getObj(astObj, "program");
    assert(getStr(program, "type") == "Program");

    auto loc = importLocation(program);
    return new AstRoot(loc, parentModule, importChildArray(program, "body"));
}

AstSourceSpan importLocation(const Local<Object>& node)
{
    Local<Object> nodeLoc = getObj(node, "loc");
    unsigned beginOff = getNumber(node, "start");
    unsigned endOff = getNumber(node, "end");
    Local<Object> nodeLocStart = getObj(nodeLoc, "start");
    unsigned beginLine = getNumber(nodeLocStart, "line");
    unsigned beginCol = getNumber(nodeLocStart, "column");
    Local<Object> nodeLocEnd = getObj(nodeLoc, "end");
    unsigned endLine = getNumber(nodeLocEnd, "line");
    unsigned endCol = getNumber(nodeLocEnd, "column");
    return {
        {beginOff, beginLine, beginCol},
        {endOff, endLine, endCol}
    };
}

AstNode* importIdentifier(const Local<Object>& node, AstSourceSpan& loc)
{
    bool isOptional = false;
    if (auto it = tryGetBool(node, "optional"); it.has_value())
        isOptional = *it;

    return new Identifier(loc, getStr(node, "name"), (TypeAnnotation*)importChildOrNullptr(node, "typeAnnotation"), isOptional);
}

AstNode* importStringLiteral(const Local<Object>& node, AstSourceSpan& loc)
{
    return new StringLiteral(loc, getStr(node, "value"));
}

AstNode* importRegExpLiteral(const Local<Object>& node, AstSourceSpan& loc)
{
    return new RegExpLiteral(loc, getStr(node, "pattern"), getStr(node, "flags"));
}

AstNode* importNullLiteral(const Local<Object>& node, AstSourceSpan& loc)
{
    return new NullLiteral(loc);
}

AstNode* importBooleanLiteral(const Local<Object>& node, AstSourceSpan& loc)
{
    return new BooleanLiteral(loc, getBool(node, "value"));
}

AstNode* importNumericLiteral(const Local<Object>& node, AstSourceSpan& loc)
{
    return new NumericLiteral(loc, getNumber(node, "value"));
}

AstNode* importTemplateLiteral(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TemplateLiteral(loc, importChildArray(node, "quasis"), importChildArray(node, "expressions"));
}

AstNode* importTemplateElement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TemplateElement(loc, getStr(getObj(node, "value"), "raw"), getBool(node, "tail"));
}

AstNode* importTaggedTemplateExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TaggedTemplateExpression(loc, importChild(node, "tag"), importChild(node, "quasi"));
}

AstNode* importExpressionStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ExpressionStatement(loc, importChild(node, "expression"));
}

AstNode* importEmptyStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new EmptyStatement(loc);
}

AstNode* importWithStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new WithStatement(loc, importChild(node, "object"), importChild(node, "body"));
}

AstNode* importDebuggerStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new DebuggerStatement(loc);
}

AstNode* importReturnStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* argument = importChildOrNullptr(node, "argument");
    return new ReturnStatement(loc, argument);
}

AstNode* importLabeledStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new LabeledStatement(loc, importChild(node, "label"), importChild(node, "body"));
}

AstNode* importBreakStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* label = importChildOrNullptr(node, "label");
    return new BreakStatement(loc, label);
}

AstNode* importContinueStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* label = importChildOrNullptr(node, "label");
    return new ContinueStatement(loc, label);
}

AstNode* importIfStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* alternate = importChildOrNullptr(node, "alternate");
    return new IfStatement(loc, importChild(node, "test"), importChild(node, "consequent"), alternate);
}

AstNode* importSwitchStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new SwitchStatement(loc, importChild(node, "discriminant"), importChildArray(node, "cases"));
}

AstNode* importSwitchCase(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* test = importChildOrNullptr(node, "test");
    return new SwitchCase(loc, test, importChildArray(node, "consequent"));
}

AstNode* importThrowStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ThrowStatement(loc, importChild(node, "argument"));
}

AstNode* importTryStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* handler = importChildOrNullptr(node, "handler");
    AstNode* finalizer = importChildOrNullptr(node, "finalizer");
    return new TryStatement(loc, importChild(node, "block"), handler, finalizer);
}

AstNode* importCatchClause(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* param = importChildOrNullptr(node, "param");
    return new CatchClause(loc, param, importChild(node, "body"));
}

AstNode* importWhileStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new WhileStatement(loc, importChild(node, "test"), importChild(node, "body"));
}

AstNode* importDoWhileStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new DoWhileStatement(loc, importChild(node, "test"), importChild(node, "body"));
}

AstNode* importForStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* init = importChildOrNullptr(node, "init");
    AstNode* test = importChildOrNullptr(node, "test");
    AstNode* update = importChildOrNullptr(node, "update");
    return new ForStatement(loc, init, test, update, importChild(node, "body"));
}

AstNode* importForInStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ForInStatement(loc, importChild(node, "left"), importChild(node, "right"), importChild(node, "body"));
}

AstNode* importForOfStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    bool await = false;
    if (auto it = tryGetBool(node, "await"); it.has_value())
        await = *it;
    return new ForOfStatement(loc, importChild(node, "left"), importChild(node, "right"), importChild(node, "body"), await);
}

AstNode* importBlockStatement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new BlockStatement(loc, importChildArray(node, "body"));
}

AstNode* importSuper(const Local<Object>&, AstSourceSpan& loc)
{
    return new Super(loc);
}

AstNode* importImport(const Local<Object>&, AstSourceSpan& loc)
{
    return new Import(loc);
}

AstNode* importThisExpression(const Local<Object>&, AstSourceSpan& loc)
{
    return new ThisExpression(loc);
}

AstNode* importArrowFunctionExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* id = importChildOrNullptr(node, "id");
    bool isExpression = false;
    if (auto it = tryGetBool(node, "expression"); it.has_value())
        isExpression = *it;
    return new ArrowFunctionExpression(loc, id, importChildArray(node, "params"), importChild(node, "body"),
                                        (TypeParameterDeclaration*)importChildOrNullptr(node, "typeParameters"),
                                        (TypeAnnotation*)importChildOrNullptr(node, "returnType"),
                                        getBool(node, "generator"), getBool(node, "async"), isExpression);
}

AstNode* importYieldExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* argument = importChildOrNullptr(node, "argument");
    return new YieldExpression(loc, argument, getBool(node, "delegate"));
}

AstNode* importAwaitExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* argument = importChildOrNullptr(node, "argument");
    return new AwaitExpression(loc, argument);
}

AstNode* importArrayExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ArrayExpression(loc, importChildArray(node, "elements"));
}

AstNode* importObjectExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ObjectExpression(loc, importChildArray(node, "properties"));
}

AstNode* importObjectProperty(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ObjectProperty(loc, importChild(node, "key"),
        importChild(node, "value"),
        getBool(node, "shorthand"),
        getBool(node, "computed"));
}

AstNode* importObjectMethod(const Local<Object>& node, AstSourceSpan& loc)
{
    using Kind = ObjectMethod::Kind;
    Kind kind;
    string kindStr = getStr(node, "kind");
    if (kindStr == "method")
        kind = Kind::Method;
    else if (kindStr == "get")
        kind = Kind::Get;
    else if (kindStr == "set")
        kind = Kind::Set;
    else
        throw runtime_error("Unknown private class method declaration kind " + kindStr);

    AstNode* id = importChildOrNullptr(node, "id");
    return new ObjectMethod(loc, id, importChildArray(node, "params"), importChild(node, "body"),
                            (TypeParameterDeclaration*)importChildOrNullptr(node, "typeParameters"),
                            (TypeAnnotation*)importChildOrNullptr(node, "returnType"),
                            importChild(node, "key"), kind,getBool(node, "generator"), getBool(node, "async"), getBool(node, "computed"));
}

AstNode* importFunctionExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* id = importChildOrNullptr(node, "id");
    return new FunctionExpression(loc, id, importChildArray(node, "params"), importChild(node, "body"),
                                    (TypeParameterDeclaration*)importChildOrNullptr(node, "typeParameters"),
                                    (TypeAnnotation*)importChildOrNullptr(node, "returnType"),
                                    getBool(node, "generator"), getBool(node, "async"));
}

AstNode* importUnaryExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    using Operator = UnaryExpression::Operator;
    Operator op;
    string opStr = getStr(node, "operator");
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
    return new UnaryExpression(loc, importChild(node, "argument"), op, getBool(node, "prefix"));
}

AstNode* importUpdateExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    using Operator = UpdateExpression::Operator;
    Operator op;
    string opStr = getStr(node, "operator");
    if (opStr == "++")
        op = Operator::Increment;
    else if (opStr == "--")
        op = Operator::Decrement;
    else
        throw runtime_error("Unknown update operator " + opStr);
    return new UpdateExpression(loc, importChild(node, "argument"), op, getBool(node, "prefix"));
}

AstNode* importBinaryExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    using Operator = BinaryExpression::Operator;
    Operator op;
    string opStr = getStr(node, "operator");
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
    return new BinaryExpression(loc, importChild(node, "left"), importChild(node, "right"), op);
}

AstNode* importAssignmentExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    using Operator = AssignmentExpression::Operator;
    Operator op;
    string opStr = getStr(node, "operator");
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
    return new AssignmentExpression(loc, importChild(node, "left"), importChild(node, "right"), op);
}

AstNode* importLogicalExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    using Operator = LogicalExpression::Operator;
    Operator op;
    string opStr = getStr(node, "operator");
    if (opStr == "||")
        op = Operator::Or;
    else if (opStr == "&&")
        op = Operator::And;
    else
        throw runtime_error("Unknown logical operator " + opStr);
    return new LogicalExpression(loc, importChild(node, "left"), importChild(node, "right"), op);
}

AstNode* importMemberExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    return new MemberExpression(loc, importChild(node, "object"), importChild(node, "property"), getBool(node, "computed"));
}

AstNode* importBindExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* object = importChildOrNullptr(node, "object");
    return new BindExpression(loc, object, importChild(node, "callee"));
}

AstNode* importConditionalExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ConditionalExpression(loc, importChild(node, "test"), importChild(node, "alternate"), importChild(node, "consequent"));
}

AstNode* importCallExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    return new CallExpression(loc, importChild(node, "callee"), importChildArray(node, "arguments"));
}

AstNode* importNewExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    return new NewExpression(loc, importChild(node, "callee"), importChildArray(node, "arguments"));
}

AstNode* importSequenceExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    return new SequenceExpression(loc, importChildArray(node, "expressions"));
}

AstNode* importDoExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    return new DoExpression(loc, importChild(node, "body"));
}

AstNode* importClassExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* id = importChildOrNullptr(node, "id");
    AstNode* superClass = importChildOrNullptr(node, "superClass");
    vector<ClassImplements*> implements;
    if (hasKey(node, "implements"))
        implements = importChildArray<ClassImplements>(node, "implements");
    return new ClassExpression(loc, id, superClass, importChild(node, "body"),
                                (TypeParameterDeclaration*)importChildOrNullptr(node, "typeParameters"),
                                (TypeParameterInstantiation*)importChildOrNullptr(node, "superTypeParameters"), move(implements));
}

AstNode* importClassBody(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ClassBody(loc, importChildArray(node, "body"));
}

AstNode* importClassMethod(const Local<Object>& node, AstSourceSpan& loc)
{
    using Kind = ClassMethod::Kind;
    Kind kind;
    string kindStr = getStr(node, "kind");
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

    AstNode* id = importChildOrNullptr(node, "id");
    return new ClassMethod(loc, id, importChildArray(node, "params"), importChild(node, "body"), importChild(node, "key"),
            (TypeParameterDeclaration*)importChildOrNullptr(node, "typeParameters"),
            (TypeAnnotation*)importChildOrNullptr(node, "returnType"),
            kind, getBool(node, "generator"), getBool(node, "async"), getBool(node, "computed"), getBool(node, "static"));
}

AstNode* importClassPrivateMethod(const Local<Object>& node, AstSourceSpan& loc)
{
    using Kind = ClassPrivateMethod::Kind;
    Kind kind;
    string kindStr = getStr(node, "kind");
    if (kindStr == "method")
        kind = Kind::Method;
    else if (kindStr == "get")
        kind = Kind::Get;
    else if (kindStr == "set")
        kind = Kind::Set;
    else
        throw runtime_error("Unknown private class method declaration kind " + kindStr);

    AstNode* id = importChildOrNullptr(node, "id");
    return new ClassPrivateMethod(loc, id, importChildArray(node, "params"), importChild(node, "body"), importChild(node, "key"),
            (TypeParameterDeclaration*)importChildOrNullptr(node, "typeParameters"),
            (TypeAnnotation*)importChildOrNullptr(node, "returnType"),
            kind, getBool(node, "generator"), getBool(node, "async"), getBool(node, "static"));
}

AstNode* importClassProperty(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ClassProperty(loc, importChild(node, "key"), importChildOrNullptr(node, "value"),
                            (TypeAnnotation*)importChildOrNullptr(node, "typeAnnotation"), getBool(node, "static"), getBool(node, "computed"));
}

AstNode* importClassPrivateProperty(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ClassPrivateProperty(loc, importChild(node, "key"), importChild(node, "value"),
                                    (TypeAnnotation*)importChildOrNullptr(node, "typeAnnotation"), getBool(node, "static"));
}

AstNode* importClassDeclaration(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* id = importChildOrNullptr(node, "id");
    AstNode* superClass = importChildOrNullptr(node, "superClass");
    vector<ClassImplements*> implements;
    if (hasKey(node, "implements"))
        implements = importChildArray<ClassImplements>(node, "implements");
   return new ClassDeclaration(loc, id, superClass, importChild(node, "body"),
                                (TypeParameterDeclaration*)importChildOrNullptr(node, "typeParameters"),
                                (TypeParameterInstantiation*)importChildOrNullptr(node, "superTypeParameters"), move(implements));
}

AstNode* importVariableDeclaration(const Local<Object>& node, AstSourceSpan& loc)
{
    using Kind = VariableDeclaration::Kind;
    Kind kind;
    string kindStr = getStr(node, "kind");
    if (kindStr == "var")
        kind = Kind::Var;
    else if (kindStr == "let")
        kind = Kind::Let;
    else if (kindStr == "const")
        kind = Kind::Const;
    else
        throw runtime_error("Unknown variable declaration kind " + kindStr);

    return new VariableDeclaration(loc, importChildArray(node, "declarations"), kind);
}

AstNode* importVariableDeclarator(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* init = importChildOrNullptr(node, "init");
    return new VariableDeclarator(loc, importChild(node, "id"), init);
}

AstNode* importFunctionDeclaration(const Local<Object>& node, AstSourceSpan& loc)
{
    AstNode* id = importChildOrNullptr(node, "id");
    return new FunctionDeclaration(loc, id, importChildArray(node, "params"), importChild(node, "body"),
                                    (TypeParameterDeclaration*)importChildOrNullptr(node, "typeParameters"),
                                    (TypeAnnotation*)importChildOrNullptr(node, "returnType"),
                                    getBool(node, "generator"), getBool(node, "async"));
}

AstNode* importSpreadElement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new SpreadElement(loc, importChild(node, "argument"));
}

AstNode* importObjectPattern(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ObjectPattern(loc, importChildArray(node, "properties"), (TypeAnnotation*)importChildOrNullptr(node, "typeAnnotation"));
}

AstNode* importArrayPattern(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ArrayPattern(loc, importChildArray(node, "elements"));
}

AstNode* importAssignmentPattern(const Local<Object>& node, AstSourceSpan& loc)
{
    return new AssignmentPattern(loc, importChild(node, "left"), importChild(node, "right"));
}

AstNode* importRestElement(const Local<Object>& node, AstSourceSpan& loc)
{
    return new RestElement(loc, importChild(node, "argument"), (TypeAnnotation*)importChildOrNullptr(node, "typeAnnotation"));
}

AstNode* importMetaProperty(const Local<Object>& node, AstSourceSpan& loc)
{
    return new MetaProperty(loc, importChild(node, "meta"), importChild(node, "property"));
}

AstNode* importImportDeclaration(const Local<Object>& node, AstSourceSpan& loc)
{
    using Kind = ImportDeclaration::Kind;
    Kind kind;
    string kindStr = getStr(node, "importKind");
    if (kindStr == "value")
        kind = Kind::Value;
    else if (kindStr == "type")
        kind = Kind::Type;
    else
        throw runtime_error("Unknown import declaration kind " + kindStr);

    return new ImportDeclaration(loc, importChildArray(node, "specifiers"), importChild(node, "source"), kind);
}

AstNode* importImportSpecifier(const Local<Object>& node, AstSourceSpan& loc)
{
    bool isTypeImport = false;
    auto kind = tryGetStr(node, "importKind");
    if (kind.has_value() && *kind == "type")
        isTypeImport = true;

    return new ImportSpecifier(loc, importChild(node, "local"), importChild(node, "imported"), isTypeImport);
}

AstNode* importImportDefaultSpecifier(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ImportDefaultSpecifier(loc, importChild(node, "local"));
}

AstNode* importImportNamespaceSpecifier(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ImportNamespaceSpecifier(loc, importChild(node, "local"));
}

AstNode* importExportNamedDeclaration(const Local<Object>& node, AstSourceSpan& loc)
{
    using Kind = ExportNamedDeclaration::Kind;
    Kind kind;
    string kindStr = getStr(node, "exportKind");
    if (kindStr == "value")
        kind = Kind::Value;
    else if (kindStr == "type")
        kind = Kind::Type;
    else
        throw runtime_error("Unknown export named declaration kind " + kindStr);

    AstNode* declaration = importChildOrNullptr(node, "declaration");
    AstNode* source = importChildOrNullptr(node, "source");
    return new ExportNamedDeclaration(loc, declaration, source, importChildArray(node, "specifiers"), kind);
}

AstNode* importExportDefaultDeclaration(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ExportDefaultDeclaration(loc, importChild(node, "declaration"));
}

AstNode* importExportAllDeclaration(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ExportAllDeclaration(loc, importChild(node, "source"));
}

AstNode* importExportSpecifier(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ExportSpecifier(loc, importChild(node, "local"), importChild(node, "exported"));
}

AstNode* importExportDefaultSpecifier(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ExportDefaultSpecifier(loc, importChild(node, "exported"));
}

AstNode* importTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TypeAnnotation(loc, importChild(node, "typeAnnotation"));
}

AstNode* importGenericTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new GenericTypeAnnotation(loc, importChild(node, "id"), importChildOrNullptr(node, "typeParameters"));
}

AstNode* importObjectTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    vector<ObjectTypeProperty*> properties = importChildArray<ObjectTypeProperty>(node, "properties");
    vector<ObjectTypeIndexer*> indexers = importChildArray<ObjectTypeIndexer>(node, "indexers");
    return new ObjectTypeAnnotation(loc, move(properties), move(indexers), getBool(node, "exact"));
}

AstNode* importObjectTypeProperty(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ObjectTypeProperty(loc, (Identifier*)importChild(node, "key"), importChild(node, "value"), getBool(node, "optional"));
}

AstNode* importObjectTypeIndexer(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ObjectTypeIndexer(loc, (Identifier*)importChildOrNullptr(node, "id"), importChild(node, "key"), importChild(node, "value"));
}

AstNode* importObjectTypeSpreadProperty(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ObjectTypeSpreadProperty(loc, importChild(node, "argument"));
}

AstNode* importStringTypeAnnotation(const Local<Object>&, AstSourceSpan& loc)
{
    return new StringTypeAnnotation(loc);
}

AstNode* importNumberTypeAnnotation(const Local<Object>&, AstSourceSpan& loc)
{
    return new NumberTypeAnnotation(loc);
}

AstNode* importBooleanTypeAnnotation(const Local<Object>&, AstSourceSpan& loc)
{
    return new BooleanTypeAnnotation(loc);
}

AstNode* importVoidTypeAnnotation(const Local<Object>&, AstSourceSpan& loc)
{
    return new VoidTypeAnnotation(loc);
}

AstNode* importAnyTypeAnnotation(const Local<Object>&, AstSourceSpan& loc)
{
    return new AnyTypeAnnotation(loc);
}

AstNode* importExistsTypeAnnotation(const Local<Object>&, AstSourceSpan& loc)
{
    return new ExistsTypeAnnotation(loc);
}

AstNode* importMixedTypeAnnotation(const Local<Object>&, AstSourceSpan& loc)
{
    return new MixedTypeAnnotation(loc);
}

AstNode* importNullableTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new NullableTypeAnnotation(loc, importChild(node, "typeAnnotation"));
}

AstNode* importArrayTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ArrayTypeAnnotation(loc, importChild(node, "elementType"));
}

AstNode* importTupleTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TupleTypeAnnotation(loc, importChildArray(node, "types"));
}

AstNode* importUnionTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new UnionTypeAnnotation(loc, importChildArray(node, "types"));
}

AstNode* importNullLiteralTypeAnnotation(const Local<Object>&, AstSourceSpan& loc)
{
    return new NullLiteralTypeAnnotation(loc);
}

AstNode* importNumberLiteralTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new NumberLiteralTypeAnnotation(loc, getNumber(node, "value"));
}

AstNode* importStringLiteralTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new StringLiteralTypeAnnotation(loc, getStr(node, "value"));
}

AstNode* importBooleanLiteralTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new BooleanLiteralTypeAnnotation(loc, getBool(node, "value"));
}


AstNode* importFunctionTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    vector<FunctionTypeParam*> params = importChildArray<FunctionTypeParam>(node, "params");
    return new FunctionTypeAnnotation(loc, move(params), (FunctionTypeParam*)importChildOrNullptr(node, "rest"), importChild(node, "returnType"));
}

AstNode* importFunctionTypeParam(const Local<Object>& node, AstSourceSpan& loc)
{
    return new FunctionTypeParam(loc, (Identifier*)importChildOrNullptr(node, "name"), importChild(node, "typeAnnotation"));
}

AstNode* importTypeParameterInstantiation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TypeParameterInstantiation(loc, importChildArray(node, "params"));
}

AstNode* importTypeParameterDeclaration(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TypeParameterDeclaration(loc, importChildArray(node, "params"));
}

AstNode* importTypeParameter(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TypeParameter(loc, getStr(node, "name"));
}

AstNode* importTypeAlias(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TypeAlias(loc, (Identifier*)importChild(node, "id"), importChildOrNullptr(node, "typeParameters"), importChild(node, "right"));
}

AstNode* importTypeCastExpression(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TypeCastExpression(loc, importChild(node, "expression"), (TypeAnnotation*)importChild(node, "typeAnnotation"));
}

AstNode* importClassImplements(const Local<Object>& node, AstSourceSpan& loc)
{
    return new ClassImplements(loc, (Identifier*)importChild(node, "id"), (TypeParameterInstantiation*)importChildOrNullptr(node, "typeParameters"));
}

AstNode* importQualifiedTypeIdentifier(const Local<Object>& node, AstSourceSpan& loc)
{
    return new QualifiedTypeIdentifier(loc, (Identifier*)importChild(node, "qualification"), (Identifier*)importChild(node, "id"));
}

AstNode* importTypeofTypeAnnotation(const Local<Object>& node, AstSourceSpan& loc)
{
    return new TypeofTypeAnnotation(loc, importChild(node, "argument"));
}

AstNode* importInterfaceDeclaration(const Local<Object>& node, AstSourceSpan& loc)
{
    vector<InterfaceExtends*> extends = importChildArray<InterfaceExtends>(node, "extends");
    vector<InterfaceExtends*> mixins = importChildArray<InterfaceExtends>(node, "mixins");
    return new InterfaceDeclaration(loc, (Identifier*)importChild(node, "id"), (TypeParameterDeclaration*)importChildOrNullptr(node, "typeParameters"),
                                    importChild(node, "body"), move(extends), move(mixins));
}

AstNode* importInterfaceExtends(const Local<Object>& node, AstSourceSpan& loc)
{
    return new InterfaceExtends(loc, (Identifier*)importChild(node, "id"), (TypeParameterInstantiation*)importChildOrNullptr(node, "typeParameters"));
}
