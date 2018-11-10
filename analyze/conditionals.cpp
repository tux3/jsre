#include "conditionals.hpp"
#include "module/module.hpp"
#include "ast/ast.hpp"
#include "ast/walk.hpp"
#include "utils/reporting.hpp"
#include <unordered_map>
#include <string>

using namespace std;

void analyzeConditionals(Module &module)
{
    findEmptyBodyConditionals(module);
    findDuplicateIfTests(module);
}

void findEmptyBodyConditionals(Module &module)
{
    AstRoot& ast = module.getAst();
    walkAst(ast, [&](AstNode& node){
        if (node.getType() == AstNodeType::IfStatement) {
            auto& conditional = (IfStatement&)node;
            if (conditional.getConsequent()->getType() != AstNodeType::EmptyStatement)
                return;
        } else if (node.getType() == AstNodeType::WhileStatement) {
            auto& conditional = (WhileStatement&)node;
            if (conditional.getBody()->getType() != AstNodeType::EmptyStatement)
                return;
        } else if (node.getType() == AstNodeType::DoWhileStatement) {
            auto& conditional = (DoWhileStatement&)node;
            if (conditional.getBody()->getType() != AstNodeType::EmptyStatement)
                return;
        } else if (node.getType() == AstNodeType::ForStatement) {
            auto& conditional = (ForStatement&)node;
            if (conditional.getBody()->getType() != AstNodeType::EmptyStatement)
                return;
        } else if (node.getType() == AstNodeType::ForInStatement) {
            auto& conditional = (ForInStatement&)node;
            if (conditional.getBody()->getType() != AstNodeType::EmptyStatement)
                return;
        } else if (node.getType() == AstNodeType::ForOfStatement) {
            auto& conditional = (ForOfStatement&)node;
            if (conditional.getBody()->getType() != AstNodeType::EmptyStatement)
                return;
        } else {
            return;
        }

        warn(node, "Suspicious semicolon after conditional"s);
    });
}

void findDuplicateIfTests(Module &module)
{
    AstRoot& ast = module.getAst();
    const std::string &source = module.getOriginalSource();
    walkAst(ast, [&](AstNode& node){
        if (node.getType() != AstNodeType::IfStatement)
            return;
        // We do all the children once from the parent, so don't process them again
        if (node.getParent()->getType() == AstNodeType::IfStatement && ((IfStatement*)node.getParent())->getAlternate() == &node)
            return;

        // We're trying to catch copy-paste errors, so it's probably fine (and faster!) to compare the source text directly!
        unordered_map<string, AstNode*> tests;
        AstNode* cur = &node;
        while (cur && cur->getType() == AstNodeType::IfStatement) {
            auto* conditional = (IfStatement*)cur;
            string testSource =  conditional->getTest()->getLocation().toString(source);
            auto result = tests.insert({testSource, conditional});
            if (!result.second)
                error(*conditional, "Duplicate if condition, previously appears on line "+to_string(tests[testSource]->getLocation().start.line));
            cur = conditional->getAlternate();
        }
    });
}
