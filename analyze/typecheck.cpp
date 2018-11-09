#include "typecheck.hpp"
#include "ast/ast.hpp"
#include "ast/walk.hpp"
#include "analyze/astqueries.hpp"
#include "analyze/identresolution.hpp"
#include "analyze/typerefinement.hpp"
#include "module/module.hpp"
#include "graph/graphbuilder.hpp"
#include "graph/dot.hpp"
#include "utils/reporting.hpp"
#include "queries/typeresolution.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>

using namespace std;

static TypeInfo resolveScopedNodeType(Graph& graph, const GraphNode* node, ScopedTypes const& scope)
{
    if (auto it = scope.types.find(node); it != scope.types.end())
        return it->second;
    return resolveNodeType(graph, node);
}

// NOTE: Be careful when checking a return type, if the function is async callers should ensure found is wrapped in a promise before passing it in
static void checkTypesCompatibility(AstNode& foundNode, const TypeInfo& found, const TypeInfo& expected)
{
    // Nothing we can do if we just don't have the information
    if (expected.getBaseType() == BaseType::Unknown || found.getBaseType() == BaseType::Unknown)
        return;
    if (found == expected)
        return;

    // Expecting a sum type is special, we have to check that the found type (or types) are all included in the expected sum
    if (expected.getBaseType() == BaseType::Sum) {
        // TODO: Check whether we satisfy the sum
        if (found.getBaseType() == BaseType::Sum) {

        } else {

        }
        return;
    }

    if (found.getBaseType() != expected.getBaseType()) {
        // TODO: Add special error messages for promise vs non-promise, and found sum types vs expected non-sum

        error(foundNode, "Expected type \""s+expected.baseTypeName()+"\", but got \""+found.baseTypeName()+"\"");
        return;
    }

    // TODO: Diagnose all incompatibilities. We know base types are equal, but complete types are different
    switch (found.getBaseType()) {
    case BaseType::Promise:
        error(foundNode, "Expected a Promise<"s+expected.baseTypeName()+">, but got an incompatible Promise<"+found.baseTypeName()+">.");
        break;
    default:
        break;
    }
}

static void checkCallNode(Graph& graph, GraphNode* node, ScopedTypes const& scope)
{
    auto& callAstNode = (CallExpression&)*node->getAstReference();
    GraphNode* callee = &graph.getNode(node->getInput(0));
    TypeInfo calleeBaseType = resolveScopedNodeType(graph, callee, scope);

    if (calleeBaseType.getBaseType() != BaseType::Function) {
        if (calleeBaseType.getBaseType() != BaseType::Unknown) {
            error(callAstNode, "Trying to call \""+callAstNode.getCallee()->getSourceString()+"\", but it has type "s+calleeBaseType.baseTypeName());
        }
        return;
    }
    auto calleeType = calleeBaseType.getExtra<FunctionTypeInfo>();

    size_t calleeArgCount = calleeType->argumentTypes.size();
    size_t nodeInputArgs = node->inputCount() - 1;
    size_t args = std::min(calleeArgCount, nodeInputArgs);
    if (nodeInputArgs > calleeArgCount && !calleeType->variadic)
        warn(callAstNode, "Function only takes "+to_string(calleeArgCount)+" arguments, but "+to_string(nodeInputArgs)+" were provided");
    for (uint16_t i=0; i<args; ++i) {
        GraphNode* argNode = &graph.getNode(node->getInput(i+1));
        TypeInfo argBaseType = resolveScopedNodeType(graph, argNode, scope);
        checkTypesCompatibility(*callAstNode.getArguments()[i], argBaseType, calleeType->argumentTypes[i]);
    }
}

static void checkPropertyLoad(Graph& graph, GraphNode* node, ScopedTypes const& scope)
{
    bool knownPropName = false;
    std::string propName;

    if (node->getType() == GraphNodeType::LoadNamedProperty) {
        propName = ((Identifier*)node->getAstReference())->getName();
        knownPropName = true;
    }
    // TODO: Try to resolve simple dynamic prop names (e.g. even obj["prop"] or obj[2] count as dynamic...)

    TypeInfo objectType = resolveNodeType(graph, &graph.getNode(node->getInput(0)));
    if (objectType.getBaseType() == BaseType::Undefined
            || objectType.getBaseType() == BaseType::Null
            || objectType.getBaseType() == BaseType::Number
            || objectType.getBaseType() == BaseType::Boolean)
    {
        error(*node->getAstReference(), "Trying to access a property on a \""s+objectType.baseTypeName()+"\" value");
    } else if (objectType.getBaseType() == BaseType::String) {
        // TODO: Check strings for undefined property accesses
    } else if (objectType.getBaseType() == BaseType::Promise) {
        // FIXME: Note that we're also inheriting all the properties of Object like hasOwnProperty, isPrototypeOf, toString, etc. We should check for those too!
        if (knownPropName) {
            if (knownPropName && (propName != "then" && propName != "catch" && propName != "finally")) {
                warn(*node->getAstReference()->getParent(), "Trying to access property \""+propName+"\" on a promise, are you missing an await?");
            }
        } else if (node->getType() == GraphNodeType::LoadProperty) {
            // Totally a guess, we could conceivably just be calling "then" or "catch" in a very contrived way
            suggest(*node->getAstReference()->getParent(), "Suspicious dynamic property access on a promise object, are you missing an await?");
        }
    } else if (objectType.getBaseType() == BaseType::Object && knownPropName) {
        const auto& extra = objectType.getExtra<ObjectTypeInfo>();
        if (extra->strict && extra->properties.find(propName) == extra->properties.end())
            error(*node->getAstReference()->getParent(), "Trying to access property \""+propName+"\", but it is always undefined in this object");
    }
}

static TypeInfo mergeTypes(const vector<const TypeInfo*>& typesToMerge)
{
    vector<TypeInfo> types;

    // TODO: Don't do a quadratic search! Fix TypeInfo to be hashable and put that in a set or something...
    auto addIfNotExists = [&types](const TypeInfo* newType) {
        bool found = false;
        for (const auto& existingType : types) {
            if (*newType == existingType) {
                found = true;
                break;
            }
        }
        if (!found)
            types.push_back(*newType);
    };

    for (const TypeInfo* type : typesToMerge) {
        if (type->getBaseType() == BaseType::Unknown) {
            return TypeInfo::makeUnknown();
        } else if (type->getBaseType() == BaseType::Sum) {
            const auto& elems = type->getExtra<SumTypeInfo>()->elements;
            for (const auto& elem : elems)
                addIfNotExists(&elem);
        } else {
            addIfNotExists(type);
        }
    }

    if (types.size() == 0) {
        trace("Merging types resulted in an impossible empty type!");
        throw std::runtime_error("Merging types resulted in an impossible empty type!");
    }
    if (types.size() == 1)
        return types[0];

    return TypeInfo::makeSum(move(types));
}

static ScopedTypes mergeScopes(ScopedTypes const& oldScope)
{
    unordered_set<ScopedTypes*> const& prevs = oldScope.prevs;
    ScopedTypes mergedScope = oldScope;

    unordered_map<const GraphNode*, vector<const TypeInfo*>> typesToMerge;
    for (const auto& prev : prevs)
        for (const auto& identType : prev->types)
            typesToMerge[identType.first].push_back(&identType.second);

    for (const auto& typeToMerge : typesToMerge)
        mergedScope.types[typeToMerge.first] = mergeTypes(typeToMerge.second);

    return mergedScope;
}

static void typecheckNode(Graph& graph, GraphNode* node, ScopedTypes& scope)
{
    auto type = node->getType();
    if (type == GraphNodeType::Call) {
        checkCallNode(graph, node, scope);
    } else if (type == GraphNodeType::LoadNamedProperty
               || type == GraphNodeType::LoadProperty) {
        checkPropertyLoad(graph, node, scope);
    }
}

static void runTypechecksInBranch(Graph& graph, unordered_map<GraphNode*, ScopedTypes>& scopes, queue<GraphNode*>& scopesToVisit, GraphNode* node)
{
    ScopedTypes& scope = scopes[node];
    if (scope.visited) {
        ScopedTypes mergedScope = mergeScopes(scope);
        if (mergedScope.types == scope.types)
            return;
    }
    scope.visited++;
    refineTypes(graph, scope, node);

    for (;;) {
        typecheckNode(graph, node, scope);

        if (node->nextCount() == 0) {
            return;
        } else if (node->nextCount() == 1) {
            auto nextNodeId = node->getNext(0);
            node = &graph.getNode(nextNodeId);
            if (node->prevCount() != 1) {
                ScopedTypes& mergeScope = scopes[node];
                mergeScope.prevs.insert(&scope);
                scopesToVisit.push(node);
                return;
            }
        } else {
            unordered_set<GraphNode*> nextScopesToVisit;
            for (uint16_t i=0; i<node->nextCount(); ++i)
                nextScopesToVisit.insert(&graph.getNode(node->getNext(i)));
            for (auto scopeNode : nextScopesToVisit) {
                scopes[scopeNode].prevs.insert(&scope);
                scopesToVisit.push(scopeNode);
            }
            return;
        }
    }
}

void runTypechecks(Module &module)
{
    AstRoot& ast = module.getAst();

    walkAst(ast, [&](AstNode& node){
        if (!isFunctionNode(node))
            return;

        auto& fun = (Function&)node;
        auto graph = module.getFunctionGraph(fun);
        if (!graph)
            return;
        trace("Graph data:\n"+graphToDOT(*graph));

        unordered_map<GraphNode*, ScopedTypes> scopes;
        queue<GraphNode*> scopesToVisit;

        /*
         * TODO:
         * - Add graph nodes for arguments
         * - Make sure type refinement works for arguments
         */

        scopesToVisit.push(&graph->getNode(0));
        while (!scopesToVisit.empty()) {
            auto next = scopesToVisit.front();
            scopesToVisit.pop();
            runTypechecksInBranch(*graph, scopes, scopesToVisit, next);
        }
    });
}
