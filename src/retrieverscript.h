//
// Created by Natty on 01.03.2021.
//

#ifndef YMD3_RETRIEVERSCRIPT_H
#define YMD3_RETRIEVERSCRIPT_H

#include <filesystem>
#include <optional>
#include <v8.h>

namespace YMD
{
    class ScriptingEngine
    {
        public:
            explicit ScriptingEngine(const std::filesystem::path& execLocation);
            ScriptingEngine(ScriptingEngine&&) = delete;
            ScriptingEngine(const ScriptingEngine&) = delete;
            ScriptingEngine(ScriptingEngine&) = delete;

            ~ScriptingEngine();
    };

    struct RetrieverResult
    {
        std::string originalURL;
        std::vector<std::string> downloadURLs;
        std::string videoName;
        std::optional<std::string> videoAuthor;
    };

    class RetrieverScript
    {
        public:
            explicit RetrieverScript(const std::string& name);

            RetrieverScript(RetrieverScript&&) = delete;
            RetrieverScript(const RetrieverScript&) = delete;
            RetrieverScript(RetrieverScript&) = delete;

            ~RetrieverScript();

            [[nodiscard]] std::optional<RetrieverResult> run(const std::string& inputURL) const;

        private:
            class ExecutionFailure : public std::runtime_error
            {
                public:
                    explicit ExecutionFailure(const std::string& errMessage);
                    ~ExecutionFailure() override = default;
            };

            static std::string loadSourceFromPath(const std::filesystem::path& path);

            v8::MaybeLocal<v8::Value> compileAndRun(v8::Local<v8::Context>& context, const std::string& sourceStr) const;

            v8::Isolate* isolate;
            v8::Isolate::CreateParams isolateCreateParams;

            std::string source;
    };
}

#endif //YMD3_RETRIEVERSCRIPT_H
