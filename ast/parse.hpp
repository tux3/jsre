#ifndef PARSE_H
#define PARSE_H

#include <string>
#include <future>

class AstRoot;
class Module;

void startParsingThreads();
void stopParsingThreads();

// This runs babel and imports its result in a worker thread, returning the AST
std::future<AstRoot*> parseSourceScriptAsync(Module& parentModule, const std::string& script);

#endif // PARSE_H
