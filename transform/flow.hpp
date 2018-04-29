#ifndef FLOW_HPP
#define FLOW_HPP

#include <string>

class AstNode;

std::string stripFlowTypes(const std::string& source, AstNode& ast);

#endif // FLOW_HPP
