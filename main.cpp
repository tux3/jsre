#include "global.hpp"
#include "isolatewrapper.hpp"
#include "module/moduleresolver.hpp"
#include <iostream>
#include <memory>
#include <v8.h>

using namespace std;
using json = nlohmann::json;

void helpAndDie(const char* selfPath)
{
    cout << "Usage: " << selfPath << " <file.js>" << endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
        helpAndDie(argv[0]);

    IsolateWrapper isolateWrapper;
    //prepareGlobalObject(isolateWrapper);

    Module& mainModule = (Module&)ModuleResolver::getModule(isolateWrapper, "/", argv[1], true);

    v8::HandleScope handleScope(*isolateWrapper);
    mainModule.analyze();
    //mainModule.getCompiledModule();
    //mainModule.getExports();

    return 0;
}
