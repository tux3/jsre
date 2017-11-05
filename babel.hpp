#ifndef BABEL_H
#define BABEL_H

#include "isolatewrapper.hpp"
#include <experimental/filesystem>
#include <json.hpp>
#include <utility>

// This runs babel, returning the transpiled script and the JSON AST
std::pair<std::string, nlohmann::json> transpileScript(IsolateWrapper& isolateWrapper, const std::string& script);
nlohmann::json getBabelConfigForFile(const std::experimental::filesystem::path& sourcePath);

#endif // BABEL_H
