#include "moduleresolver.hpp"
#include "utils.hpp"
#include "isolatewrapper.hpp"
#include "reporting.hpp"
#include "analyze/identresolution.hpp"
#include <filesystem>
#include <iostream>
#include <json.hpp>
#include <v8.h>

using namespace std;
namespace fs = filesystem;
using json = nlohmann::json;

unordered_map<std::string, NativeModule> ModuleResolver::nativeModuleMap;
unordered_map<std::string, Module> ModuleResolver::moduleMap;
unordered_map<int, Module&> ModuleResolver::compiledModuleMap;

BasicModule& ModuleResolver::getModule(const BasicModule& from, string requestedName, bool isImport)
{
    return getModule(from.getIsolateWrapper(), from.getPath(), requestedName, isImport);
}

BasicModule& ModuleResolver::getModule(IsolateWrapper& isolateWrapper, std::filesystem::path basePath, std::string requestedName, bool isImport)
{
    if (!isImport && NativeModule::hasModule(requestedName))
        return nativeModuleMap.try_emplace(requestedName, isolateWrapper, requestedName).first->second;

    if (basePath == "<builtin>") {
        throw runtime_error("Trying to import a non-native module from a builtin native module!");
    }

    fs::path fullPath = resolve(basePath, requestedName);
    if (fullPath.empty())
        throw runtime_error("Cannot find module " + requestedName + " imported from " + basePath.c_str());

    v8::HandleScope scope(*isolateWrapper);
    fullPath = fs::canonical(fullPath);
    auto& module = moduleMap.try_emplace(fullPath, isolateWrapper, fullPath).first->second;
    compiledModuleMap.emplace(module.getCompiledModuleIdentityHash(), module);
    return module;
}

fs::path ModuleResolver::getProjectMainFile(fs::path projectDir)
{
    fs::path packageJsonPath = projectDir / "package.json";
    if (!fs::is_regular_file(packageJsonPath))
        fatal("Could not find a package.json in "+projectDir.string());
    return projectDir / getNodeModuleMainFile(packageJsonPath);
}

bool ModuleResolver::isProjectModule(fs::path projectDir, fs::path filePath)
{
    fs::path relative = std::filesystem::relative(filePath, projectDir);
    return relative.begin()->string() != ".."
            && relative.string().find("node_modules") == string::npos;
}

std::vector<Module *> ModuleResolver::getLoadedProjectModules(fs::path projectDir)
{
    vector<Module*> projectMods;
    for (auto& elem : moduleMap) {
        auto& mod = elem.second;
        if (!isProjectModule(projectDir, mod.getPath()))
            continue;
        projectMods.push_back(&mod);
    }
    return projectMods;
}

fs::path ModuleResolver::resolve(fs::path fromPath, string requestedName)
{
    fs::path basePath = fromPath;
    if (fs::is_regular_file(fromPath))
            basePath = basePath.remove_filename();
    fs::path fullPath;

    if (requestedName[0] == '/')
        basePath = basePath.root_path();

    if (requestedName[0] == '/' || (requestedName[0] == '.' && (requestedName[1] == '/' || (requestedName[1] == '.' && requestedName[2] == '/')))) {
        if (fullPath = resolveAsFile(basePath / requestedName); !fullPath.empty())
            return fullPath;
        if (fullPath = resolveAsDirectory(basePath / requestedName); !fullPath.empty())
            return fullPath;
    }

    return resolveNodeModule(basePath, requestedName);
}

fs::path ModuleResolver::resolveNodeModule(fs::path basePath, string requestedName)
{
    fs::path modulesDirPath = basePath / "node_modules";
    if (fs::is_directory(modulesDirPath)) {
        fs::path modulePath = modulesDirPath / requestedName;
        if (auto fullPath = resolveAsFile(modulePath); !fullPath.empty())
            return fullPath;
        if (auto fullPath = resolveAsDirectory(modulePath); !fullPath.empty())
            return fullPath;
    }

    fs::path parentPath = basePath.parent_path();
    if (parentPath != basePath)
        return resolveNodeModule(parentPath, requestedName);
    else
        return {};
}

fs::path ModuleResolver::resolveAsFile(fs::path path)
{
    if (fs::is_regular_file(path))
        return path;
    else if (fs::is_regular_file((string)path + ".js"))
        return (string)path + ".js";
    else
        return fs::path();
}

fs::path ModuleResolver::resolveAsDirectory(fs::path path)
{
    fs::path basePath;
    if (fs::is_regular_file(path / "package.json")) {
        basePath = path / getNodeModuleMainFile(path / "package.json");
        if (auto path = resolveAsFile(basePath); !path.empty())
            return path;
    } else {
        basePath = path;
    }

    if (fs::is_regular_file(basePath / "index.js"))
        return basePath / "index.js";
    else
        return fs::path();
}

string ModuleResolver::getNodeModuleMainFile(fs::path packageFilePath)
{
    json contents = json::parse(readFileStr(packageFilePath.c_str()));
    auto it = contents.find("main");
    if (it == contents.end())
        return {};
    return *it;
}

void ModuleResolver::requireFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1)
        return;

    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Value> arg = args[0];
    v8::String::Utf8Value requested(isolate, arg);
    v8::String::Utf8Value modulePath(isolate, args.Data());
    trace("require() from "s+*modulePath+" for module \""+*requested+"\"");

    Module& module = moduleMap.at(*modulePath);
    BasicModule* importedModule;
    try {
        importedModule = &getModule(module, *requested);
    } catch (const std::runtime_error& e) {
        auto errorStr = std::string("Cannot find module '")+*requested+"': "+e.what();
        isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate, errorStr.c_str())));
        return;
    }

    v8::Local<v8::Object> exports = importedModule->getExports();

//    auto context = isolate->GetCurrentContext();
//    v8::Context::Scope contextScope(context);
//    auto props = exports->GetOwnPropertyNames(context).ToLocalChecked();
//    trace("End of require for "s+*requested+" from "+*modulePath+", found module at "+importedModule->getPath()+", got "+to_string(props->Length())+" properties");
//    for (uint32_t i=0; i<props->Length(); ++i) {
//        auto nameStr = std::string(*v8::String::Utf8Value(isolate, props->Get(i).As<v8::String>()));
//        trace("   "+nameStr);
//    }

    v8::ReturnValue<v8::Value> returnValue = args.GetReturnValue();
    returnValue.Set(exports);
}

v8::MaybeLocal<v8::Module> ModuleResolver::resolveImportCallback(v8::Local<v8::Context> context, v8::Local<v8::String> specifier, v8::Local<v8::Module> referrer)
{
    v8::Isolate* isolate = context->GetIsolate();
    v8::EscapableHandleScope handleScope(isolate);
    v8::String::Utf8Value specifierStr(isolate, specifier);
    Module& referrerModule = compiledModuleMap.at(referrer->GetIdentityHash());
    string referrerPath = referrerModule.getPath();
    trace("import from "s+referrerPath+" for module \""+*specifierStr+"\"");

    if (NativeModule::hasModule(*specifierStr))
        return nativeModuleMap.try_emplace(*specifierStr, referrerModule.getIsolateWrapper(), *specifierStr).first->second.getWrapperModule();

    Module& importedModule = reinterpret_cast<Module&>(getModule(referrerModule, *specifierStr, true));
    v8::Local<v8::Module> importedCompiledModule = importedModule.getExecutableES6Module();

    return handleScope.Escape(importedCompiledModule);
}
