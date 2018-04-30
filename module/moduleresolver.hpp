#ifndef MODULERESOLVER_HPP
#define MODULERESOLVER_HPP

#include "module.hpp"
#include "nativemodule.hpp"
#include <filesystem>
#include <string>
#include <v8.h>

class IsolateWrapper;

class ModuleResolver {
public:
    static BasicModule& getModule(const BasicModule& from, std::string requestedName, bool isImport = false);
    static BasicModule& getModule(IsolateWrapper& isolateWrapper, std::filesystem::path basePath, std::string requestedName, bool isImport = false);
    static std::filesystem::path getProjectMainFile(std::filesystem::path projectDir);
    static bool isProjectModule(std::filesystem::path projectDir, std::filesystem::path filePath);
    static bool isProjectModule(std::filesystem::path projectDir, std::filesystem::path basePath, std::string requestedName);
    static std::vector<Module*> getLoadedProjectModules(std::filesystem::path projectDir);
    static void requireFunction(const v8::FunctionCallbackInfo<v8::Value>& args);
    static v8::MaybeLocal<v8::Module> resolveImportCallback(v8::Local<v8::Context> context, v8::Local<v8::String> specifier, v8::Local<v8::Module> referrer);

private:
    static std::filesystem::path resolve(std::filesystem::path fromPath, std::string requestedName);
    static std::filesystem::path resolveAsFile(std::filesystem::path path);
    static std::filesystem::path resolveAsDirectory(std::filesystem::path path);
    static std::filesystem::path resolveNodeModule(std::filesystem::path basePath, std::string requestedName);
    static std::string getNodeModuleMainFile(std::filesystem::path packageFilePath);

private:
    static std::unordered_map<std::string, NativeModule> nativeModuleMap;
    static std::unordered_map<std::string, Module> moduleMap;
    // Maps from the v8 identity hash of a v8 module to our Module class
    static std::unordered_map<int, Module&> compiledModuleMap;
};

#endif // MODULERESOLVER_HPP
