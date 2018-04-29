#include "global.hpp"
#include "isolatewrapper.hpp"
#include "module/moduleresolver.hpp"
#include <iostream>
#include <memory>
#include <filesystem>
#include <v8.h>

using namespace std;
namespace fs = filesystem;
using json = nlohmann::json;

void helpAndDie(const char* selfPath)
{
    cout << "Usage: " << selfPath << " <file.js | project_dir>" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    if (argc != 2)
        helpAndDie(argv[0]);

    fs::path argPath(argv[1]);
    fs::path mainFilePath;
    bool singleFile;
    if (fs::is_directory(argPath)) {
        mainFilePath = ModuleResolver::getProjectMainFile(argPath);
        singleFile = false;
    } else {
        mainFilePath = argPath;
        singleFile = true;
    }

    IsolateWrapper isolateWrapper;
    Module& mainModule = (Module&)ModuleResolver::getModule(isolateWrapper, "/", argv[1], true);

    if (singleFile) {
        mainModule.analyze();
        return EXIT_SUCCESS;
    }

    mainModule.resolveProjectImports(argPath); // Loads all the project modules (and other dependencies)
    vector<Module*> projectModules = ModuleResolver::getLoadedProjectModules(argPath);
    for (Module* module : projectModules)
        module->analyze();

    return EXIT_SUCCESS;
}
