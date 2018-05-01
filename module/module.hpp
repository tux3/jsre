#ifndef MODULE_HPP
#define MODULE_HPP

#include "basicmodule.hpp"
#include <filesystem>
#include <string>
#include <unordered_map>
#include <future>
#include <v8.h>

class IsolateWrapper;
class AstRoot;
class Identifier;
class ImportSpecifier;

class Module final : public BasicModule {
public:
    Module(IsolateWrapper& isolateWrapper, std::filesystem::path path);
    // This instantiates modules recursively imported or statically require()'d by this one if they are part of the project
    void resolveProjectImports(const std::filesystem::path &projectDir);
    void analyze(); //< Performs analysis and reports result to the user
    AstRoot& getAst();
    v8::Local<v8::Module> getExecutableModule();
    v8::Local<v8::Module> getExecutableES6Module();
    int getCompiledModuleIdentityHash();
    virtual std::string getPath() const override;

    std::unordered_map<Identifier*, std::vector<Identifier*>> getLocalXRefs();
    std::unordered_map<Identifier*, Identifier*> getResolvedLocalIdentifiers();

    enum class EmbedderDataIndex : int {
        Reserved = 0, // Has a special meaning for the Chrome Debugger, or so I'm told
        ModulePath = 1,
    };

private:
    v8::Local<v8::Module> compileModuleFromSource(const std::string& filename, const std::string& source);
    v8::Local<v8::Module> getCompiledModule(); //< This module is NOT ready to be run, since Identifiers won't be resolved.
    void resolveLocalIdentifiers();
    void resolveLocalXRefs();
    void resolveImportedIdentifiers();
    virtual void evaluate() override;
    bool isES6Module();
    void compileModule();

private:
    std::filesystem::path path;
    std::string originalSource;
    std::future<AstRoot*> astFuture;
    std::unique_ptr<AstRoot> ast;
    v8::Persistent<v8::Module> compiledModule;
    v8::Persistent<v8::Module> compiledThunkModule; //< ES6 thunk generated if this module doesn't use ES6 import/exports
    bool importsResolved = false; //< Breaks cycles when manually instantiating all imports
    bool localIdentifierResolutionDone = false; //< True after we've run the identifiers resolution pass
    std::vector<std::string> missingContextIdentifiers;
    std::unordered_map<Identifier*, Identifier*> resolvedLocalIdentifiers; //< Maps identifiers to their local declaration
    bool importedIdentifierResolutionDone = false; //< True after we're run the imported identifiers resolution pass
    std::unordered_map<ImportSpecifier*, Identifier*> resolvedImportedIdentifiers; //< Maps named imports to their declaration in the imported module
    bool localXRefsDone = false;
    std::unordered_map<Identifier*, std::vector<Identifier*>> localXRefs; //< Maps local declarations to their previously resolved uses
};

#endif // MODULE_HPP
