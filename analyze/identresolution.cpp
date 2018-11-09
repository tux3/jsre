#include "identresolution.hpp"
#include "ast/ast.hpp"
#include "ast/walk.hpp"
#include "analyze/astqueries.hpp"
#include "utils/reporting.hpp"
#include "module/moduleresolver.hpp"
#include <cassert>
#include <vector>
#include <unordered_map>

using namespace std;

struct Scope {
    unordered_map<string, Identifier*> declarations;
    bool isBlockScope;
    // vars may shadow each other in the same scope, so we prefix them with their index to support multiple vars per scope
    int writeCurrentVarDeclaration = 0;
    int readCurrentVarDeclaration = 0;
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

static bool isPartialScopeNodeType(AstNodeType type)
{
    return type == AstNodeType::BlockStatement
            || type == AstNodeType::CatchClause
            || type == AstNodeType::ForStatement
            || type == AstNodeType::ForInStatement
            || type == AstNodeType::ForOfStatement
            || type == AstNodeType::SwitchCase
            || type == AstNodeType::TypeAlias; // Not a traditional scope!
}

// True if this identifier is a property in a member expression or a qualified expression, and doesn't refer to a value in a local scope
static bool isMemberPropertyOrQualifiedIdentifier(Identifier& node)
{
    auto parent = node.getParent();
    if (parent->getType() == AstNodeType::MemberExpression)
        return ((MemberExpression*)parent)->getProperty() == &node;
    else if (parent->getType() == AstNodeType::QualifiedTypeIdentifier)
        return ((QualifiedTypeIdentifier*)parent)->getId() == &node;
    else
        return false;
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
        // We insert in every relevant scope, to allow per-scope shadowing of vars (e.g. "if (x) {var i; foo(i);} var i; bar(i);")
        while (scopes[i].isBlockScope) {
            assert(i>0);
            ++scopes[i].writeCurrentVarDeclaration;
            auto prefixedName = to_string(scopes[i].writeCurrentVarDeclaration)+id.getName();
            scopes[i].declarations.insert({prefixedName, &id});
            i -= 1;
        }
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
        for (auto prop : patternNode.getProperties()) {
            if (prop->getType() == AstNodeType::ObjectProperty)
                findIdentifierOrFancyDeclarations(scopes, *((ObjectProperty*)prop)->getValue());
            else if (prop->getType() == AstNodeType::RestElement)
                findIdentifierOrFancyDeclarations(scopes, *((RestElement*)prop)->getArgument());
            else
                trace(id, "Unhandled type "s+prop->getTypeName()+" while resolving object pattern declarations");
        }
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

static void findTypeParameterDeclarations(vector<Scope>& scopes, AstNode& node)
{
    const auto& params = ((TypeParameterDeclaration&)node).getParams();
    for (auto param : params)
        addDeclaration(scopes, *param->getName());
}

static void findScopeDeclarations(vector<Scope>& scopes, AstNode& scopeNode)
{
    vector<AstNode*> children;
    switch (scopeNode.getType()) {
    case AstNodeType::FunctionExpression:
        // FunctionExpressions don't belong in a parent scope, their name is in their own scope
        if (auto id = ((FunctionExpression&)scopeNode).getId(); id)
            addDeclaration(scopes, *id);
    [[fallthrough]];
    case AstNodeType::FunctionDeclaration:
    case AstNodeType::ArrowFunctionExpression:
    case AstNodeType::ObjectMethod:
    case AstNodeType::ClassMethod:
    case AstNodeType::ClassPrivateMethod:
    {
        auto& fun = (Function&)scopeNode;
        children = {fun.getBody()}; // We let the body be its own scope, so that locals can shadow arguments
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

    scopeNode.applyChildren([&](AstNode* child) {
        switch (child->getType()) {
        case AstNodeType::ClassDeclaration:
            addDeclaration(scopes, *((ClassDeclaration*)child)->getId());
            if (auto typeParameters = ((ClassDeclaration*)child)->getTypeParameters())
                findTypeParameterDeclarations(scopes, *typeParameters);
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
        case AstNodeType::InterfaceDeclaration:
            addDeclaration(scopes, *((InterfaceDeclaration*)child)->getId());
            break;
        case AstNodeType::TypeAlias:
            addDeclaration(scopes, *((TypeAlias*)child)->getId());
            break;
        case AstNodeType::TypeParameterDeclaration:
            findTypeParameterDeclarations(scopes, *child);
            break;
        default: break;
        }
        return true;
    });
}

IdentifierResolutionResult resolveModuleIdentifiers(v8::Local<v8::Context> context, AstRoot& ast)
{
    int fullScopeLevel = 0;
    vector<Scope> scopeDeclarations;
    unordered_map<Identifier*, Identifier*> identifierTargets;
    unordered_map<string, Identifier*> unresolvedTopLevelIdentifiers;

    std::function<void(AstNode&)> walkScopes;
    walkScopes = [&](AstNode& node){
        //trace("[Scope level "s+to_string(fullScopeLevel)+"/"+to_string(scopeDeclarations.size())+"] "+node.getTypeName()+" node");
        bool isFullScopeNode = isFullScopeNodeType(node.getType());
        bool isBlockScopeNode = isPartialScopeNodeType(node.getType());

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
            for (ssize_t i = (ssize_t)scopeDeclarations.size()-1; i>=0; --i) {
                const auto& scope = scopeDeclarations[(size_t)i];
                // Search for all possible shadowing var declarations first, then nornal let/const
                for (int varIndex = scope.readCurrentVarDeclaration; varIndex>0; --varIndex) {
                    auto prefixedName = to_string(varIndex)+name;
                    if (auto it = scope.declarations.find(prefixedName); it != scope.declarations.end()) {
                        identifierTargets.insert({&identifier, it->second});
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
                // Only if we're not refering to a preceding var, we might be referencing a hoisted var
                for (int varIndex = scope.readCurrentVarDeclaration+1; varIndex<=scope.writeCurrentVarDeclaration; ++varIndex) {
                    auto prefixedName = to_string(varIndex)+name;
                    if (auto it = scope.declarations.find(prefixedName); it != scope.declarations.end()) {
                        identifierTargets.insert({&identifier, it->second});
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
                // If it's not a var, we can check other normal declarations
                if (auto it = scope.declarations.find(name); it != scope.declarations.end()) {
                    identifierTargets.insert({&identifier, it->second});
                    found = true;
                    break;
                }
            }
            if (!found && (isExternalIdentifier(identifier)
                           || isUnscopedPropertyOrMethodIdentifier(identifier)
                           || isUnscopedTypeIdentifier(identifier))) {
                identifierTargets.insert({&identifier, &identifier});
                found = true;
            }
            if (!found && fullScopeLevel == 1 && !isMemberPropertyOrQualifiedIdentifier(identifier))
                unresolvedTopLevelIdentifiers.insert({name, &identifier});
        }

        node.applyChildren([&](AstNode* child) {
            walkScopes(*child);
            return true;
        });

        // vars are allowed to shadow each other, so we have to keep track of the current number of vars we saw
        if (node.getType() == AstNodeType::VariableDeclarator) {
            auto parent = node.getParent();
            if (parent->getType() == AstNodeType::VariableDeclaration) {
                auto varNode = (VariableDeclaration*)parent;
                if (varNode->getKind() == VariableDeclaration::Kind::Var) {
                    for (size_t i = scopeDeclarations.size() - 1; scopeDeclarations[i].isBlockScope; --i)
                        ++scopeDeclarations[i].readCurrentVarDeclaration;
                }
            }
        }

        if (isFullScopeNode || isBlockScopeNode) {
            // Make sure all declarations are registered as referring to themselves
            for (const auto& pair : scopeDeclarations.back().declarations)
                identifierTargets[pair.second] = pair.second;

            fullScopeLevel -= isFullScopeNode;
            scopeDeclarations.pop_back();
        }
    };
    walkScopes(ast);

    using namespace v8;

    Isolate* isolate = context->GetIsolate();
    Isolate::Scope isolateScope(isolate);
    HandleScope handleScope(isolate);
    Context::Scope contextScope(context);
    Local<Object> global = context->Global();
    std::vector<std::string> missingGlobalIdentifiers;
    for (auto elem : unresolvedTopLevelIdentifiers) {
        auto name = elem.first;
        Local<String> nameStr = String::NewFromUtf8(isolate, name.c_str());

        if (global->Has(context, nameStr).FromJust())
            continue;

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
        if (!global->Has(context, nameStr).FromJust())
            global->Set(nameStr, poserObject);
    }
}

AstNode *resolveImportedIdentifierDeclaration(AstNode &importSpec)
{
    // TODO: Make some attempt at resolving exported identifiers of non-ES6 modules

    Module& sourceMod = importSpec.getParentModule();
    string source, importSpecName;

    if (importSpec.getType() == AstNodeType::ExportSpecifier) {
        auto exportDeclNode = (ExportNamedDeclaration*)importSpec.getParent();
        auto sourceLiteral = (StringLiteral*)exportDeclNode->getSource();
        assert(sourceLiteral);
        source = sourceLiteral->getValue();
        importSpecName = ((ExportSpecifier&)importSpec).getLocal()->getName();
    } else {
        assert(importSpec.getParent()->getType() == AstNodeType::ImportDeclaration);
        auto importDeclNode = (ImportDeclaration*)importSpec.getParent();
        source = importDeclNode->getSource();
        if (importSpec.getType() == AstNodeType::ImportSpecifier)
            importSpecName = ((ImportSpecifier&)importSpec).getImported()->getName();
    }

    if (NativeModule::hasModule(source))
        return nullptr;
    Module& importedMod = reinterpret_cast<Module&>(ModuleResolver::getModule(sourceMod, source, true));

    AstNode* exported = nullptr;
    if (importSpec.getType() == AstNodeType::ImportDefaultSpecifier) {
        walkAst(importedMod.getAst(), [&](AstNode& node) {
            if (node.getType() == AstNodeType::ExportDefaultDeclaration) {
                exported = ((ExportDefaultDeclaration&)node).getDeclaration();
            } else if (node.getType() == AstNodeType::ExportSpecifier) {
                auto& specifier = (ExportSpecifier&)node;
                if (specifier.getExported()->getName() == "default")
                    exported = specifier.getLocal();
            }
        }, [&](AstNode& node) {
            if (node.getType() == AstNodeType::ExportDefaultDeclaration || node.getType() == AstNodeType::ExportSpecifier)
                return WalkDecision::WalkOver;
            else if (node.getType() == AstNodeType::ExportNamedDeclaration)
                return WalkDecision::SkipInto;
            else
                return WalkDecision::SkipOver;
        });
    } else if (importSpec.getType() == AstNodeType::ImportSpecifier || importSpec.getType() == AstNodeType::ExportSpecifier) {
        walkAst(importedMod.getAst(), [&](AstNode& node) {
            if (node.getType() == AstNodeType::ExportAllDeclaration) {
                exported = &node;
            } else if (node.getType() == AstNodeType::ExportSpecifier) {
                auto& specifier = (ExportSpecifier&)node;
                if (specifier.getExported()->getName() == importSpecName)
                    exported = specifier.getLocal();
            } else if (node.getType() == AstNodeType::TypeAlias) {
                auto& typeAlias = (TypeAlias&)node;
                if (typeAlias.getId()->getName() == importSpecName)
                    exported = &typeAlias;
            } else if (node.getType() == AstNodeType::InterfaceDeclaration) {
                auto& decl = (InterfaceDeclaration&)node;
                if (decl.getId()->getName() == importSpecName)
                    exported = &decl;
            } else if (node.getType() == AstNodeType::FunctionDeclaration) {
                auto& decl = (FunctionDeclaration&)node;
                if (decl.getId()->getName() == importSpecName)
                    exported = &decl;
            } else if (node.getType() == AstNodeType::ClassDeclaration) {
                auto& decl = (ClassDeclaration&)node;
                if (decl.getId()->getName() == importSpecName)
                    exported = &decl;
            } else if (node.getType() == AstNodeType::VariableDeclarator) {
                auto& decl = (VariableDeclarator&)node;
                if (decl.getId()->getType() == AstNodeType::Identifier && ((Identifier*)decl.getId())->getName() == importSpecName)
                    exported = &decl;
            }
        }, [&](AstNode& node) {
            if (node.getType() == AstNodeType::ExportAllDeclaration
                    || node.getType() == AstNodeType::ExportSpecifier
                    || node.getType() == AstNodeType::TypeAlias
                    || node.getType() == AstNodeType::InterfaceDeclaration
                    || node.getType() == AstNodeType::FunctionDeclaration
                    || node.getType() == AstNodeType::ClassDeclaration
                    || node.getType() == AstNodeType::VariableDeclarator)
                return WalkDecision::WalkOver;
            else if (node.getType() == AstNodeType::ExportNamedDeclaration
                     || node.getType() == AstNodeType::VariableDeclaration)
                return WalkDecision::SkipInto;
            else
                return WalkDecision::SkipOver;
        });
    } else {
        assert(false && "Unexpected import specifier type");
    }

    if (exported && exported->getType() == AstNodeType::Identifier) {
        const auto& resolvedLocals = importedMod.getResolvedLocalIdentifiers();
        const auto& resolvedIt = resolvedLocals.find((Identifier*)exported);
        if (resolvedIt != resolvedLocals.end())
            exported = resolvedIt->second;
    }

    return exported;
}

AstNode *resolveIdentifierDeclaration(Identifier &identifier)
{
    auto& module = identifier.getParentModule();
    const auto& resolvedIds = module.getResolvedLocalIdentifiers();
    const auto& it = resolvedIds.find(&identifier);
    if (it == resolvedIds.end())
        return nullptr;

    AstNode* decl = it->second->getParent();
    while (decl->getType() == AstNodeType::ImportDefaultSpecifier
           || decl->getType() == AstNodeType::ImportSpecifier
           || (decl->getType() == AstNodeType::ExportSpecifier
               && ((ExportNamedDeclaration*)decl->getParent())->getSource())) {
        decl = resolveImportedIdentifierDeclaration(*decl);
        if (!decl)
            return nullptr;

        // TODO: FIXME: This is a hack because sometimes when looking up a class we would get the identifier instead
        if (decl->getType() == AstNodeType::Identifier)
            decl = decl->getParent();
    }

    // We consider parameters of a function to be their own declaration, not the function!
    // Furthermore, they may shadow the function name, so we need to check them by name
    if (isFunctionNode(*decl)) {
        Function* declFun = (Function*)decl;
        const auto& params = declFun->getParams();
        for (const auto& param : params) {
            if (param->getType() == AstNodeType::Identifier) {
                if (((Identifier*)param)->getName() == identifier.getName())
                    return param;
            } else if (param->getType() == AstNodeType::AssignmentPattern) {
                auto assignPattern = (AssignmentPattern*)param;
                if (assignPattern->getLeft()->getType() == AstNodeType::Identifier) {
                    if (auto paramId = (Identifier*)assignPattern->getLeft(); paramId->getName() == identifier.getName())
                        return param;
                } else {
                    trace("Cannot handle non-identifier left-side of assignment pattern of function parameters in resolveIdentifierDeclaration");
                    return nullptr;
                }
            } else if (param->getType() == AstNodeType::RestElement) {
                auto restElem = (RestElement*)param;
                if (restElem->getArgument()->getType() == AstNodeType::Identifier) {
                    if (auto paramId = (Identifier*)restElem->getArgument(); paramId->getName() == identifier.getName())
                        return param;
                } else {
                    trace("Cannot handle non-identifier left-side of rest element of function parameters in resolveIdentifierDeclaration");
                    return nullptr;
                }
            } else {
                // TODO: FIXME: Handle more function parameter types than just this!
                trace("Cannot handle non-identifier function parameters in resolveIdentifierDeclaration");
                return nullptr;
            }
        }
    }
    return decl;
}

AstNode *resolveMemberExpression(MemberExpression &expr)
{
    // TODO: Handle more than just ThisExpressions, try to lookup the object
    if (expr.getObject()->getType() != AstNodeType::ThisExpression)
        return nullptr;

    // We can't resolve dynamic expressions statically
    if (expr.getProperty()->getType() != AstNodeType::Identifier)
        return nullptr;

    auto propName = ((Identifier*)expr.getProperty())->getName();
    AstNode* targetScope = resolveThisExpression((ThisExpression&)*expr.getObject());
    if (!targetScope)
        return nullptr;

    // Currently we only support lookups in class scopes
    if (targetScope->getType() == AstNodeType::ClassDeclaration
           || targetScope->getType() == AstNodeType::ClassExpression) {
        vector<AstNode*> body = ((Class*)targetScope)->getBody()->getBody();
        for (auto node : body) {
            if (auto member = (ClassMethod*)node; node->getType() == AstNodeType::ClassMethod && member->getKey()->getName() == propName)
                return member;
            else if (auto member = (ClassPrivateMethod*)node; node->getType() == AstNodeType::ClassPrivateMethod && member->getKey()->getName() == propName)
                return member;
            else if (auto member = (ClassProperty*)node; node->getType() == AstNodeType::ClassProperty && member->getKey()->getName() == propName)
                return member;
            else if (auto member = (ClassPrivateProperty*)node; node->getType() == AstNodeType::ClassPrivateProperty && member->getKey()->getName() == propName)
                return member;
        }
        // TODO: If we don't find the property in the class, it might be in a parent class, check the extends!
        return nullptr;
    } else {
        return nullptr;
    }
}

AstNode *resolveThisExpression(ThisExpression &thisExpression)
{
    AstNode* parent = &thisExpression;
    while (parent && !isFunctionNode(*parent))
        parent = parent->getParent();
    if (!parent)
        return nullptr;
    return resolveThisValue(*parent);
}

AstNode *resolveThisValue(AstNode &lexicalScope)
{
    AstNode* targetScope = lexicalScope.getParent();
    while (targetScope && !isLexicalScopeNode(*targetScope))
        targetScope = targetScope->getParent();
    if (!targetScope)
        return nullptr;

    if (targetScope->getType() == AstNodeType::ClassDeclaration
               || targetScope->getType() == AstNodeType::ClassExpression) {
        return targetScope;
    } else if (targetScope->getType() == AstNodeType::ArrowFunctionExpression
               || targetScope->getType() == AstNodeType::ClassMethod
               || targetScope->getType() == AstNodeType::ClassPrivateMethod
               || targetScope->getParent()->getType() == AstNodeType::ClassProperty
               || targetScope->getParent()->getType() == AstNodeType::ClassPrivateProperty) {
            return resolveThisValue(*targetScope);
    } else {
        // Not much we can do in normal functions, since their this value is dynamic
        return nullptr;
    }
}
