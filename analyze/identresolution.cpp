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

LexicalBindings::LexicalBindings(LexicalBindings *parent, AstNode *code, bool isFullScope)
    : typeDeclarations{}
    , localDeclarations{}
    , varDeclarations{isFullScope ? localDeclarations : parent->varDeclarations}
    , children{}
    , parent{parent}
    , code{code}
{
}

const LexicalBindings &LexicalBindings::scopeForChildNode(AstNode *node) const
{
    for (const auto& child : children)
      if (child->code == node)
          return *child;
    return *this;
}

static void instantiateScopeDeclarations(unordered_map<Identifier*, Identifier*>& identifierTargets, LexicalBindings& bindings);

static bool isFullScopeNodeType(AstNodeType type)
{
    return type == AstNodeType::Root
            || type == AstNodeType::FunctionDeclaration
            || type == AstNodeType::FunctionExpression
            || type == AstNodeType::ArrowFunctionExpression
            || type == AstNodeType::ClassDeclaration
            || type == AstNodeType::ClassExpression
            || type == AstNodeType::ClassMethod
            || type == AstNodeType::ClassPrivateMethod
            || type == AstNodeType::ObjectMethod
            ;
}

static bool isPartialScopeNodeType(AstNodeType type)
{
    return type == AstNodeType::BlockStatement
            || type == AstNodeType::CatchClause
            || type == AstNodeType::ForStatement
            || type == AstNodeType::ForInStatement
            || type == AstNodeType::ForOfStatement
            || type == AstNodeType::SwitchStatement
            || type == AstNodeType::TypeAlias // Not a traditional scope!
            ;
}

static bool isExportedTypeIdentifier(Identifier& id)
{
    if (id.getParent()->getType() != AstNodeType::ExportSpecifier)
        return false;
    auto* spec = (ExportSpecifier*)id.getParent();
    assert(spec->getParent()->getType() == AstNodeType::ExportNamedDeclaration);
    auto* exportDecl = (ExportNamedDeclaration*)spec->getParent();
    return exportDecl->getKind() == ExportNamedDeclaration::Kind::Type;
}

static void walkImportDeclarations(vector<Identifier*>& lexicalDeclarations, unordered_map<string, Identifier*>& typeDeclarations, ImportDeclaration& node)
{
    bool isDeclTypeImport = node.getKind() == ImportDeclaration::Kind::Type;
    for (auto specifier : node.getSpecifiers()) {
        assert(specifier->getType() == AstNodeType::ImportSpecifier
               || specifier->getType() == AstNodeType::ImportDefaultSpecifier
               || specifier->getType() == AstNodeType::ImportNamespaceSpecifier);
        auto spec = (ImportBaseSpecifier*)specifier;
        if (spec->isTypeImport() || isDeclTypeImport)
            typeDeclarations[spec->getLocal()->getName()] = spec->getLocal();
        else
            lexicalDeclarations.push_back(spec->getLocal());
    }
}

static void walkTypeParameterDeclarations(unordered_map<string, Identifier*>& typeDeclarations, TypeParameterDeclaration* node)
{
    if (!node)
        return;
    const auto& params = node->getParams();
    for (auto param : params)
        typeDeclarations[param->getName()->getName()] = param->getName();
}

static void walkComplexDeclaration(vector<Identifier*>& declarationsFound, AstNode& node)
{
    auto type = node.getType();
    if (type == AstNodeType::Identifier) {
        auto& id = (Identifier&)node;
        declarationsFound.push_back(&id);
    } else if (type == AstNodeType::VariableDeclaration) {
        auto declarators = ((VariableDeclaration&)node).getDeclarators();
        for (auto declarator : declarators)
            walkComplexDeclaration(declarationsFound, *(declarator)->getId());
    } else if (type == AstNodeType::ObjectPattern) {
        auto& patternNode = (ObjectPattern&)node;
        for (auto prop : patternNode.getProperties()) {
            if (prop->getType() == AstNodeType::ObjectProperty)
                walkComplexDeclaration(declarationsFound, *((ObjectProperty*)prop)->getValue());
            else if (prop->getType() == AstNodeType::RestElement)
                walkComplexDeclaration(declarationsFound, *((RestElement*)prop)->getArgument());
            else
                throw std::runtime_error("Unhandled type "s+prop->getTypeName()+" while resolving object pattern declarations");
        }
    } else if (type == AstNodeType::ArrayPattern) {
        auto& patternNode = (ArrayPattern&)node;
        for (auto elem : patternNode.getElements())
            walkComplexDeclaration(declarationsFound, *elem);
    } else if (type == AstNodeType::AssignmentPattern) {
        auto& patternNode = (AssignmentPattern&)node;
        walkComplexDeclaration(declarationsFound, *patternNode.getLeft());
    } else if (type == AstNodeType::RestElement) {
        auto& patternNode = (RestElement&)node;
        walkComplexDeclaration(declarationsFound, *patternNode.getArgument());
    } else {
        // This can happen when some thoroughly non-strict code uses assignements deep inside expressions to introduce a new variable.
        // We don't go inside complex expressions here (if we did, we'd have to filter what identifiers are actually declarations as opposed to exprs, depending on the parent)
        // We can run in arbitrarily crazy expressions in the init of a "for", for example. I've seen `for (cond && (x = value); test; ) {}` in minified code.
        trace(node, "Unexpected id type for identifier or object pattern: "s+node.getTypeName());
    }
}

static void walkComplexDeclaration(vector<Identifier*>& varDeclarations, vector<Identifier*>& lexicalDeclarations, AstNode* node)
{
    if (!node)
        return;
    auto* targetDeclarations = &varDeclarations;
    if (node->getType() == AstNodeType::VariableDeclaration) {
        auto decl = (VariableDeclaration*)node;
        if (decl->getKind() != VariableDeclaration::Kind::Var)
            targetDeclarations = &lexicalDeclarations;
    }
    walkComplexDeclaration(*targetDeclarations, *node);
}

static void walkChildrenForDeclarations(unordered_map<Identifier*, Identifier*>& identifierTargets,
                                        unordered_map<string, Identifier*>& typeDeclarations,
                                        vector<Identifier*>& varDeclarations,
                                        vector<Identifier*>& lexicalDeclarations,
                                        unordered_map<string, Identifier*>& functionDeclarations,
                                        LexicalBindings& bindings, AstNode& parent)
{
    parent.applyChildren([&](AstNode* node) {
        auto type = node->getType();
        if (type == AstNodeType::FunctionDeclaration) {
            auto fun = (Function*)node;
            if (auto id = fun->getId())
                functionDeclarations[id->getName()] = id;
        } else if (type == AstNodeType::ObjectMethod) {
            auto decl = (ObjectMethod*)node;
            if (auto keyNode = decl->getKey(); !decl->isComputed()) {
                auto key = (Identifier*)keyNode;
                functionDeclarations[key->getName()] = key;
            }
        } else if (type == AstNodeType::ClassMethod || type == AstNodeType::ClassPrivateMethod) {
            auto method = (ClassBaseMethod*)node;
            if (auto keyNode = method->getKey(); !method->isComputed()) {
                auto key = (Identifier*)keyNode;
                functionDeclarations[key->getName()] = key;
            }
        } else if (type == AstNodeType::ClassProperty || type == AstNodeType::ClassPrivateProperty) {
            auto prop = (ClassBaseProperty*)node;
            if (auto keyNode = prop->getKey(); !prop->isComputed()) {
                auto key = (Identifier*)keyNode;
                functionDeclarations[key->getName()] = key;
            }
        } else if (type == AstNodeType::ClassDeclaration) {
            auto decl = (ClassDeclaration*)node;
            lexicalDeclarations.push_back(decl->getId());
        } else if (type == AstNodeType::VariableDeclaration) {
            walkComplexDeclaration(varDeclarations, lexicalDeclarations, node);
        } else if (type == AstNodeType::TypeAlias) {
            auto decl = (TypeAlias*)node;
            typeDeclarations[decl->getId()->getName()] = decl->getId();
        } else if (type == AstNodeType::InterfaceDeclaration) {
            auto decl = (InterfaceDeclaration*)node;
            typeDeclarations[decl->getId()->getName()] = decl->getId();
        } else if (type == AstNodeType::ImportDeclaration) {
            walkImportDeclarations(lexicalDeclarations, typeDeclarations, *(ImportDeclaration*)node);
        }

        if (isPartialScopeNodeType(type)) {
            auto& newScope = bindings.children.emplace_back(make_unique<LexicalBindings>(&bindings, node, false));
            instantiateScopeDeclarations(identifierTargets, *newScope);
        } else if (isFullScopeNodeType(type)) {
            auto& newScope = bindings.children.emplace_back(make_unique<LexicalBindings>(&bindings, node, true));
            instantiateScopeDeclarations(identifierTargets, *newScope);
        } else {
            walkChildrenForDeclarations(identifierTargets, typeDeclarations, varDeclarations, lexicalDeclarations, functionDeclarations, bindings, *node);
        }

        return true;
    });
}

// Some scope nodes introduce a name inwards only, not visible in the parent scope.
static void instantiateScopeNodeInnerDeclaration(vector<Identifier*>& varDeclarations, vector<Identifier*>& lexicalDeclarations, LexicalBindings& bindings)
{
    AstNode* node = bindings.code;
    auto type = node->getType();
    if (type == AstNodeType::FunctionExpression) {
        if (auto id = ((FunctionExpression*)node)->getId())
            varDeclarations.push_back(id);
    } else if (type == AstNodeType::ClassExpression) {
        if (auto id = ((ClassExpression*)node)->getId())
            varDeclarations.push_back(id);
    } else if (type == AstNodeType::CatchClause) {
        auto clause = (CatchClause*)node;
        walkComplexDeclaration(lexicalDeclarations, *clause->getParam());
    } else if (type == AstNodeType::ForStatement) {
        walkComplexDeclaration(varDeclarations, lexicalDeclarations, ((ForStatement*)node)->getInit());
    } else if (type == AstNodeType::ForInStatement) {
        walkComplexDeclaration(varDeclarations, lexicalDeclarations, ((ForInStatement*)node)->getLeft());
    } else if (type == AstNodeType::ForOfStatement) {
        walkComplexDeclaration(varDeclarations, lexicalDeclarations, ((ForOfStatement*)node)->getLeft());
    }

    if (node->getType() == AstNodeType::ClassDeclaration)
        walkTypeParameterDeclarations(bindings.typeDeclarations, ((ClassDeclaration*)node)->getTypeParameters());
    else if (node->getType() == AstNodeType::ClassExpression)
        walkTypeParameterDeclarations(bindings.typeDeclarations, ((ClassExpression*)node)->getTypeParameters());
    else if (isFunctionNode(*node))
        walkTypeParameterDeclarations(bindings.typeDeclarations, ((Function*)node)->getTypeParameters());
}

static void instantiateScopeDeclarations(unordered_map<Identifier*, Identifier*>& identifierTargets, LexicalBindings& bindings)
{
    vector<Identifier*> varDeclarations, lexicalDeclarations;
    unordered_map<string, Identifier*> functionDeclarations;

    instantiateScopeNodeInnerDeclaration(varDeclarations, lexicalDeclarations, bindings);

    if (isFunctionNode(*bindings.code)) {
        auto& fun = (Function&)*bindings.code;
        bool hasParameterExpressions = false;
        for (auto const& param : fun.getParams()) {
            walkComplexDeclaration(varDeclarations, *param);
            if (param->getType() != AstNodeType::Identifier && param->getType() != AstNodeType::RestElement)
                hasParameterExpressions = true;
        }

        if (auto returnType = fun.getReturnType())
            walkChildrenForDeclarations(identifierTargets, bindings.typeDeclarations, varDeclarations,
                                        lexicalDeclarations, functionDeclarations, bindings, *returnType);
        if (auto typeParamDecl = fun.getTypeParameters())
            walkChildrenForDeclarations(identifierTargets, bindings.typeDeclarations, varDeclarations,
                                        lexicalDeclarations, functionDeclarations, bindings, *typeParamDecl);

        if (hasParameterExpressions || isFullScopeNodeType((fun.getBody()->getType()))) {
            // The standard says we have to create a separate scope for params and body if we have parameter expressions
            auto& bodyScope = bindings.children.emplace_back(make_unique<LexicalBindings>(&bindings, fun.getBody(), true));
            instantiateScopeDeclarations(identifierTargets, *bodyScope);
        } else if (isPartialScopeNodeType(fun.getBody()->getType())) {
            // Note that we're not creating a full separate scope (standard doesn't ask), but if the body is e.g. a Block it still needs a partial one
            auto& bodyScope = bindings.children.emplace_back(make_unique<LexicalBindings>(&bindings, fun.getBody(), false));
            instantiateScopeDeclarations(identifierTargets, *bodyScope);
        } else {
            walkChildrenForDeclarations(identifierTargets, bindings.typeDeclarations, varDeclarations,
                                        lexicalDeclarations, functionDeclarations, bindings, *fun.getBody());
        }
    } else {
        walkChildrenForDeclarations(identifierTargets, bindings.typeDeclarations, varDeclarations,
                                    lexicalDeclarations, functionDeclarations, bindings, *bindings.code);
    }

    for (Identifier* id : varDeclarations) {
        bindings.varDeclarations[id->getName()] = id;
        identifierTargets[id] = id;
    }
    for (Identifier* id : lexicalDeclarations) {
        bindings.localDeclarations[id->getName()] = id;
        identifierTargets[id] = id;
    }
    for (auto& [name, id] : functionDeclarations) {
        bindings.varDeclarations[name] = id;
        identifierTargets[id] = id;
    }
    for (auto& [name, id] : bindings.typeDeclarations) {
        identifierTargets[id] = id;
    }
}

static Identifier* findDeclarationBinding(const LexicalBindings& bindings, const std::string& name, bool isType = false)
{
    for (const LexicalBindings* scope = &bindings; scope; scope = scope->parent) {
        if (isType) {
            // Things like class types look like regular identifiers (for example when imported), so they may not be found in typeDeclarations
            if (auto it = scope->typeDeclarations.find(name); it != scope->typeDeclarations.end())
                return it->second;
        }
        if (auto it = scope->localDeclarations.find(name); it != scope->localDeclarations.end())
            return it->second;
    }
    return nullptr;
}

static void walkScopeIdentifiers(unordered_map<Identifier*, Identifier*>& identifierTargets,
                                    unordered_map<string, Identifier*>& unresolvedTopLevelIdentifiers,
                                    const LexicalBindings& bindings, AstNode* node)
{
    auto type = node->getType();
    if (isPartialScopeNodeType(type) || isFullScopeNodeType(type))
        return;

    if (type == AstNodeType::Identifier) {
        auto identifier = (Identifier*)node;
        if (auto typeAnnotation = identifier->getTypeAnnotation())
             walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, typeAnnotation);
        bool isType = isExportedTypeIdentifier(*identifier)
                || identifier->getParent()->getType() == AstNodeType::GenericTypeAnnotation
                || identifier->getParent()->getType() == AstNodeType::ClassImplements;
        if (auto it = identifierTargets.find(identifier); it != identifierTargets.end())
            return;
        const auto& name = identifier->getName();
        if (auto decl = findDeclarationBinding(bindings, name, isType))
            identifierTargets[identifier] = decl;
        else if (!bindings.parent)
            unresolvedTopLevelIdentifiers[name] = identifier;
    } else if (type == AstNodeType::ObjectProperty) {
        auto& obj = (ObjectProperty&)*node;
        if (auto* val = obj.getValue())
            walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, val);
        if (obj.isComputed())
            walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, obj.getKey());
    } else if (type == AstNodeType::MemberExpression) {
        auto& expr = (MemberExpression&)*node;
        if (auto* obj = expr.getObject())
            walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, obj);
        if (expr.isComputed())
            walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, expr.getProperty());
    } else if (type == AstNodeType::QualifiedTypeIdentifier) {
        auto& type = (QualifiedTypeIdentifier&)*node;
        walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, type.getQualification());
    } else {
        node->applyChildren([&](AstNode* child) {
             walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, child);
             return true;
        });
    }
}

static void resolveScopeIdentifiers(unordered_map<Identifier*, Identifier*>& identifierTargets,
                                    unordered_map<string, Identifier*>& unresolvedTopLevelIdentifiers,
                                    const LexicalBindings& bindings)
{
    for (auto const& childScope : bindings.children)
        resolveScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, *childScope);

    if (isFunctionNode(*bindings.code)) {
        auto fun = (Function*)bindings.code;
        // If we had to create a separate full scope for params and body (because standard says so), only process the params and types in this scope
        if (bindings.children.size() == 1 && &bindings.children[0]->varDeclarations != &bindings.localDeclarations
                && bindings.children[0]->code == fun->getBody()) {
            for (auto param : fun->getParams())
                walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, param);
            if (auto returnType = fun->getReturnType())
                walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, returnType);
            if (auto typeParamDecl = fun->getTypeParameters())
                walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, typeParamDecl);
            return;
        }
    }

    bindings.code->applyChildren([&](AstNode* node) {
         walkScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, bindings, node);
         return true;
    });
}

IdentifierResolutionResult resolveModuleIdentifiers(v8::Local<v8::Context> context, AstRoot& ast)
{
    auto rootBindings = make_unique<LexicalBindings>(nullptr, &ast, true);
    unordered_map<Identifier*, Identifier*> identifierTargets;
    unordered_map<string, Identifier*> unresolvedTopLevelIdentifiers;

    instantiateScopeDeclarations(identifierTargets, *rootBindings);
    resolveScopeIdentifiers(identifierTargets, unresolvedTopLevelIdentifiers, *rootBindings);

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

    return {move(identifierTargets), move(missingGlobalIdentifiers), move(rootBindings)};
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
    // TODO: Make some attempt at resolving exported identifiers of non-ES6 modules (maybe fill the root scope dynamically at import time)

    Module& sourceMod = importSpec.getParentModule();
    string source, importSpecName;
    bool isType = false;

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
        if (importSpec.getType() == AstNodeType::ImportSpecifier) {
            importSpecName = ((ImportSpecifier&)importSpec).getImported()->getName();
            if (((ImportSpecifier&)importSpec).isTypeImport())
                isType = true;
            else if (((ImportDeclaration*)importSpec.getParent())->getKind() == ImportDeclaration::Kind::Type)
                isType = true;
        }
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

        if (exported && exported->getType() == AstNodeType::Identifier) {
            const auto& resolvedLocals = importedMod.getResolvedLocalIdentifiers();
            const auto& resolvedIt = resolvedLocals.find((Identifier*)exported);
            if (resolvedIt != resolvedLocals.end())
                exported = resolvedIt->second;
        }
    } else if (importSpec.getType() == AstNodeType::ImportSpecifier || importSpec.getType() == AstNodeType::ExportSpecifier) {
        auto& scopeChain = importedMod.getScopeChain();
        if (isType) {
            if (auto it = scopeChain.typeDeclarations.find(importSpecName); it != scopeChain.typeDeclarations.end())
                return it->second;
        }
        if (auto it = scopeChain.localDeclarations.find(importSpecName); it != scopeChain.localDeclarations.end())
            return it->second;
    } else {
        assert(false && "Unexpected import specifier type");
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

    AstNode* decl = it->second;
    AstNode* parent = decl->getParent();
    while (parent->getType() == AstNodeType::ImportDefaultSpecifier
           || parent->getType() == AstNodeType::ImportSpecifier
           || (parent->getType() == AstNodeType::ExportSpecifier
               && ((ExportNamedDeclaration*)parent->getParent())->getSource())) {
        decl = resolveImportedIdentifierDeclaration(*parent);
        if (!decl)
            return nullptr;
        parent = decl->getParent();
    }

    if (isFunctionNode(*parent)) {
        auto declFun = (Function*)parent;
        if (declFun->getId() == decl)
            return declFun;
    } else if (parent->getType() == AstNodeType::ClassDeclaration) {
        if (((ClassDeclaration*)parent)->getId() == decl)
            return parent;
    } else if (parent->getType() == AstNodeType::VariableDeclarator) {
        if (((VariableDeclarator*)parent)->getId() == decl)
            return parent;
    } else if (parent->getType() == AstNodeType::InterfaceDeclaration) {
        if (((InterfaceDeclaration*)parent)->getId() == decl)
            return parent;
    } else if (parent->getType() == AstNodeType::TypeAlias) {
        if (((TypeAlias*)parent)->getId() == decl)
            return parent;
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
            if (node->getType() == AstNodeType::ClassMethod || node->getType() == AstNodeType::ClassPrivateMethod) {
                auto member = (ClassBaseMethod*)node;
                if (!member->isComputed() && ((Identifier*)member->getKey())->getName() == propName)
                    return member;
            } else if (node->getType() == AstNodeType::ClassProperty || node->getType() == AstNodeType::ClassPrivateProperty) {
                auto member = (ClassBaseProperty*)node;
                if (!member->isComputed() && ((Identifier*)member->getKey())->getName() == propName)
                    return member;
            }
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
