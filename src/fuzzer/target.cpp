#include "Mmi.h"
#include "SecurityBaseline.h"
#include "CommonUtils.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>
#include <map>
#include <stdexcept>
#include <limits>

#include <iostream>

/**
 * @brief Tells libfuzzer to skip the input when it doesn't contain a valid target
 */
static const int c_skip_input = -1;

/**
 * @brief Tells libfuzzer the input was valid and may be used to create a new corpus input
 */
static const int c_valid_input = 0;

struct size_range
{
    std::size_t min = 1;
    std::size_t max = std::numeric_limits<std::size_t>::max();

    size_range() = default;
    size_range(std::size_t min, std::size_t max) : min(min), max(max) {}
};

/**
 * @brief A class to keep a single static initialization of the SecurityBaseline library
 */
struct Context
{
    MMI_HANDLE handle;
    std::string tempdir;
public:
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

    std::string nextTempfileName() const noexcept
    {
        static int counter = 0;
        return tempdir + "." + std::to_string(counter++);
    }

    std::string makeTempfile(const char* data, std::size_t size) const noexcept(false)
    {
        auto path = nextTempfileName();
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

    std::string extractVariant(const char*& data, std::size_t& size, size_range range = size_range{}) const noexcept
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

    void remove(const std::string& path) const noexcept
    {
        ::remove(path.c_str());
    }
};

static Context g_context;

static int LoadStringFromFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.makeTempfile(data, size);
    free(LoadStringFromFile(filename.c_str(), true, nullptr));
    g_context.remove(filename);
    return 0;
}

static int GetNumberOfLinesInFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.makeTempfile(data, size);
    GetNumberOfLinesInFile(filename.c_str());
    g_context.remove(filename);
    return 0;
}

static int SavePayloadToFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.nextTempfileName();
    SavePayloadToFile(filename.c_str(), data, size, nullptr);
    g_context.remove(filename);
    return 0;
}

static int AppendPayloadToFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.makeTempfile(nullptr, 0);
    AppendPayloadToFile(filename.c_str(), data, size, nullptr);
    g_context.remove(filename);
    return 0;
}

static int SecureSaveToFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.nextTempfileName();
    SecureSaveToFile(filename.c_str(), data, size, nullptr);
    g_context.remove(filename);
    return 0;
}

static int AppendToFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.makeTempfile(nullptr, 0);
    AppendToFile(filename.c_str(), data, size, nullptr);
    g_context.remove(filename);
    return 0;
}

static int ReplaceMarkedLinesInFile_target(const char* data, std::size_t size) noexcept
{
    auto marker = g_context.extractVariant(data, size);
    if (marker.empty())
    {
        return c_skip_input;
    }

    auto newline = g_context.extractVariant(data, size);
    if (newline.empty())
    {
        return c_skip_input;
    }

    auto comment = g_context.extractVariant(data, size, size_range{ 1, 1 });
    if (comment.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.makeTempfile(data, size);
    ReplaceMarkedLinesInFile(filename.c_str(), marker.c_str(), newline.c_str(), comment.at(0), true, nullptr);
    g_context.remove(filename);
    return 0;
}

static int CheckFileSystemMountingOption_target(const char* data, std::size_t size) noexcept
{
    auto mountDirectory = g_context.extractVariant(data, size);
    if (mountDirectory.empty())
    {
        return c_skip_input;
    }

    auto mountType = g_context.extractVariant(data, size);
    if (mountType.empty())
    {
        return c_skip_input;
    }

    auto desiredOption = g_context.extractVariant(data, size);
    if (desiredOption.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.makeTempfile(data, size);
    char* reason = nullptr;
    CheckFileSystemMountingOption(filename.c_str(), mountDirectory.c_str(), mountType.c_str(), desiredOption.c_str(), &reason, nullptr);
    g_context.remove(filename);
    free(reason);
    return 0;
}

static int CharacterFoundInFile_target(const char* data, std::size_t size) noexcept
{
    auto what = g_context.extractVariant(data, size, size_range{ 1, 1 });
    if (what.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.makeTempfile(data, size);
    CharacterFoundInFile(filename.c_str(), what.at(0));
    g_context.remove(filename);
    return 0;
}

static int CheckNoLegacyPlusEntriesInFile_target(const char* data, std::size_t size) noexcept
{
    auto filename = g_context.makeTempfile(data, size);
    char* reason = nullptr;
    CheckNoLegacyPlusEntriesInFile(filename.c_str(), &reason, nullptr);
    g_context.remove(filename);
    free(reason);
    return 0;
}

static int FindTextInFile_target(const char* data, std::size_t size) noexcept
{
    auto text = g_context.extractVariant(data, size);
    if (text.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.makeTempfile(data, size);
    FindTextInFile(filename.c_str(), text.c_str(), nullptr);
    g_context.remove(filename);
    return 0;
}

static int CheckTextIsFoundInFile_target(const char* data, std::size_t size) noexcept
{
    auto text = g_context.extractVariant(data, size);
    if (text.empty())
    {
        return c_skip_input;
    }

    auto filename = g_context.makeTempfile(data, size);
    char* reason = nullptr;
    CheckTextIsFoundInFile(filename.c_str(), text.c_str(), &reason, nullptr);
    g_context.remove(filename);
    free(reason);
    return 0;
}

/* Skipping CheckTextIsNotFoundInFile due to similarity */

static int SecurityBaselineMmiGet_target(const char* data, std::size_t size) noexcept
{
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto input = std::string(data, size);
    SecurityBaselineMmiGet(g_context.handle, "SecurityBaseline", input.c_str(), &payload, &payloadSizeBytes);
    SecurityBaselineMmiFree(payload);
    return 0;
}

static int SecurityBaselineMmiSet_target(const char* data, std::size_t size) noexcept
{
    const char* prefix = reinterpret_cast<const char*>(std::memchr(data, '.', size));
    if (prefix == nullptr)
    {
        /* Colon not found, skip the input */
        return c_skip_input;
    }

    /* Include the delimiter */
    prefix++;
    const auto prefix_size = prefix - data;
    size -= prefix_size;

    char* payload = reinterpret_cast<char*>(malloc(size));
    if(!payload)
    {
        return c_skip_input;
    }
    memcpy(payload, prefix, size);

    auto input = std::string(data, prefix_size-1);
    SecurityBaselineMmiSet(g_context.handle, "SecurityBaseline", input.c_str(), payload, size);
    SecurityBaselineMmiFree(payload);
    return 0;
}

/**
 * @brief List of supported fuzzing targets
 *
 * The key is taken from the input data and is used to determine which target to call.
 */
static const std::map<std::string, int (*)(const char*, std::size_t)> g_targets = {
    // { "SecurityBaselineMmiGet.", SecurityBaselineMmiGet_target },
    // { "SecurityBaselineMmiSet.", SecurityBaselineMmiSet_target },
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
};

/**
 * @brief libfuzzer entry point
 */
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto* input = reinterpret_cast<const char*>(data);
    const auto* prefix = reinterpret_cast<const char*>(std::memchr(input, '.', size));
    if (prefix == nullptr)
    {
        /* Colon not found, skip the input */
        return c_skip_input;
    }

    /* Include the delimiter */
    prefix++;
    const auto prefix_size = prefix - input;
    auto it = g_targets.find(std::string(input, prefix_size));
    if(it == g_targets.end())
    {
        /* Target mismatch, skip the input */
        return c_skip_input;
    }

    return it->second(prefix, size - prefix_size);
}