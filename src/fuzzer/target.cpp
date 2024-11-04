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

#include <iostream>

/**
 * @brief Tells libfuzzer to skip the input when it doesn't contain a valid target
 */
static const int c_skip_input = -1;

/**
 * @brief Tells libfuzzer the input was valid and may be used to create a new corpus input
 */
static const int c_valid_input = 0;

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