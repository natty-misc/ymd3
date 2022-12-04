//
// Created by Natty on 01.03.2021.
//

#include <cstdio>
#include <string>
#include <filesystem>
#include <regex>

#include <libplatform/libplatform.h>
#include <iostream>

#include "retrieverscript.h"
#include "version.h"

#include <curl/curl.h>


std::unique_ptr<v8::Platform> platform;
YMD::ScriptingEngine* engineInstance = nullptr;

static void getVersion(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 0)
    {
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8Literal(args.GetIsolate(), "Bad parameters"));

        return;
    }

    const std::string version(YMD_VERSION);

    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), &version[0], v8::NewStringType::kNormal).ToLocalChecked());
}

static void log(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    for (int i = 0; i < args.Length(); i++)
    {
        std::cout << "[LOG] " << *v8::String::Utf8Value(args.GetIsolate(), args[i]) << std::endl;
    }
}

static void retrieve(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    v8::EscapableHandleScope scope(isolate);

    if (args.Length() != 1)
    {
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8Literal(args.GetIsolate(), "Bad parameters: Missing required parameter 'url'."));
        return;
    }

    v8::Local<v8::Value> param = args[0];

    if (!param->IsString())
    {
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8Literal(args.GetIsolate(), "Bad parameter 'url': Must be a string."));
        return;
    }

    v8::Local<v8::String> url = param.As<v8::String>();
    v8::String::Utf8Value utf8(args.GetIsolate(), url);
    char* strURL = *utf8;

    std::cout << "Retrieval from " << strURL << " requested." << std::endl;

    CURL *curl = curl_easy_init();

    if (!curl)
    {
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8Literal(args.GetIsolate(), "Failed to init cURL."));
        return;
    }

    // CURLOPT_WRITEFUNCTION requires a function pointer with these types, luckily the pointers don't matter
    using CURLWriteFuncPtr = decltype(&std::fwrite);
    CURLWriteFuncPtr writeFunction = [](const void* ptr, size_t size, size_t nmemb, auto* stream) -> size_t {
        auto* srcPtr = static_cast<const char*>(ptr);
        auto* destPtr = reinterpret_cast<std::vector<char>*>(stream);
        std::copy(srcPtr, srcPtr + size * nmemb, std::back_inserter(*destPtr));
        return nmemb;
    };

    std::vector<char> data;

    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, strURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        char err[1024];
        snprintf(err, sizeof(err), "cURL error: %s\n", curl_easy_strerror(res));
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8Literal(args.GetIsolate(), err));
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);

    std::string resultStr(data.cbegin(), data.cend());
    v8::Local<v8::String> result = v8::String::NewFromUtf8(isolate, resultStr.c_str(), v8::NewStringType::kNormal, resultStr.size()).ToLocalChecked();
    args.GetReturnValue().Set(scope.Escape(result));
}

YMD::ScriptingEngine::ScriptingEngine(const std::filesystem::path& execLocation)
{
    if (engineInstance != nullptr)
    {
        throw std::runtime_error("Cannot have multiple scripting engine instances!");
    }

    std::cout << "Initializing V8..." << std::endl;

    const auto pathName = execLocation.string();

    v8::V8::InitializeICUDefaultLocation(pathName.c_str());
    v8::V8::InitializeExternalStartupData(pathName.c_str());

    platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    engineInstance = this;
}

YMD::ScriptingEngine::~ScriptingEngine()
{
    std::cout << "Destroying V8..." << std::endl;

    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    platform.reset();
    engineInstance = nullptr;
}

YMD::RetrieverScript::RetrieverScript(const std::string& name)
{
    std::regex scriptPattern(R"([a-zA-Z0-9_-]+)");

    if (!std::regex_match(name, scriptPattern))
        throw std::runtime_error("Invalid script name: " + name);

    const std::filesystem::path baseScriptDir("../data/scripts");
    const std::filesystem::path scriptPath = baseScriptDir / (name + ".js");

    this->source = loadSourceFromPath(scriptPath);

    this->isolateCreateParams.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    this->isolate = v8::Isolate::New(this->isolateCreateParams);
}

YMD::RetrieverScript::~RetrieverScript()
{
    this->isolate->Dispose();
    delete this->isolateCreateParams.array_buffer_allocator;
};

std::string YMD::RetrieverScript::loadSourceFromPath(const std::filesystem::path& path)
{
    if (!std::filesystem::is_regular_file(path))
        throw std::runtime_error("Script not found: " + path.string());

    std::ifstream input(path, std::ios::binary);

    if (!input)
        throw std::runtime_error("Failed to open file: " + path.string());

    const size_t codeSize = std::filesystem::file_size(path);

    std::string source(codeSize, '\0');
    input.read(source.data(), codeSize);

    return source;
}

std::optional<YMD::RetrieverResult> YMD::RetrieverScript::run(const std::string& inputURL) const
{
    v8::Isolate::Scope isolate_scope(isolate);

    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);

    v8::Local<v8::ObjectTemplate> ymdObj = v8::ObjectTemplate::New(isolate);
    global->Set(isolate, "YMD", ymdObj);

    ymdObj->Set(isolate, "getVersion", v8::FunctionTemplate::New(isolate, getVersion));
    ymdObj->Set(isolate, "retrieve", v8::FunctionTemplate::New(isolate, retrieve));
    ymdObj->Set(isolate, "log", v8::FunctionTemplate::New(isolate, log));
    ymdObj->Set(isolate, "inputURL", v8::String::NewFromUtf8(isolate, inputURL.c_str()).ToLocalChecked());

    v8::Local<v8::Context> context = v8::Context::New(isolate, nullptr, global);

    v8::Context::Scope context_scope(context);

    std::optional<RetrieverResult> ymdResult;

    try
    {
        this->compileAndRun(context, loadSourceFromPath("../data/scripts/dom.js"));

        this->compileAndRun(context, this->source);

        v8::Local<v8::Object> resultGlobal = context->Global();
        v8::Local<v8::Object> resultYmd = resultGlobal->Get(context, v8::String::NewFromUtf8(isolate, "YMD").ToLocalChecked()).ToLocalChecked().As<v8::Object>();

        auto getProperty = [this, &context, &resultYmd] (const char name[], bool required = true) -> std::optional<std::string> {
            v8::Local<v8::String> propName = v8::String::NewFromUtf8(this->isolate, name).ToLocalChecked();
            v8::MaybeLocal<v8::Value> result = resultYmd->Get(context, propName);

            if (!result.IsEmpty())
            {
                return *v8::String::Utf8Value(isolate, result.ToLocalChecked());
            }
            else if (required)
            {
                throw ExecutionFailure(std::string("Missing required result property: ") + name);
            }

            return std::optional<std::string>();
        };

        ymdResult = RetrieverResult();
        ymdResult->originalURL = *getProperty("videoURL");
        ymdResult->videoName = *getProperty("videoName");
        ymdResult->videoAuthor = getProperty("videoAuthor", false);
        ymdResult->downloadURLs = { *getProperty("downloadURL") };
    }
    catch (ExecutionFailure& e)
    {
        std::cerr << e.what() << std::endl;
        return std::optional<RetrieverResult>();
    }

    return ymdResult;
}

v8::MaybeLocal<v8::Value> YMD::RetrieverScript::compileAndRun(v8::Local<v8::Context>& context, const std::string& sourceStr) const
{
    const char* sourceBuf = sourceStr.c_str();
    const int sourceLength = static_cast<int>(sourceStr.size());

    v8::Local<v8::String> src = v8::String::NewFromUtf8(isolate,
                                                        sourceBuf,
                                                        v8::NewStringType::kNormal,
                                                        sourceLength).ToLocalChecked();

    v8::TryCatch tryCatch(isolate);

    v8::MaybeLocal<v8::Script> compileResult = v8::Script::Compile(context, src);

    if (!compileResult.IsEmpty())
    {
        v8::Local<v8::Script> script = compileResult.ToLocalChecked();

        v8::MaybeLocal<v8::Value> result = script->Run(context);

        if (tryCatch.HasCaught())
        {
            std::stringstream errBuf;

            std::cerr << "Runtime error!\n";
            v8::Local<v8::Message> message = tryCatch.Message();
            std::cerr << "Line: " << message->GetLineNumber(context).ToChecked() << '\n';
            std::cerr << "Column: " << message->GetStartColumn(context).ToChecked() << '\n';
            std::cerr << "Offset: " << message->GetStartPosition() << '\n';
            v8::Local<v8::String> line = message->GetSourceLine(context).ToLocalChecked();
            v8::String::Utf8Value lineStr(isolate, line);
            std::cerr << "Offending line:\n" << *lineStr << '\n';
            v8::String::Utf8Value errMessage(isolate, tryCatch.Exception());
            std::cerr << "Message:\n" << *errMessage << std::endl;
        }

        return result;
    }
    else
    {
        std::stringstream errBuf;

        errBuf << "Failed to compile code!\n";
        v8::Local<v8::Message> message = tryCatch.Message();
        errBuf << "Line: " << message->GetLineNumber(context).ToChecked() << '\n';
        errBuf << "Column: " << message->GetStartColumn(context).ToChecked() << '\n';
        errBuf << "Offset: " << message->GetStartPosition() << '\n';
        v8::Local<v8::String> line = message->GetSourceLine(context).ToLocalChecked();
        v8::String::Utf8Value lineStr(isolate, line);
        errBuf << "Offending line:\n" << *lineStr << '\n';
        v8::String::Utf8Value errMessage(isolate, tryCatch.Exception());
        errBuf << "Message:\n" << *errMessage;

        throw ExecutionFailure(errBuf.str());
    }
}

YMD::RetrieverScript::ExecutionFailure::ExecutionFailure(const std::string& errMessage) : runtime_error(errMessage)
{

}
