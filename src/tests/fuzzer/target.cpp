// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Mmi.h"
#include "SecurityBaseline.h"
#include "CommonUtils.h"
#include "UserUtils.h"
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

// A struct to keep a single static initialization of the SecurityBaseline library
struct Context
{
    MMI_HANDLE handle;
    std::string tempdir;

    Context() noexcept(false)
    {
        char path[] = "/tmp/osconfig-fuzzer-XXXXXX";
        if(::mkdtemp(path) == nullptr)
        {
            throw std::runtime_error(std::string{ "failed to create temporary directory: " } + std::strerror(errno));
        }
        tempdir = path;

        SecurityBaselineInitialize();
        handle = SecurityBaselineMmiOpen("SecurityBaselineTest", 4096);
        if (handle == nullptr)
        {
            SecurityBaselineShutdown();
            throw std::runtime_error("failed to initialized SecurityBaseline library");
        }
    }

    ~Context() noexcept
    {
        ::remove(tempdir.c_str());
        SecurityBaselineMmiClose(handle);
        SecurityBaselineShutdown();
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

static Context g_context;

static int LoadStringFromFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.MakeTemporaryFile(data, size);
    free(LoadStringFromFile(filename.c_str(), true, nullptr));
    g_context.Remove(filename);
    return 0;
}

static int GetNumberOfLinesInFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.MakeTemporaryFile(data, size);
    GetNumberOfLinesInFile(filename.c_str());
    g_context.Remove(filename);
    return 0;
}

static int SavePayloadToFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.GenerateNextTemporaryFileName();
    SavePayloadToFile(filename.c_str(), data, size, nullptr);
    g_context.Remove(filename);
    return 0;
}

static int AppendPayloadToFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.MakeTemporaryFile(nullptr, 0);
    AppendPayloadToFile(filename.c_str(), data, size, nullptr);
    g_context.Remove(filename);
    return 0;
}

static int SecureSaveToFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.GenerateNextTemporaryFileName();
    SecureSaveToFile(filename.c_str(), data, size, nullptr);
    g_context.Remove(filename);
    return 0;
}

static int AppendToFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.MakeTemporaryFile(nullptr, 0);
    AppendToFile(filename.c_str(), data, size, nullptr);
    g_context.Remove(filename);
    return 0;
}

static int ReplaceMarkedLinesInFile_target(const char* data, std::size_t size) noexcept
{
    auto marker = g_context.ExtractVariant(data, size);
    if (marker.empty())
    {
        return c_skip_input;
    }

    auto newline = g_context.ExtractVariant(data, size);
    if (newline.empty())
    {
        return c_skip_input;
    }

    auto comment = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (comment.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    ReplaceMarkedLinesInFile(filename.c_str(), marker.c_str(), newline.c_str(), comment.at(0), true, nullptr);
    g_context.Remove(filename);
    return 0;
}

static int CheckFileSystemMountingOption_target(const char* data, std::size_t size) noexcept
{
    auto mountDirectory = g_context.ExtractVariant(data, size);
    if (mountDirectory.empty())
    {
        return c_skip_input;
    }

    auto mountType = g_context.ExtractVariant(data, size);
    if (mountType.empty())
    {
        return c_skip_input;
    }

    auto desiredOption = g_context.ExtractVariant(data, size);
    if (desiredOption.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    char* reason = nullptr;
    CheckFileSystemMountingOption(filename.c_str(), mountDirectory.c_str(), mountType.c_str(), desiredOption.c_str(), &reason, nullptr);
    g_context.Remove(filename);
    free(reason);
    return 0;
}

static int CharacterFoundInFile_target(const char* data, std::size_t size) noexcept
{
    auto what = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (what.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    CharacterFoundInFile(filename.c_str(), what.at(0));
    g_context.Remove(filename);
    return 0;
}

static int CheckNoLegacyPlusEntriesInFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.MakeTemporaryFile(data, size);
    char* reason = nullptr;
    CheckNoLegacyPlusEntriesInFile(filename.c_str(), &reason, nullptr);
    g_context.Remove(filename);
    free(reason);
    return 0;
}

static int FindTextInFile_target(const char* data, std::size_t size) noexcept
{
    auto text = g_context.ExtractVariant(data, size);
    if (text.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    FindTextInFile(filename.c_str(), text.c_str(), nullptr);
    g_context.Remove(filename);
    return 0;
}

static int CheckTextIsFoundInFile_target(const char* data, std::size_t size) noexcept
{
    auto text = g_context.ExtractVariant(data, size);
    if (text.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    char* reason = nullptr;
    CheckTextIsFoundInFile(filename.c_str(), text.c_str(), &reason, nullptr);
    g_context.Remove(filename);
    free(reason);
    return 0;
}

// Skipping CheckTextIsNotFoundInFile due to similarity

static int CheckMarkedTextNotFoundInFile_target(const char* data, std::size_t size) noexcept
{
    auto text = g_context.ExtractVariant(data, size);
    if (text.empty())
    {
        return c_skip_input;
    }

    auto marker = g_context.ExtractVariant(data, size);
    if (marker.empty())
    {
        return c_skip_input;
    }

    auto comment = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (comment.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    char* reason = nullptr;
    CheckMarkedTextNotFoundInFile(filename.c_str(), text.c_str(), marker.c_str(), comment.at(0), &reason, nullptr);
    g_context.Remove(filename);
    free(reason);
    return 0;
}

static int CheckTextNotFoundInEnvironmentVariable_target(const char* data, std::size_t size) noexcept
{
    auto variable = g_context.ExtractVariant(data, size);
    if (variable.empty())
    {
        return c_skip_input;
    }

    auto text = g_context.ExtractVariant(data, size);
    if (text.empty())
    {
        return c_skip_input;
    }

    auto strict = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (strict.empty())
    {
        return c_skip_input;
    }

    char* reason = nullptr;
    CheckTextNotFoundInEnvironmentVariable(variable.c_str(), text.c_str(), strict.at(0) == '1' ? true : false, &reason, nullptr);
    free(reason);
    return 0;
}

static int CheckSmallFileContainsText_target(const char* data, std::size_t size) noexcept
{
    auto text = g_context.ExtractVariant(data, size);
    if (text.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    char* reason = nullptr;
    CheckSmallFileContainsText(filename.c_str(), text.c_str(), &reason, nullptr);
    g_context.Remove(filename);
    free(reason);
    return 0;
}

static int CheckLineNotFoundOrCommentedOut_target(const char* data, std::size_t size) noexcept
{
    auto text = g_context.ExtractVariant(data, size);
    if (text.empty())
    {
        return c_skip_input;
    }

    auto comment = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (comment.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    char* reason = nullptr;
    CheckLineNotFoundOrCommentedOut(filename.c_str(), comment.at(0), text.c_str(), &reason, nullptr);
    g_context.Remove(filename);
    free(reason);
    return 0;
}

static int GetStringOptionFromBuffer_target(const char* data, std::size_t size) noexcept
{
    auto option = g_context.ExtractVariant(data, size);
    if (option.empty())
    {
        return c_skip_input;
    }

    auto separator = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (separator.empty())
    {
        return c_skip_input;
    }

    auto buffer = std::string(data, size);
    free(GetStringOptionFromBuffer(buffer.c_str(), option.c_str(), separator.at(0), nullptr));
    return 0;
}

static int GetIntegerOptionFromBuffer_target(const char* data, std::size_t size) noexcept
{
    auto option = g_context.ExtractVariant(data, size);
    if (option.empty())
    {
        return c_skip_input;
    }

    auto separator = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (separator.empty())
    {
        return c_skip_input;
    }

    auto buffer = std::string(data, size);
    GetIntegerOptionFromBuffer(buffer.c_str(), option.c_str(), separator.at(0), nullptr);
    return 0;
}

static int CheckLockoutForFailedPasswordAttempts_target(const char* data, std::size_t size) noexcept
{
    auto pamSo = g_context.ExtractVariant(data, size);
    if (pamSo.empty())
    {
        return c_skip_input;
    }

    auto comment = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (comment.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    char* reason = nullptr;
    CheckLockoutForFailedPasswordAttempts(filename.c_str(), pamSo.c_str(), comment.at(0), &reason, nullptr);
    g_context.Remove(filename);
    free(reason);
    return 0;
}

static int CheckPasswordCreationRequirements_target(const char* data, std::size_t size) noexcept
{
    try
    {
        auto integer = g_context.ExtractVariant(data, size);
        if (integer.empty())
        {
            return c_skip_input;
        }
        auto retry = std::stoi(integer);

        integer = g_context.ExtractVariant(data, size);
        if (integer.empty())
        {
            return c_skip_input;
        }
        auto minlen = std::stoi(integer);

        integer = g_context.ExtractVariant(data, size);
        if (integer.empty())
        {
            return c_skip_input;
        }
        auto minclass = std::stoi(integer);

        integer = g_context.ExtractVariant(data, size);
        if (integer.empty())
        {
            return c_skip_input;
        }
        auto dcredit = std::stoi(integer);

        integer = g_context.ExtractVariant(data, size);
        if (integer.empty())
        {
            return c_skip_input;
        }
        auto ucredit = std::stoi(integer);

        integer = g_context.ExtractVariant(data, size);
        if (integer.empty())
        {
            return c_skip_input;
        }
        auto ocredit = std::stoi(integer);

        auto lcredit = std::stoi(std::string(data, size));
        char* reason = nullptr;
        CheckPasswordCreationRequirements(retry, minlen, minclass, dcredit, ucredit, ocredit, lcredit, &reason, nullptr);
        free(reason);
        return 0;
    }
    catch(const std::exception& e)
    {
        return c_skip_input;
    }
}

static int GetStringOptionFromFile_target(const char* data, std::size_t size) noexcept
{
    auto option = g_context.ExtractVariant(data, size);
    if (option.empty())
    {
        return c_skip_input;
    }

    auto separator = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (separator.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    free(GetStringOptionFromFile(filename.c_str(), option.c_str(), separator.at(0), nullptr));
    g_context.Remove(filename);
    return 0;
}

static int GetIntegerOptionFromFile_target(const char* data, std::size_t size) noexcept
{
    auto option = g_context.ExtractVariant(data, size);
    if (option.empty())
    {
        return c_skip_input;
    }

    auto separator = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (separator.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    GetIntegerOptionFromFile(filename.c_str(), option.c_str(), separator.at(0), nullptr);
    g_context.Remove(filename);
    return 0;
}

static int CheckIntegerOptionFromFileEqualWithAny_target(const char* data, std::size_t size) noexcept
{
    auto option = g_context.ExtractVariant(data, size);
    if (option.empty())
    {
        return c_skip_input;
    }

    auto separator = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (separator.empty())
    {
        return c_skip_input;
    }

    static const std::size_t max_values = 1000;
    int* values = new int[max_values];
    std::size_t count = 0;
    while (count < max_values)
    {
        auto value = g_context.ExtractVariant(data, size);
        if (value.empty())
        {
            break;
        }

        try
        {
            values[count++] = std::stoi(value);
        }
        catch(const std::exception& e)
        {
            break;
        }
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    char* reason = nullptr;
    CheckIntegerOptionFromFileEqualWithAny(filename.c_str(), option.c_str(), separator.at(0), values, count, &reason, nullptr);
    g_context.Remove(filename);
    free(reason);
    delete[] values;
    return 0;
}

static int CheckIntegerOptionFromFileLessOrEqualWith_target(const char* data, std::size_t size) noexcept
{
    auto option = g_context.ExtractVariant(data, size);
    if (option.empty())
    {
        return c_skip_input;
    }

    auto separator = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (separator.empty())
    {
        return c_skip_input;
    }

    auto integer = g_context.ExtractVariant(data, size);
    if (integer.empty())
    {
        return c_skip_input;
    }

    int value;
    try
    {
        value = std::stoi(integer);
    }
    catch(const std::exception& e)
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    char* reason = nullptr;
    CheckIntegerOptionFromFileLessOrEqualWith(filename.c_str(), option.c_str(), separator.at(0), value, &reason, nullptr);
    g_context.Remove(filename);
    free(reason);
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
    auto a = g_context.ExtractVariant(data, size);
    if (a.empty())
    {
        return c_skip_input;
    }

    auto b = std::string(data, size);
    free(ConcatenateStrings(a.c_str(), b.c_str()));
    return 0;
}

static int DuplicateStringToLowercase_target(const char* data, std::size_t size) noexcept
{
    auto source = std::string(data, size);
    free(DuplicateStringToLowercase(source.c_str()));
    return 0;
}

static int ConvertStringToIntegers_target(const char* data, std::size_t size) noexcept
{
    auto separator = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (separator.empty())
    {
        return c_skip_input;
    }

    auto source = std::string(data, size);
    int* values = nullptr;
    int count = 0;
    ConvertStringToIntegers(source.c_str(), separator.at(0), &values, &count, nullptr);
    free(values);
    return 0;
}

static int RemoveCharacterFromString_target(const char* data, std::size_t size) noexcept
{
    auto what = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (what.empty())
    {
        return c_skip_input;
    }

    auto source = std::string(data, size);
    free(RemoveCharacterFromString(source.c_str(), what.at(0), nullptr));
    return 0;
}

static int ReplaceEscapeSequencesInString_target(const char* data, std::size_t size) noexcept
{
    auto escapes = g_context.ExtractVariant(data, size);
    if (escapes.empty())
    {
        return c_skip_input;
    }

    auto replacement = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (replacement.empty())
    {
        return c_skip_input;
    }

    auto source = std::string(data, size);
    free(ReplaceEscapeSequencesInString(source.c_str(), escapes.c_str(), escapes.size(), replacement.at(0), nullptr));
    return 0;
}

static int HashString_target(const char* data, std::size_t size) noexcept
{
    auto source = std::string(data, size);
    HashString(source.c_str());
    return 0;
}

static int ParseHttpProxyData_target(const char* data, std::size_t size) noexcept
{
    auto source = std::string(data, size);
    char* hostAddress = nullptr;
    int port = 0;
    char* username = nullptr;
    char* password = nullptr;
    ParseHttpProxyData(source.c_str(), &hostAddress, &port, &username, &password, nullptr);
    free(hostAddress);
    free(username);
    free(password);
    return 0;
}

static int CheckCpuFlagSupported_target(const char* data, std::size_t size) noexcept
{
    auto cpuFlag = std::string(data, size);
    char* reason = nullptr;
    CheckCpuFlagSupported(cpuFlag.c_str(), &reason, nullptr);
    free(reason);
    return 0;
}

static int CheckLoginUmask_target(const char* data, std::size_t size) noexcept
{
    auto desired = std::string(data, size);
    char* reason = nullptr;
    CheckLoginUmask(desired.c_str(), &reason, nullptr);
    free(reason);
    return 0;
}

static int IsCurrentOs_target(const char* data, std::size_t size) noexcept
{
    auto name = std::string(data, size);
    IsCurrentOs(name.c_str(), nullptr);
    return 0;
}

static int RemovePrefixBlanks_target(const char* data, std::size_t size) noexcept
{
    auto name = std::string(data, size);
    RemovePrefixBlanks(&name[0]);
    return 0;
}

static int RemovePrefixUpTo_target(const char* data, std::size_t size) noexcept
{
    auto marker = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (marker.empty())
    {
        return c_skip_input;
    }

    auto name = std::string(data, size);
    RemovePrefixUpTo(&name[0], marker.at(0));
    return 0;
}

static int RemovePrefixUpToString_target(const char* data, std::size_t size) noexcept
{
    auto marker = g_context.ExtractVariant(data, size);
    if (marker.empty())
    {
        return c_skip_input;
    }

    auto name = std::string(data, size);
    RemovePrefixUpToString(&name[0], marker.c_str());
    return 0;
}

static int RemoveTrailingBlanks_target(const char* data, std::size_t size) noexcept
{
    auto name = std::string(data, size);
    RemoveTrailingBlanks(&name[0]);
    return 0;
}

static int TruncateAtFirst_target(const char* data, std::size_t size) noexcept
{
    auto marker = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (marker.empty())
    {
        return c_skip_input;
    }

    auto name = std::string(data, size);
    TruncateAtFirst(&name[0], marker.at(0));
    return 0;
}

static int UrlEncode_target(const char* data, std::size_t size) noexcept
{
    auto name = std::string(data, size);
    free(UrlEncode(&name[0]));
    return 0;
}

static int UrlDecode_target(const char* data, std::size_t size) noexcept
{
    auto name = std::string(data, size);
    free(UrlDecode(&name[0]));
    return 0;
}

static int IsDaemonActive_target(const char* data, std::size_t size) noexcept
{
    auto name = std::string(data, size);
    IsDaemonActive(name.c_str(), nullptr);
    return 0;
}

static int RepairBrokenEolCharactersIfAny_target(const char* data, std::size_t size) noexcept
{
    auto name = std::string(data, size);
    free(RepairBrokenEolCharactersIfAny(name.c_str()));
    return 0;
}

static int RemoveEscapeSequencesFromFile_target(const char* data, std::size_t size) noexcept
{
    auto escapes = g_context.ExtractVariant(data, size);
    if (escapes.empty())
    {
        return c_skip_input;
    }

    auto replacement = g_context.ExtractVariant(data, size, size_range{ 1, 1 });
    if (replacement.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.MakeTemporaryFile(data, size);
    RemoveEscapeSequencesFromFile(filename.c_str(), escapes.c_str(), escapes.size(), replacement.at(0), nullptr);
    g_context.Remove(filename);
    return 0;
}

static int IsCommandLoggingEnabledInJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    IsCommandLoggingEnabledInJsonConfig(json.c_str());
    return 0;
}

static int IsFullLoggingEnabledInJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    IsFullLoggingEnabledInJsonConfig(json.c_str());
    return 0;
}

static int IsIotHubManagementEnabledInJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    IsIotHubManagementEnabledInJsonConfig(json.c_str());
    return 0;
}

static int GetReportingIntervalFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    GetReportingIntervalFromJsonConfig(json.c_str(), nullptr);
    return 0;
}

static int GetModelVersionFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    GetModelVersionFromJsonConfig(json.c_str(), nullptr);
    return 0;
}

static int GetLocalManagementFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    GetLocalManagementFromJsonConfig(json.c_str(), nullptr);
    return 0;
}

static int GetIotHubProtocolFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    GetIotHubProtocolFromJsonConfig(json.c_str(), nullptr);
    return 0;
}

static int LoadReportedFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    REPORTED_PROPERTY* reported = nullptr;
    LoadReportedFromJsonConfig(json.c_str(), &reported, nullptr);
    free(reported);
    return 0;
}

static int GetGitManagementFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    GetGitManagementFromJsonConfig(json.c_str(), nullptr);
    return 0;
}

static int GetGitRepositoryUrlFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    GetGitRepositoryUrlFromJsonConfig(json.c_str(), nullptr);
    return 0;
}

static int GetGitBranchFromJsonConfig_target(const char* data, std::size_t size) noexcept
{
    auto json = std::string(data, size);
    GetGitBranchFromJsonConfig(json.c_str(), nullptr);
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
    { "SavePayloadToFile.", SavePayloadToFile_target },
    { "AppendPayloadToFile.", AppendPayloadToFile_target },
    { "SecureSaveToFile.", SecureSaveToFile_target },
    { "AppendToFile.", AppendToFile_target },
    { "ReplaceMarkedLinesInFile.", ReplaceMarkedLinesInFile_target },
    { "CheckFileSystemMountingOption.", CheckFileSystemMountingOption_target },
    { "CharacterFoundInFile.", CharacterFoundInFile_target },
    { "CheckNoLegacyPlusEntriesInFile.", CheckNoLegacyPlusEntriesInFile_target },
    { "FindTextInFile.", FindTextInFile_target },
    { "CheckTextIsFoundInFile.", CheckTextIsFoundInFile_target },
    { "CheckMarkedTextNotFoundInFile.", CheckMarkedTextNotFoundInFile_target },
    { "CheckTextNotFoundInEnvironmentVariable.", CheckTextNotFoundInEnvironmentVariable_target },
    { "CheckSmallFileContainsText.", CheckSmallFileContainsText_target },
    { "CheckLineNotFoundOrCommentedOut.", CheckLineNotFoundOrCommentedOut_target },
    { "GetStringOptionFromBuffer.", GetStringOptionFromBuffer_target },
    { "GetIntegerOptionFromBuffer.", GetIntegerOptionFromBuffer_target },
    { "CheckLockoutForFailedPasswordAttempts.", CheckLockoutForFailedPasswordAttempts_target },
    { "CheckPasswordCreationRequirements.", CheckPasswordCreationRequirements_target },
    { "GetStringOptionFromFile.", GetStringOptionFromFile_target },
    { "GetIntegerOptionFromFile.", GetIntegerOptionFromFile_target },
    { "CheckIntegerOptionFromFileEqualWithAny.", CheckIntegerOptionFromFileEqualWithAny_target },
    { "CheckIntegerOptionFromFileLessOrEqualWith.", CheckIntegerOptionFromFileLessOrEqualWith_target },
    { "DuplicateString.", DuplicateString_target },
    { "ConcatenateStrings.", ConcatenateStrings_target },
    { "DuplicateStringToLowercase.", DuplicateStringToLowercase_target },
    { "ConvertStringToIntegers.", ConvertStringToIntegers_target },
    { "RemoveCharacterFromString.", RemoveCharacterFromString_target },
    { "ReplaceEscapeSequencesInString.", ReplaceEscapeSequencesInString_target },
    { "HashString.", HashString_target },
    { "ParseHttpProxyData.", ParseHttpProxyData_target },
    { "CheckCpuFlagSupported.", CheckCpuFlagSupported_target },
    { "CheckLoginUmask.", CheckLoginUmask_target },
    { "IsCurrentOs.", IsCurrentOs_target },
    { "RemovePrefixBlanks.", RemovePrefixBlanks_target },
    { "RemovePrefixUpTo.", RemovePrefixUpTo_target },
    { "RemovePrefixUpToString.", RemovePrefixUpToString_target },
    { "RemoveTrailingBlanks.", RemoveTrailingBlanks_target },
    { "TruncateAtFirst.", TruncateAtFirst_target },
    { "UrlEncode.", UrlEncode_target },
    { "UrlDecode.", UrlDecode_target },
    { "IsDaemonActive.", IsDaemonActive_target },
    { "RepairBrokenEolCharactersIfAny.", RepairBrokenEolCharactersIfAny_target },
    { "RemoveEscapeSequencesFromFile.", RemoveEscapeSequencesFromFile_target },
    { "IsCommandLoggingEnabledInJsonConfig.", IsCommandLoggingEnabledInJsonConfig_target },
    { "IsFullLoggingEnabledInJsonConfig.", IsFullLoggingEnabledInJsonConfig_target },
    { "IsIotHubManagementEnabledInJsonConfig.", IsIotHubManagementEnabledInJsonConfig_target },
    { "GetReportingIntervalFromJsonConfig.", GetReportingIntervalFromJsonConfig_target },
    { "GetModelVersionFromJsonConfig.", GetModelVersionFromJsonConfig_target },
    { "GetLocalManagementFromJsonConfig.", GetLocalManagementFromJsonConfig_target },
    { "GetIotHubProtocolFromJsonConfig.", GetIotHubProtocolFromJsonConfig_target },
    { "LoadReportedFromJsonConfig.", LoadReportedFromJsonConfig_target },
    { "GetGitManagementFromJsonConfig.", GetGitManagementFromJsonConfig_target },
    { "GetGitRepositoryUrlFromJsonConfig.", GetGitRepositoryUrlFromJsonConfig_target },
    { "GetGitBranchFromJsonConfig.", GetGitBranchFromJsonConfig_target },
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
