#include "ast/parse.hpp"
#include "ast/import.hpp"
#include "utils/utils.hpp"
#include "utils/reporting.hpp"
#include "v8/isolatewrapper.hpp"
#include <atomic>
#include <thread>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <fstream>
#include <v8.h>

#include "module/module.hpp"

using namespace std;

extern "C" {
extern const char babelScriptStart[];
extern uint32_t babelScriptSize;
}

struct ParseWorkPackage
{
    Module& module;
    const std::string& source;
    promise<AstRoot*> astPromise;
    bool keepComments;
};

static const char babelCompileCacheFileName[] = "babel_compile_cache.bin";
static vector<uint8_t> babelCompileCacheBackingVector;
static atomic<v8::ScriptCompiler::CachedData*> babelCompileCache;

static atomic_int workersStarted{0};
static atomic_bool workersStopFlag{false};
static vector<thread> workers;
static vector<ParseWorkPackage> workQueue;
static std::condition_variable condvar;
static std::mutex condvar_mutex;

static void loadBabelCompileCacheFile();
static void writeBabelCompileCacheFile(v8::ScriptCompiler::CachedData* cachedData);
static v8::Local<v8::Object> makeBabelObject(IsolateWrapper& isolateWrapper);
static v8::Local<v8::Object> parseSourceScript(IsolateWrapper& isolateWrapper, v8::Local<v8::Object> babelObject, const std::string& scriptSource);

static void worker_thread_loop()
{
    using namespace v8;

    IsolateWrapper isolateWrapper;
    HandleScope handleScope(*isolateWrapper);
    Local<Object> babelObj = makeBabelObject(isolateWrapper);

    unique_lock condvar_lock(condvar_mutex);
    workersStarted++;

    while (!workersStopFlag.load(memory_order::memory_order_acquire)) {
        if (workQueue.empty()) {
            condvar.wait(condvar_lock);
            continue;
        }
        ParseWorkPackage package = move(workQueue.back());
        workQueue.pop_back();
        condvar_lock.unlock();

        Isolate* isolate = isolateWrapper.get();
        Isolate::Scope isolateScope(isolate);
        HandleScope handleScope(isolate);
        Local<Context> context = Context::New(*isolateWrapper);
        Context::Scope contextScope(context);

        auto astObj = parseSourceScript(isolateWrapper, babelObj, package.source);
        auto ast = importBabylonAst(package.module, astObj, package.keepComments);
        package.astPromise.set_value(ast);

        condvar_lock.lock();
    }

    workersStarted--;
}

std::future<AstRoot*> parseSourceScriptAsync(Module &parentModule, const std::string& script, bool keepComments)
{
    assert(workersStopFlag.load(memory_order::memory_order_acquire) == false);
    lock_guard condvar_lock(condvar_mutex);

    promise<AstRoot*> promise;
    future<AstRoot*> future = promise.get_future();

    ParseWorkPackage package{parentModule, script, move(promise), keepComments};
    workQueue.push_back(move(package));
    condvar.notify_one();

    return future;
}

static v8::Local<v8::Object> makeBabelObject(IsolateWrapper& isolateWrapper)
{
    using namespace v8;

    Isolate* isolate = isolateWrapper.get();
    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    TryCatch trycatch(isolate);

    Local<String> babelSourceStr = String::NewFromUtf8(isolate, babelScriptStart,
                                       NewStringType::kNormal, static_cast<int>(babelScriptSize))
                                       .ToLocalChecked();

    ScriptCompiler::CachedData* cachedData = babelCompileCache.load();
    if (cachedData) // ScriptCompiler::Source takes ownership, so we need a copy
        cachedData = new ScriptCompiler::CachedData(cachedData->data, cachedData->length);
    ScriptCompiler::Source source(babelSourceStr, cachedData);
    Local<UnboundScript> unboundScript;

    auto compileFlags = cachedData ? ScriptCompiler::kConsumeCodeCache : ScriptCompiler::kEagerCompile;
    if (!ScriptCompiler::CompileUnboundScript(isolate, &source, compileFlags).ToLocal(&unboundScript)) {
        reportV8Exception(isolate, &trycatch);
        throw std::runtime_error("makeBabelObject: Error compiling babel script");
    }

    if (!cachedData) {
        cachedData = ScriptCompiler::CreateCodeCache(unboundScript);
        ScriptCompiler::CachedData* expected = nullptr;
        if (babelCompileCache.compare_exchange_strong(expected, cachedData))
            writeBabelCompileCacheFile(cachedData);
    }

    auto script = unboundScript->BindToCurrentContext();
    script->Run(context).ToLocalChecked();
    Local<Object> babelObject = context->Global()->Get(String::NewFromUtf8(isolate, "babylon")).As<Object>();
    return handleScope.Escape(babelObject);
}

static v8::Local<v8::Object> parseSourceScript(IsolateWrapper& isolateWrapper, v8::Local<v8::Object> babelObject, const std::string& scriptSource)
{
    using namespace v8;

    Isolate* isolate = isolateWrapper.get();
    Isolate::Scope isolateScope(isolate);
    EscapableHandleScope handleScope(isolate);
    TryCatch trycatch(isolate);
    Local<Context> context = isolateWrapper.get()->GetCurrentContext();
    Local<Value> scriptSourceStr = String::NewFromUtf8(isolate, scriptSource.data(),
                                       NewStringType::kNormal, scriptSource.size())
                                       .ToLocalChecked();
    Local<Value> transformOptions;
    if (!JSON::Parse(context, String::NewFromUtf8(isolate, R"({
        "sourceMaps": "false",
        "plugins" : [
          "objectRestSpread",
          "classProperties",
          "exportExtensions",
          "asyncGenerators",
          "flow"
        ],
        "sourceType": "module"
    })")).ToLocal(&transformOptions)) {
        reportV8Exception(isolate, &trycatch);
        throw std::runtime_error("parseSourceScript: Failed to parse JSON options");
    }
    std::array arguments = { scriptSourceStr, transformOptions };

    Local<String> parseFunctionName = String::NewFromUtf8(isolate, "parse");
    Local<v8::Function> parseFunction = babelObject->Get(context, parseFunctionName).ToLocalChecked().As<v8::Function>();

    Local<Value> result;
    if (!parseFunction.As<v8::Function>()->Call(context, context->Global(), arguments.size(), arguments.data()).ToLocal(&result)
        || !result->IsObject()) {
        reportV8Exception(isolate, &trycatch);
        throw std::runtime_error("parseSourceScript: Failed to parse script");
    }

    return handleScope.Escape(result.As<Object>());
}

static void loadBabelCompileCacheFile()
{
    optional<vector<uint8_t>> data = tryReadCacheFile(babelCompileCacheFileName);
    if (!data.has_value())
        return;
    uint32_t versionTag;
    memcpy(&versionTag, data->data(), sizeof(versionTag));
    if (versionTag != v8::ScriptCompiler::CachedDataVersionTag()) {
        trace("Invalidating Babel compile cache (v8 version mismatch)");
        tryRemoveCacheFile(babelCompileCacheFileName);
        return;
    }

    babelCompileCacheBackingVector = move(*data);
    auto cachedData = new v8::ScriptCompiler::CachedData(babelCompileCacheBackingVector.data()+sizeof(versionTag),
                                                         babelCompileCacheBackingVector.size()-sizeof(versionTag));
    babelCompileCache.store(cachedData);
}


static void writeBabelCompileCacheFile(v8::ScriptCompiler::CachedData* cachedData)
{
    uint32_t versionTag = v8::ScriptCompiler::CachedDataVersionTag();
    vector<uint8_t> data(sizeof(versionTag)+cachedData->length);
    memcpy(data.data(), &versionTag, sizeof(versionTag));
    memcpy(data.data()+sizeof(versionTag), cachedData->data, cachedData->length);
    tryWriteCacheFile(babelCompileCacheFileName, data);
}

static unsigned workersCount()
{
    return std::max(4U, thread::hardware_concurrency()/2);
}

static void prepareOtherThreads()
{
    loadBabelCompileCacheFile();

    {
        lock_guard condvar_lock(condvar_mutex);
        auto count = workersCount();
        for (decltype(count) i = 0; i<count-1; ++i)
            workers.emplace_back(worker_thread_loop);
    }

    worker_thread_loop();
}

void startParsingThreads()
{
    lock_guard condvar_lock(condvar_mutex); // Taken for concurrent push into workers vector
    workersStopFlag.store(false, memory_order::memory_order_release);
    workers.emplace_back(prepareOtherThreads);
}

void stopParsingThreads()
{
    // Wait for the threads to be fully started before notify_all, should not take long and avoids deadlock in a crash-proof way.
    while (workersStarted != (int)workersCount())
        ;

    workersStopFlag.store(true, memory_order::memory_order_release);

    {
        lock_guard condvar_lock(condvar_mutex);
        condvar.notify_all();
    }

    for (auto& worker : workers)
        worker.join();
    workers.clear();
}
