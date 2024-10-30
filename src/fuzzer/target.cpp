#include "Mmi.h"
#include "SecurityBaseline.h"
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
public:
    Context() noexcept(false)
    {
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
        SecurityBaselineMmiClose(handle);
        SecurityBaselineShutdown();
    }
};

static Context g_context;

static int SecurityBaselineMmiGet_target(const char* data, std::size_t size) noexcept {
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto input = std::string(data, size);
    SecurityBaselineMmiGet(g_context.handle, "SecurityBaseline", input.c_str(), &payload, &payloadSizeBytes);
    SecurityBaselineMmiFree(payload);
    return 0;
}

static int SecurityBaselineMmiSet_target(const char* data, std::size_t size) noexcept {
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
    { "SecurityBaselineMmiGet.", SecurityBaselineMmiGet_target },
    { "SecurityBaselineMmiSet.", SecurityBaselineMmiSet_target }
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