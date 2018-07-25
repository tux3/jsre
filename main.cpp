#include "global.hpp"
#include "isolatewrapper.hpp"
#include "module/moduleresolver.hpp"
#include "reporting.hpp"
#include "ast/parse.hpp"
#include "utils.hpp"
#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <v8.h>

using namespace std;
namespace fs = filesystem;

void helpAndDie(const char* selfPath)
{
    cout << "Usage: " << selfPath << " [-s] [-d] <file.js | package.json | project_dir>" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    // Handle arguments
    if (argc < 2)
        helpAndDie(argv[0]);

    bool debug = false;
    bool suggest = false;
    for (int c; (c = getopt(argc, argv, "ds")) != -1;) {
        switch (c) {
        case 'd':
            debug = true;
            break;
        case 's':
            suggest = true;
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
    setSuggest(suggest);

    // Start real work
    IsolateWrapper isolateWrapper;
    vector<Module*> modulesToAnalyze;
    startParsingThreads();

    fs::path argPath(argv[optind]);
    if (argPath.is_relative())
        argPath = "./" / argPath; // In JS relative imports look like "./foo/bar" not "foo/bar", otherwise it refers to something in mode_modules

    if (fs::is_directory(argPath)) {
        vector<string> sourceFiles;
        findSourceFiles(argPath.lexically_normal(), sourceFiles);
        for (auto filePath : sourceFiles)
            modulesToAnalyze.push_back((Module*)&ModuleResolver::getModule(isolateWrapper, argPath, filePath, true));
    } else if (argPath.filename() == "package.json") {
        ModuleResolver::getProjectMainFile(argPath.remove_filename());
        cout << "Resolving project imports..." << endl;
        Module& mainModule = (Module&)ModuleResolver::getModule(isolateWrapper, fs::current_path(), argPath, true);
        mainModule.resolveProjectImports(argPath); // Loads all the project modules (and other dependencies)
        modulesToAnalyze = ModuleResolver::getLoadedProjectModules(argPath);
    } else {
        modulesToAnalyze.push_back((Module*)&ModuleResolver::getModule(isolateWrapper, fs::current_path(), argPath, true));
    }

    cout << "Starting analysis..." << endl;
    for (Module* module : modulesToAnalyze)
        module->analyze();

    // Cleanup
    stopParsingThreads();

    const auto& report = getReportingStatistics();
    cout << "Found " << report.errors << " error(s), " << report.warnings << " warning(s) and " << report.suggestions << " suggestion(s)." << endl;

    return EXIT_SUCCESS;
}
