#include "global.hpp"
#include "isolatewrapper.hpp"
#include "module/moduleresolver.hpp"
#include "reporting.hpp"
#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <v8.h>

using namespace std;
namespace fs = filesystem;
using json = nlohmann::json;

void helpAndDie(const char* selfPath)
{
    cout << "Usage: " << selfPath << " [options] <file.js | project_dir>" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    if (argc < 2)
        helpAndDie(argv[0]);

    bool debug = false;
    for (int c; (c = getopt(argc, argv, "d")) != -1;) {
        switch (c) {
        case 'd':
            debug = 1;
            break;
        case '?':
            if (optopt == 'c')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            return EXIT_FAILURE;
        default:
            throw std::runtime_error("getopt() failed hard =(");
        }
    }
    setDebug(debug);

    fs::path argPath(argv[optind]);
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
    Module& mainModule = (Module&)ModuleResolver::getModule(isolateWrapper, "/", argv[optind], true);

    if (singleFile) {
        mainModule.analyze();
        return EXIT_SUCCESS;
    }

    cout << "Resolving project imports..." << endl;
    mainModule.resolveProjectImports(argPath); // Loads all the project modules (and other dependencies)
    vector<Module*> projectModules = ModuleResolver::getLoadedProjectModules(argPath);
    cout << "Starting analysis..." << endl;
    for (Module* module : projectModules)
        module->analyze();

    return EXIT_SUCCESS;
}
