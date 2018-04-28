#include "identresolution.hpp"
#include "ast/ast.hpp"
#include "ast/walk.hpp"
#include "analyze/astqueries.hpp"
#include "reporting.hpp"
#include "module/moduleresolver.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace std;

struct Scope {
    unordered_map<string, Identifier*> declarations;
    bool isBlockScope;
};

static bool isFullScopeNodeType(AstNodeType type)
{
    return type == AstNodeType::Root
            || type == AstNodeType::ClassBody
            || type == AstNodeType::ObjectMethod
            || type == AstNodeType::ArrowFunctionExpression
            || type == AstNodeType::FunctionExpression
            || type == AstNodeType::ClassMethod
            || type == AstNodeType::ClassPrivateMethod
            || type == AstNodeType::FunctionDeclaration;
}

static bool isBlockScopeNodeType(AstNodeType type)
{
    return type == AstNodeType::BlockStatement
            || type == AstNodeType::CatchClause
            || type == AstNodeType::ForStatement
            || type == AstNodeType::ForInStatement
            || type == AstNodeType::ForOfStatement
            || type == AstNodeType::SwitchCase;
}

// True if this identifier is not a local declaration, but refers to an exported or imported name
static bool isExternalIdentifier(Identifier& node)
{
    auto parent = node.getParent();
    if (parent->getType() == AstNodeType::ImportSpecifier) {
        return ((ImportSpecifier*)parent)->getImported() == &node;
    } else if (parent->getType() == AstNodeType::ExportDefaultSpecifier) {
        return ((ExportDefaultSpecifier*)parent)->getExported() == &node;
    } else if (parent->getType() == AstNodeType::ExportSpecifier) {
        auto exportNode = (ExportSpecifier*)parent;
        if (exportNode->getExported() == &node)
            return true;
        else if (exportNode->getLocal() == &node)
            return exportNode->getLocal()->getName() == exportNode->getExported()->getName();
        else
            return false;
    } else {
        return false;
    }
}

// True if this identifier refers to an object property through a member expression, and not to a value in a local scope
static bool isMemberExpressionPropertyIdentifier(Identifier& node)
{
    auto parent = node.getParent();
    if (parent->getType() != AstNodeType::MemberExpression)
        return false;
    return ((MemberExpression*)parent)->getProperty() == &node;
}

// True if this identifier is a declaration for a "var" variable (not block scoped)
static bool isVarDeclarationIdentifier(Identifier& node)
{
    auto parent = node.getParent();
    if (parent->getType() != AstNodeType::VariableDeclarator)
        return false;
    auto grandpa = parent->getParent();
    if (grandpa->getType() != AstNodeType::VariableDeclaration)
        return false;
    auto varNode = (VariableDeclaration*)grandpa;
    return varNode->getKind() == VariableDeclaration::Kind::Var;
}

static void addDeclaration(vector<Scope>& scopes, Identifier& id)
{
    assert(id.getType() == AstNodeType::Identifier);
    if (isVarDeclarationIdentifier(id)) {
        size_t i = scopes.size() - 1;
        while (scopes[i].isBlockScope) {
            assert(i>0);
            i -= 1;
        }
        scopes[i].declarations.insert({id.getName(), &id});
    } else {
        scopes.back().declarations.insert({id.getName(), &id});
    }
}

static void findImportDeclarations(vector<Scope>& scopes, AstNode& node)
{
    auto specifiers = ((ImportDeclaration&)node).getSpecifiers();
    for (auto specifier : specifiers) {
        switch (specifier->getType()) {
        case AstNodeType::ImportDefaultSpecifier:
            addDeclaration(scopes, *((ImportDefaultSpecifier*)specifier)->getLocal());
            break;
        case AstNodeType::ImportSpecifier:
            addDeclaration(scopes, *((ImportSpecifier*)specifier)->getLocal());
            break;
        case AstNodeType::ImportNamespaceSpecifier:
            addDeclaration(scopes, *((ImportNamespaceSpecifier*)specifier)->getLocal());
            break;
        default: break;
        }
    }
}

static void findIdentifierOrFancyDeclarations(vector<Scope>& scopes, AstNode& id)
{
    auto type = id.getType();
    if (type == AstNodeType::Identifier) {
        addDeclaration(scopes, (Identifier&)id);
    } else if (type == AstNodeType::VariableDeclaration) {
        auto declarators = ((VariableDeclaration&)id).getDeclarators();
        for (auto declarator : declarators)
            findIdentifierOrFancyDeclarations(scopes, *((VariableDeclarator*)declarator)->getId());
    } else if (type == AstNodeType::ObjectPattern) {
        auto& patternNode = (ObjectPattern&)id;
        for (auto prop : patternNode.getProperties())
            findIdentifierOrFancyDeclarations(scopes, *prop->getKey());
    } else if (type == AstNodeType::ArrayPattern) {
        auto& patternNode = (ArrayPattern&)id;
        for (auto elem : patternNode.getElements())
            findIdentifierOrFancyDeclarations(scopes, *elem);
    } else if (type == AstNodeType::AssignmentPattern) {
        auto& patternNode = (AssignmentPattern&)id;
        findIdentifierOrFancyDeclarations(scopes, *patternNode.getLeft());
    } else if (type == AstNodeType::RestElement) {
        auto& patternNode = (RestElement&)id;
        findIdentifierOrFancyDeclarations(scopes, *patternNode.getArgument());
    } else {
        throw std::runtime_error("Unexpected id type for identifier or object pattern: "s+id.getTypeName());
    }
}

static void findScopeDeclarations(vector<Scope>& scopes, AstNode& scopeNode)
{
    vector<AstNode*> children;
    switch (scopeNode.getType()) {
    case AstNodeType::FunctionExpression:
        // FunctionExpressions don't belong in a parent scope, their name is in their own scope
        if (auto id = ((FunctionExpression&)scopeNode).getId(); id)
            addDeclaration(scopes, *id);
    case AstNodeType::FunctionDeclaration:
    case AstNodeType::ArrowFunctionExpression:
    case AstNodeType::ObjectMethod:
    case AstNodeType::ClassMethod:
    case AstNodeType::ClassPrivateMethod:
    {
        auto& fun = (Function&)scopeNode;
        children = fun.getBody()->getChildren();
        for (auto param : fun.getParams()) {
            // paran can also be an AssignmentPattern!
            findIdentifierOrFancyDeclarations(scopes, *param);
        }
        break;
    }
    case AstNodeType::CatchClause:
    {
        auto& clause = (CatchClause&)scopeNode;
        findIdentifierOrFancyDeclarations(scopes, *clause.getParam());
        break;
    }
    case AstNodeType::ForInStatement:
    {
        auto& node = (ForInStatement&)scopeNode;
        findIdentifierOrFancyDeclarations(scopes, *node.getLeft());
        break;
    }
    case AstNodeType::ForOfStatement:
    {
        auto& node = (ForOfStatement&)scopeNode;
        findIdentifierOrFancyDeclarations(scopes, *node.getLeft());
        break;
    }
    case AstNodeType::ForStatement:
    {
        auto& node = (ForStatement&)scopeNode;
        if (auto init = node.getInit(); init && init->getType() == AstNodeType::VariableDeclaration)
            findIdentifierOrFancyDeclarations(scopes, *init);
        break;
    }
    default:
        children = scopeNode.getChildren();
    }

    for (auto child : scopeNode.getChildren()) {
        if (!child)
            continue;
        switch (child->getType()) {
        case AstNodeType::ClassDeclaration:
            addDeclaration(scopes, *((ClassDeclaration*)child)->getId());
            break;
        case AstNodeType::FunctionDeclaration:
            addDeclaration(scopes, *((FunctionDeclaration*)child)->getId());
            break;
        case AstNodeType::ExportNamedDeclaration:
        case AstNodeType::ExportDefaultDeclaration:
        case AstNodeType::ExportAllDeclaration:
            findScopeDeclarations(scopes, *child);
            break;
        case AstNodeType::ImportDeclaration:
            findImportDeclarations(scopes, *child);
            break;
        case AstNodeType::VariableDeclaration:
            findIdentifierOrFancyDeclarations(scopes, *child);
            break;
        default: break;
        }
    }
}

IdentifierResolutionResult resolveModuleIdentifiers(v8::Local<v8::Context> context, AstRoot* ast)
{
    int fullScopeLevel = 0;
    vector<Scope> scopeDeclarations;
    unordered_map<Identifier*, Identifier*> identifierTargets;
    unordered_set<string> unresolvedTopLevelIdentifiers;

    std::function<void(AstNode&)> walkScopes;
    walkScopes = [&](AstNode& node){
        //trace("[Scope level "s+to_string(fullScopeLevel)+"/"+to_string(scopeDeclarations.size())+"] "+node.getTypeName()+" node");
        bool isFullScopeNode = isFullScopeNodeType(node.getType());
        bool isBlockScopeNode = isBlockScopeNodeType(node.getType());

        if (isFullScopeNode || isBlockScopeNode) {
            fullScopeLevel += isFullScopeNode;
            Scope newScope;
            newScope.isBlockScope = isBlockScopeNode;
            scopeDeclarations.push_back(newScope);
            findScopeDeclarations(scopeDeclarations, node);
        } else if (node.getType() == AstNodeType::Identifier) {
            Identifier& identifier = reinterpret_cast<Identifier&>(node);
            const auto& name = identifier.getName();
            bool found = false;
            for (const auto& scope : scopeDeclarations) {
                if (auto it = scope.declarations.find(name); it != scope.declarations.end()) {
                    identifierTargets.insert({&identifier, it->second});
                    found = true;
                    break;
                }
            }
            if (!found && (isExternalIdentifier(identifier) || isUnscopedPropertyOrMethodIdentifier(identifier))) {
                identifierTargets.insert({&identifier, &identifier});
                found = true;
            }
            if (!found && fullScopeLevel == 1 && !isMemberExpressionPropertyIdentifier(identifier))
                unresolvedTopLevelIdentifiers.insert(name);
        }

        for (auto child : node.getChildren())
            if (child)
                walkScopes(*child);

        if (isFullScopeNode || isBlockScopeNode) {
            fullScopeLevel -= isFullScopeNode;
            scopeDeclarations.pop_back();
        }
    };
    walkScopes(*ast);

    using namespace v8;

    Isolate* isolate = context->GetIsolate();
    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);
    Context::Scope contextScope(context);
    Local<Object> global = context->Global();
    std::vector<std::string> missingGlobalIdentifiers;
    for (auto name : unresolvedTopLevelIdentifiers) {
        Local<String> nameStr = String::NewFromUtf8(isolate, name.c_str());

        if (global->Has(nameStr))
            continue;

        warn("Couldn't find declaration for top-level identifier "+name);
        missingGlobalIdentifiers.push_back(std::move(name));
    }

    return {identifierTargets, missingGlobalIdentifiers};
}

void defineMissingGlobalIdentifiers(v8::Local<v8::Context> context, const std::vector<string>& missingGlobalIdentifiers)
{
    using namespace v8;

    Isolate* isolate = context->GetIsolate();
    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);
    Context::Scope contextScope(context);
    Local<Object> global = context->Global();

    // This object will pretend to have the value undefined, but when any property is accessed it returns itself!
    auto getter = [](auto, const PropertyCallbackInfo<Value>& info) {
        info.GetReturnValue().Set(info.This());
    };
    Local<ObjectTemplate> poserObjectTemplate = ObjectTemplate::New(isolate);
    poserObjectTemplate->SetHandler(NamedPropertyHandlerConfiguration(getter));
    poserObjectTemplate->SetHandler(IndexedPropertyHandlerConfiguration(getter));
    poserObjectTemplate->SetCallAsFunctionHandler([](const FunctionCallbackInfo<Value>& info){
        info.GetReturnValue().Set(info.This());
    });
    poserObjectTemplate->MarkAsUndetectable();
    Local<Object> poserObject = poserObjectTemplate->NewInstance(context).ToLocalChecked();

    for (auto name : missingGlobalIdentifiers) {
        Local<String> nameStr = String::NewFromUtf8(isolate, name.c_str());
        if (!global->Has(nameStr))
            global->Set(nameStr, poserObject);
    }
}

AstNode *resolveImportedIdentifierDeclaration(ImportSpecifier &importSpec)
{
    Module& sourceMod = importSpec.getParentModule();
    auto importDeclNode = (ImportDeclaration*)importSpec.getParent();
    const string& source = importDeclNode->getSource();

    if (NativeModule::hasModule(source))
        return nullptr;
    Module& importedMod = reinterpret_cast<Module&>(ModuleResolver::getModule(sourceMod, source, true));

    /// TODO: This
    throw std::runtime_error("Not implemented!");
}
