#include "ast/walk.hpp"
#include <iostream>

using namespace std;
using json = nlohmann::json;

void walkAst(json& astNode, AstNodeCallback cb)
{
    auto typeIt = astNode.find("type");
    if (typeIt == astNode.end())
        throw runtime_error("getAstChildren: An AST node must have a type");
    string type = *typeIt;
}
