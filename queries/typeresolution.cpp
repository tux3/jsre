#include "typeresolution.hpp"
#include "ast/ast.hpp"
#include "analyze/astqueries.hpp"
#include "graph/graph.hpp"
#include "analyze/identresolution.hpp"
#include "module/module.hpp"
#include "utils/reporting.hpp"

#include <utility>
#include <cassert>

using namespace std;

static TypeInfo resolveObjectTypeAnnotation(ObjectTypeAnnotation& node)
{
    bool strict = node.isExact();
    unordered_map<std::string, TypeInfo> props;
    for (auto propGenericNode : node.getProperties()) {
        if (propGenericNode->getType() == AstNodeType::ObjectTypeSpreadProperty) {
            // TODO: Support object type spread annotations (I think this just merges the props of the identifier's type)
            trace(node, "Unsupported spread in object type annotation");
            return {};
        }
        assert(propGenericNode->getType() == AstNodeType::ObjectTypeProperty);
        auto propNode = (ObjectTypeProperty*)propGenericNode;
        if (propNode->isOptional()) {
            // TODO: Better support for optional object type annotation fields, instead of ignoring them we should handle them (probably just w/ a sum type)
            // We probably should implement the idea that sum(undefined|whatever) means a field with that sum type may not be present/is not required.
            // It's okay to put a lot of special meaning in the Sum type, this is the one that keeps all the complexity.
            trace(node, "Ignoring optional object type annotation field, full optional support not implemented");
            strict = false;
            continue;
        }
        string key = propNode->getKey()->getName();
        props[key] = resolveAstAnnotationType(*propNode->getValue());
    }

    return TypeInfo::makeObject(move(props), strict);
}

static TypeInfo resolveFunctionTypeAnnotation(FunctionTypeAnnotation& node)
{
    // TODO: Handle rest param annotation
    if (node.getRestParam())
        return {};

    std::vector<TypeInfo> argumentTypes;
    for (auto paramNode : node.getParams())
        argumentTypes.push_back(resolveAstAnnotationType(*paramNode->getTypeAnnotation()));

    return TypeInfo::makeFunction(move(argumentTypes), resolveAstAnnotationType(*node.getReturnType()));
}

static TypeInfo resolveInterfaceDeclaration(InterfaceDeclaration& node)
{
    if (node.getTypeParameters()) {
        // TODO: Support interface type parameters
        trace(node, "Unsupported type parameters in interface type annotation");
        return {};
    }
    if (!node.getMixins().empty() || !node.getExtends().empty()) {
        trace(node, "Unsupported extends or mixings in interface type annotation");
        return {};
    }

    assert(node.getBody()->getType() == AstNodeType::ObjectTypeAnnotation);
    return resolveObjectTypeAnnotation((ObjectTypeAnnotation&)*node.getBody());
}

static TypeInfo resolveAstGenericTypeAnnotation(GenericTypeAnnotation& node)
{
    AstNode* decl = resolveIdentifierDeclaration((Identifier&)*node.getId());
    if (!decl)
        return {};

    if (decl->getType() == AstNodeType::ClassDeclaration || decl->getType() == AstNodeType::ClassExpression) {
        auto classType = TypeInfo::makeClass((Class&)*decl);
        auto extra = classType.getExtra<ClassTypeInfo>();
        return TypeInfo::makeObject((decltype(ClassTypeInfo::properties))extra->properties, extra->strict);
    } else if (decl->getType() == AstNodeType::InterfaceDeclaration) {
        auto interface = (InterfaceDeclaration*)decl;
        return resolveInterfaceDeclaration(*interface);
    } else if (decl->getType() == AstNodeType::TypeAlias) {
        auto alias = (TypeAlias*)decl;
        return resolveAstAnnotationType(*alias->getRight());
    } else {
        trace(node, "Failed to resolve AST generic annotation type: "s+decl->getTypeName());
        trace(*decl, "Declared here ");
        return {};
    }
}

TypeInfo resolveAstAnnotationType(AstNode& node)
{
    // TODO: Support resolving more flow annotation types, especially object types!

    auto astType = node.getType();
    if (astType == AstNodeType::AnyTypeAnnotation)
        return TypeInfo::makeUnknown();
    else if (astType == AstNodeType::NullLiteralTypeAnnotation)
        return TypeInfo::makeNull();
    else if (astType == AstNodeType::NumberLiteralTypeAnnotation)
        return TypeInfo::makeNumber();
    else if (astType == AstNodeType::NumberTypeAnnotation)
        return TypeInfo::makeNumber();
    else if (astType == AstNodeType::StringLiteralTypeAnnotation)
        return TypeInfo::makeString();
    else if (astType == AstNodeType::StringTypeAnnotation)
        return TypeInfo::makeString();
    else if (astType == AstNodeType::BooleanLiteralTypeAnnotation)
        return TypeInfo::makeBoolean();
    else if (astType == AstNodeType::BooleanTypeAnnotation)
        return TypeInfo::makeBoolean();
    else if (astType == AstNodeType::NullableTypeAnnotation)
        return TypeInfo::makeSum({TypeInfo::makeNull(), resolveAstAnnotationType(*((NullableTypeAnnotation&)node).getTypeAnnotation())});
    else if (astType == AstNodeType::GenericTypeAnnotation)
        return resolveAstGenericTypeAnnotation((GenericTypeAnnotation&)node);
    else if (astType == AstNodeType::ObjectTypeAnnotation)
        return resolveObjectTypeAnnotation((ObjectTypeAnnotation&)node);
    else if (astType == AstNodeType::FunctionTypeAnnotation)
        return resolveFunctionTypeAnnotation((FunctionTypeAnnotation&)node);
    else {
        trace(node, "Failed to resolve AST annotation type: "s+node.getTypeName());
        return {};
    }
}

TypeInfo resolveAstNodeType(AstNode& node)
{
    // TODO: Support resolving more AstNode types

    auto astType = node.getType();
    if (astType == AstNodeType::TypeAnnotation)
        return resolveAstAnnotationType(*((TypeAnnotation&)node).getTypeAnnotation());
    else if (astType == AstNodeType::NullLiteral)
        return TypeInfo::makeNull();
    else if (astType == AstNodeType::NumericLiteral)
        return TypeInfo::makeNumber();
    else if (astType == AstNodeType::StringLiteral)
        return TypeInfo::makeString(((StringLiteral&)node).getValue());
    else if (astType == AstNodeType::BooleanLiteral)
        return TypeInfo::makeBoolean();
    else if (isFunctionNode(node))
        return TypeInfo::makeFunction((Function&)node);
    else {
        trace(node, "Failed to resolve AST literal type: "s+node.getTypeName());
        return {};
    }
}

static TypeInfo resolveCallNode(Graph& graph, GraphNode* node)
{
    GraphNode* callee = &graph.getNode(node->getInput(0));
    TypeInfo calleeBaseType = resolveNodeType(graph, callee);

    if (calleeBaseType.getBaseType() != BaseType::Function)
        return {};
    auto calleeType = calleeBaseType.getExtra<FunctionTypeInfo>();
    return calleeType->returnType;
}

static TypeInfo resolveNewCallNode(Graph& graph, GraphNode* node)
{
    GraphNode* callee = &graph.getNode(node->getInput(0));
    TypeInfo calleeBaseType = resolveNodeType(graph, callee);

    if (calleeBaseType.getBaseType() == BaseType::Class) {
        auto calleeType = calleeBaseType.getExtra<ClassTypeInfo>();
        return TypeInfo::makeObject((decltype(ClassTypeInfo::properties))calleeType->properties, calleeType->strict);
    } else if (calleeBaseType.getBaseType() == BaseType::Function) {
        //auto calleeType = calleeBaseType.getExtra<FunctionTypeInfo>();
        // TODO: We need to find every this property store the function does to know what fields are defined (same as for class constructors)
        return TypeInfo::makeObject({}, false);
    } else {
        return TypeInfo::makeObject({}, false);
    }
}

static TypeInfo resolveCatchType(Graph& graph, GraphNode* catchNode)
{
    auto prevCount = catchNode->prevCount();

    vector<TypeInfo> types;
    for (uint16_t i=0; i<prevCount; ++i) {
        GraphNode& node = graph.getNode(catchNode->getPrev(i));
        TypeInfo newType = resolveNodeType(graph, &node);

        bool found = false;
        for (const auto& type : types) {
            if (type == newType) {
                found = true;
                break;
            }
        }
        if (!found)
            types.push_back(move(newType));
    }

    if (types.size() == 0)
        return {}; // We don't have any known thrown types
    if (types.size() == 1)
        return types[0];

    return TypeInfo::makeSum(move(types));
}

TypeInfo resolveReturnType(Function &fun)
{
    Graph* graph = fun.getParentModule().getFunctionGraph(fun);
    if (!graph) {
        if (fun.isAsync())
            return TypeInfo::makePromise({});
        else
            return {};
    }
    const GraphNode& end = graph->getNode(graph->size()-1);
    if (end.getType() != GraphNodeType::End)
        return {}; // A graph without an End at all is noreturn, that can happen
    const uint16_t exits = static_cast<uint16_t>(end.prevCount());

    vector<TypeInfo> types;
    for (uint16_t i=0; i<exits; ++i) {
        GraphNode& node = graph->getNode(end.getPrev(i));
        TypeInfo newType;
        if (node.getType() == GraphNodeType::Return) {
            newType = resolveNodeType(*graph, &node);
        } else if (node.getType() == GraphNodeType::Throw) {
            continue; // If we exit by throwing, that's not a return type
        } else {
            newType = TypeInfo::makeUndefined();
            if (fun.isAsync())
                newType = TypeInfo::makePromise(move(newType));
        }

        bool found = false;
        for (const auto& type : types) {
            if (type == newType) {
                found = true;
                break;
            }
        }
        if (!found)
            types.push_back(move(newType));
    }

    if (types.size() == 0)
        return {}; // The function might never return/unconditionally throw. We can't really say that it returns undefined.
    if (types.size() == 1)
        return types[0];

    return TypeInfo::makeSum(move(types));
}

TypeInfo resolveNodeType(Graph& graph, GraphNode* node)
{
    if (auto it = graph.nodeTypes.find(node); it != graph.nodeTypes.end())
        return it->second;

    TypeInfo type;

    if (node->getType() == GraphNodeType::Literal) {
        type = resolveAstNodeType(*node->getAstReference());
    } else if (node->getType() == GraphNodeType::LoadValue) {
        AstNode* decl = resolveIdentifierDeclaration((Identifier&)*node->getAstReference());
        if (decl) {
            if (isFunctionNode(*decl)) {
                type = TypeInfo::makeFunction((Function&)*decl);
            } else if (decl->getType() == AstNodeType::ClassDeclaration || decl->getType() == AstNodeType::ClassExpression) {
                type = TypeInfo::makeClass((Class&)*decl);
            } else if (decl->getType() == AstNodeType::Identifier && decl->getParent() == &graph.getFun()) { // We're loading an argument
                if (auto typeAnnotation = ((Identifier*)decl)->getTypeAnnotation())
                    type = resolveAstAnnotationType(*typeAnnotation->getTypeAnnotation());
            }
        }
    } else if (node->getType() == GraphNodeType::Call) {
        type = resolveCallNode(graph, node);
    } else if (node->getType() == GraphNodeType::NewCall) {
        type = resolveNewCallNode(graph, node);
    } else if (node->getType() == GraphNodeType::Function) {
        type = TypeInfo::makeFunction(*(Function*)node->getAstReference());
    } else if (node->getType() == GraphNodeType::ObjectLiteral) {
        std::unordered_map<std::string, TypeInfo> propTypes;
        bool strict = true;

        for (uint16_t i = 0; i<node->inputCount(); ++i) {
            bool propKeysKnown = true;

            auto& input = graph.getNode(node->getInput(i));
            if (input.getType() == GraphNodeType::ObjectProperty) {
                TypeInfo value = resolveNodeType(graph, &graph.getNode(input.getInput(0)));
                if (input.inputCount() == 1) { // Not computed
                    string keyStr;
                    AstNode* key = ((ObjectProperty&)*input.getAstReference()).getKey();
                    if (key->getType() == AstNodeType::Identifier)
                        keyStr = ((Identifier*)key)->getName();
                    else if (key->getType() == AstNodeType::StringLiteral)
                        keyStr = (((StringLiteral*)key)->getValue());
                    else if (key->getType() == AstNodeType::NumericLiteral)
                        keyStr = to_string((((NumericLiteral*)key)->getValue()));
                    else
                        throw runtime_error("Unexpected object property key of type "s+key->getTypeName());

                    propTypes[keyStr] = value;
                } else {
                    // TODO: Try to statically resolve the property name? (are there many usecases where we statically can?)
                    TypeInfo keyType = resolveNodeType(graph, &graph.getNode(input.getInput(1)));
                    if (keyType.getBaseType() == BaseType::String && keyType.hasExtra()) {
                        auto key = keyType.getExtra<const string>();
                        propTypes[*key] = value;
                    } else {
                        propKeysKnown = false;
                    }
                }
            } else if (input.getType() == GraphNodeType::Spread) {
                auto& spreadObj = graph.getNode(input.getInput(0));
                TypeInfo spreadObjInfo = resolveNodeType(graph, &spreadObj);
                if (spreadObjInfo.getBaseType() == BaseType::Object) {
                    auto extra = spreadObjInfo.getExtra<ObjectTypeInfo>();
                    strict &= extra->strict;
                    for (const auto& spreadProp : extra->properties)
                        propTypes[spreadProp.first] = spreadProp.second;
                } else {
                    propKeysKnown = false;
                }
            } else {
                trace(*node->getAstReference(), "Cannot resolve type of "s+input.getTypeName()+" in object literal");
                propKeysKnown = false;
            }

            if (!propKeysKnown) {
                // If there's a property name we can't resolve, all the previous properties now have unknown values because they could get overwritten...
                trace(*node->getAstReference(), "Failed to resolve property "+to_string(i)+" of object literal, forgetting all previous values' types");
                for (auto& prop : propTypes)
                    prop.second = TypeInfo::makeUnknown();
            }
            strict &= propKeysKnown;
        }

        type = TypeInfo::makeObject(move(propTypes), strict);
    } else if (node->getType() == GraphNodeType::LoadNamedProperty) {
        auto inputType = resolveNodeType(graph, &graph.getNode(node->getInput(0)));
        if (inputType.getBaseType() == BaseType::Object) {
            const auto& extra = inputType.getExtra<ObjectTypeInfo>();
            const auto& name = ((Identifier*)node->getAstReference())->getName();
            if (auto it = extra->properties.find(name); it != extra->properties.end())
                type = it->second;
            else if (extra->strict)
                type = TypeInfo::makeUndefined();
        }
    } else if (node->getType() == GraphNodeType::StoreNamedProperty) {
        auto inputType = resolveNodeType(graph, &graph.getNode(node->getInput(0)));
        if (inputType.getBaseType() == BaseType::Object) {
            const auto& extra = inputType.getExtra<ObjectTypeInfo>();
            auto props = extra->properties;
            const auto& name = ((Identifier*)node->getAstReference())->getName();
            props[name] = resolveNodeType(graph, &graph.getNode(node->getInput(1)));
            type = TypeInfo::makeObject(move(props), extra->strict);
        }
    } else if (node->getType() == GraphNodeType::Return) {
        if (node->inputCount())
            type = resolveNodeType(graph, &graph.getNode(node->getInput(0)));
        else
            type = TypeInfo::makeUndefined();
        if (graph.getFun().isAsync() && type.getBaseType() != BaseType::Promise)
            type = TypeInfo::makePromise(type);

    } else if (node->getType() == GraphNodeType::Await) {
        auto inputType = resolveNodeType(graph, &graph.getNode(node->getInput(0)));
        if (inputType.getBaseType() == BaseType::Promise)
            type = inputType.getExtra<PromiseTypeInfo>()->nestedType;
        else
            type = inputType;
    } else if (node->getType() == GraphNodeType::PrepareException) {
        type = resolveNodeType(graph, &graph.getNode(node->getInput(0)));
    } else if (node->getType() == GraphNodeType::CatchException) {
        type = resolveCatchType(graph, node);
    }

    return graph.nodeTypes.insert(pair{node, move(type)}).first->second;
}
