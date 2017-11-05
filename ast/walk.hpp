#ifndef WALK_HPP
#define WALK_HPP

#include <json.hpp>
#include <vector>

using AstNodeCallback = bool (*)(nlohmann::json&);
void walkAst(nlohmann::json& astNode, AstNodeCallback);

#endif // WALK_HPP
