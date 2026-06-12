// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "UserUtils.h"
#include "Evaluator.h"
#include "Optional.h"
#include "Base64.h"
#include "Procedure.h"

#include <unistd.h>
#include <fcntl.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>
#include <map>
#include <stdexcept>
#include <limits>
#include <vector>
#include <sstream>

using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Error;
using ComplianceEngine::Evaluator;
using ComplianceEngine::action_func_t;

// Tells libfuzzer to skip the input when it doesn't contain a valid target
static const int c_skip_input = -1;

// Tells libfuzzer the input was valid and may be used to create a new corpus input
static const int c_valid_input = 0;

struct size_range
{
    std::size_t min = 1;
    std::size_t max = std::numeric_limits<std::size_t>::max();

    size_range() = default;
    size_range(std::size_t min, std::size_t max) : min(min), max(max) {}
};

struct Context
{
    std::string tempdir;

    Context() noexcept(false)
    {
        char path[] = "/tmp/osconfig-fuzzer-XXXXXX";
        if(::mkdtemp(path) == nullptr)
        {
            throw std::runtime_error(std::string{ "failed to create temporary directory: " } + std::strerror(errno));
        }
        tempdir = path;
    }

    ~Context() noexcept
    {
        ::remove(tempdir.c_str());
    }

    std::string GenerateNextTemporaryFileName() const noexcept
    {
        static int counter = 0;
        return tempdir + "/" + std::to_string(counter++);
    }

    std::string MakeTemporaryFile(const char* data, std::size_t size) const noexcept(false)
    {
        auto path = GenerateNextTemporaryFileName();
        auto fd = ::open(path.c_str(), O_EXCL | O_CREAT | O_WRONLY | O_TRUNC, 0600);
        while (size)
        {
            auto written = ::write(fd, data, size);
            if (written == -1)
            {
                ::close(fd);
                throw std::runtime_error(std::string{ "failed to write to temporary file: " } + std::strerror(errno));
            }

            size -= written;
            data += written;
        }
        ::close(fd);

        return path;
    }

    std::string ExtractVariant(const char*& data, std::size_t& size, size_range range = size_range{}) const noexcept
    {
        auto variant = std::string(data, size);
        auto pos = variant.find('.');
        if (pos == std::string::npos || pos < range.min || pos > range.max)
        {
            return {};
        }

        data += pos + 1;
        size -= pos + 1;
        return variant.substr(0, pos);
    }

    void Remove(const std::string& path) const noexcept
    {
        ::remove(path.c_str());
    }
};

static Context* g_context = nullptr;

static Context& GetContext()
{
    if (g_context == nullptr)
    {
        throw std::runtime_error("Context not initialized. Call LLVMFuzzerInitialize first.");
    }
    return *g_context;
}

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    (void)argc;
    (void)argv;
    g_context = new Context();
    return 0;
}

static int LoadStringFromFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = GetContext().MakeTemporaryFile(data, size);
    free(LoadStringFromFile(filename.c_str(), true, nullptr));
    GetContext().Remove(filename);
    return 0;
}

static int GetNumberOfLinesInFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = GetContext().MakeTemporaryFile(data, size);
    GetNumberOfLinesInFile(filename.c_str());
    GetContext().Remove(filename);
    return 0;
}

static int AppendPayloadToFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = GetContext().MakeTemporaryFile(nullptr, 0);
    AppendPayloadToFile(filename.c_str(), data, size, nullptr);
    GetContext().Remove(filename);
    return 0;
}

static int DuplicateString_target(const char* data, std::size_t size) noexcept
{
    auto source = std::string(data, size);
    free(DuplicateString(source.c_str()));
    return 0;
}

static int ConcatenateStrings_target(const char* data, std::size_t size) noexcept
{
    auto a = GetContext().ExtractVariant(data, size);
    if (a.empty())
    {
        return c_skip_input;
    }

    auto b = std::string(data, size);
    free(ConcatenateStrings(a.c_str(), b.c_str()));
    return 0;
}

static int TruncateAtFirst_target(const char* data, std::size_t size) noexcept
{
    auto marker = GetContext().ExtractVariant(data, size, size_range{ 1, 1 });
    if (marker.empty())
    {
        return c_skip_input;
    }

    auto name = std::string(data, size);
    TruncateAtFirst(&name[0], marker.at(0));
    return 0;
}

static int IsDaemonActive_target(const char* data, std::size_t size) noexcept
{
    auto name = std::string(data, size);
    IsDaemonActive(name.c_str(), nullptr);
    return 0;
}

static int GetLoggingLevelFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    GetLoggingLevelFromJsonConfig(json.c_str(), nullptr);
    return 0;
}

static int GetMaxLogSizeFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    GetMaxLogSizeFromJsonConfig(json.c_str(), nullptr);
    return 0;
}

static int GetMaxLogSizeDebugMultiplierFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    GetMaxLogSizeDebugMultiplierFromJsonConfig(json.c_str(), nullptr);
    return 0;
}

static int Base64Decode_target(const char* data, std::size_t size) noexcept
{
    ComplianceEngine::Base64Decode(std::string(data, size));
    return c_valid_input;
}

static int ProcedureUpdateUserParameters_target(const char* data, std::size_t size) noexcept
{
    auto input = std::string(data, size);
    for (auto& c : input)
    {
        if (!std::isspace(c) && !std::isprint(c))
        {
            return c_skip_input;
        }
    }
    ComplianceEngine::Procedure proc;
    ComplianceEngine::ProcedureParameters params;
    params["X"] = "1";
    params["Y"] = "2";
    proc.SetParameters(std::move(params));
    Optional<Error> error = proc.UpdateUserParameters(input);
    if (error)
    {
        // printf("Error: %s\n", error->message.c_str());
    }
    else
    {
        for (auto& param : proc.Parameters())
        {
            // printf("Parameter: %s = %s\n", param.first.c_str(), param.second.c_str());
        }
    }
    return 0;
}

static int CheckOrEnsureUsersDontHaveDotFiles_target(const char* data, std::size_t size) noexcept
{
    auto username = std::string(data, size);
    char* reason = nullptr;
    CheckOrEnsureUsersDontHaveDotFiles(username.c_str(), false, &reason, nullptr);
    free(reason);
    return 0;
}

static int CheckUserAccountsNotFound_target(const char* data, std::size_t size) noexcept
{
    auto usernames = std::string(data, size);
    char* reason = nullptr;
    CheckUserAccountsNotFound(usernames.c_str(), &reason, nullptr);
    free(reason);
    return 0;
}

// List of supported fuzzing targets.
// The key is taken from the input data and is used to determine which target to call.
static const std::map<std::string, int (*)(const char*, std::size_t)> g_targets = {
    { "GetNumberOfLinesInFile.", GetNumberOfLinesInFile_target },
    { "LoadStringFromFile.", LoadStringFromFile_target },
    { "AppendPayloadToFile.", AppendPayloadToFile_target },
    { "DuplicateString.", DuplicateString_target },
    { "ConcatenateStrings.", ConcatenateStrings_target },
    { "TruncateAtFirst.", TruncateAtFirst_target },
    { "IsDaemonActive.", IsDaemonActive_target },
    { "GetLoggingLevelFromJsonConfig.", GetLoggingLevelFromJsonConfig_target },
    { "GetMaxLogSizeFromJsonConfig.", GetMaxLogSizeFromJsonConfig_target },
    { "GetMaxLogSizeDebugMultiplierFromJsonConfig.", GetMaxLogSizeDebugMultiplierFromJsonConfig_target },
    { "Base64Decode.", Base64Decode_target },
    { "ProcedureUpdateUserParameters.", ProcedureUpdateUserParameters_target },
    { "CheckOrEnsureUsersDontHaveDotFiles.", CheckOrEnsureUsersDontHaveDotFiles_target },
    { "CheckUserAccountsNotFound.", CheckUserAccountsNotFound_target },
};

// libfuzzer entry point
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto* input = reinterpret_cast<const char*>(data);
    const auto* prefix = reinterpret_cast<const char*>(std::memchr(input, '.', size));
    if (prefix == nullptr)
    {
        // Separator not found, skip the input
        return c_skip_input;
    }

    // Include the separator
    prefix++;
    const auto prefix_size = prefix - input;
    auto it = g_targets.find(std::string(input, prefix_size));
    if(it == g_targets.end())
    {
        // Target mismatch, skip the input
        return c_skip_input;
    }

    return it->second(prefix, size - prefix_size);
}
