#include "typecheck.hpp"
#include "ast/ast.hpp"
#include "ast/walk.hpp"
#include "analyze/astqueries.hpp"
#include "analyze/identresolution.hpp"
#include "module/module.hpp"
#include "graph/graphbuilder.hpp"
#include "graph/dot.hpp"
#include "reporting.hpp"
#include "queries/typeresolution.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace std;

// NOTE: Be careful when checking a return type, if the function is async you should ensure found is wrapped in a promise before checking it
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

static void checkCallNode(Graph& graph, GraphNode* node)
{
    auto& callAstNode = (CallExpression&)*node->getAstReference();
    GraphNode* callee = &graph.getNode(node->getInput(0));
    TypeInfo calleeBaseType = resolveNodeType(graph, callee);

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
    if (nodeInputArgs > calleeArgCount)
        warn(callAstNode, "Function only takes "+to_string(calleeArgCount)+" arguments, but "+to_string(nodeInputArgs)+" were provided");
    for (uint16_t i=0; i<args; ++i) {
        GraphNode* argNode = &graph.getNode(node->getInput(i+1));
        TypeInfo argBaseType = resolveNodeType(graph, argNode);
        checkTypesCompatibility(*callAstNode.getArguments()[i], argBaseType, calleeType->argumentTypes[i]);
    }
}

static void checkPropertyLoad(Graph& graph, GraphNode* node)
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

void typecheckGraph(Graph& graph)
{
    trace("Graph data:\n"+graphToDOT(graph));

    unordered_set<uint16_t> visited;
    vector<GraphNode*> controlNodes = {&graph.getNode(0)};
    while (!controlNodes.empty()) {
        GraphNode* node = controlNodes.back();
        controlNodes.pop_back();

        // TODO: FIXME: Keep a "scope chain" of objects that are modified without reassignment
        // Normal reassignments are taken into account through readVariable/writeVariable and Phis,
        // but things like StoreNamedProperty, function calls modifying arguments, or aliased/escaping properties change a type without assignment!
        //
        // We need to keep a map of the known changes on top of the last writeVariable node (like StoreNamedProperties).
        // When reading any input node, we first check in our maps if we have an updated type, otherwise we can just resolve the input node's type
        // For branches, we create a new scope on each side and process each side separately.
        // At the end we merge all changes from all the child scopes into the parent scope. This can really just be marking all written types as Unknown in the parent scope...
        // (Of course we only need to do that if the child scope actually merges back into the parent scope instead of returning or throwing through)

        for (uint16_t i=0; i<node->nextCount(); ++i) {
            auto nextNodeId = node->getNext(i);
            if (visited.insert(nextNodeId).second)
                controlNodes.push_back(&graph.getNode(nextNodeId));
        }

        resolveNodeType(graph, node);

        auto type = node->getType();
        if (type == GraphNodeType::Call) {
            checkCallNode(graph, node);
        } else if (type == GraphNodeType::LoadNamedProperty
                   || type == GraphNodeType::LoadProperty) {
            checkPropertyLoad(graph, node);
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
        typecheckGraph(*graph);
    });
}
