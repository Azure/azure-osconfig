// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CommonContext.h"

#include "CommonUtils.h"

#include <cerrno>
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#include <set>

namespace ComplianceEngine
{
constexpr char CommonContext::sFsCachePath[];
constexpr char CommonContext::sLockPath[];

CommonContext::~CommonContext() = default;

Result<std::string> CommonContext::ExecuteCommand(const std::string& cmd) const
{
    char* output = nullptr;
    int err = ::ExecuteCommand(NULL, cmd.c_str(), false, false, 0, 0, &output, NULL, mLog);
    if (err != 0 || output == nullptr)
    {
        std::string outStr = output == NULL ? "Failed to execute command" : output;
        free(output);
        return Error(outStr, err);
    }
    std::string result(output);
    free(output);
    return result;
}

Result<std::string> CommonContext::GetFileContents(const std::string& filePath) const
{
    struct stat st;
    if (stat(filePath.c_str(), &st) != 0)
    {
        int status = errno;
        if (ENOENT == status)
        {
            return Error("File not found: " + filePath, status);
        }

        return Error("Failed to stat file: " + std::string(strerror(status)), status);
    }

    char* output = LoadStringFromFile(filePath.c_str(), false, mLog);
    if (output == nullptr)
    {
        return Error("Failed to load file contents");
    }
    std::string result(output);
    free(output);
    return result;
}

std::string CommonContext::GetSpecialFilePath(const std::string& path) const
{
    return path;
}

Result<std::vector<InterfaceInfo>> CommonContext::GetNetworkInterfaces() const
{
    struct ifaddrs* ifap = nullptr;
    if (getifaddrs(&ifap) != 0)
    {
        int status = errno;
        OsConfigLogError(mLog, "getifaddrs() failed: %s (%d)", strerror(status), status);
        return Error("Failed to enumerate network interfaces: " + std::string(strerror(status)), status);
    }

    // getifaddrs() reports one entry per address, so an interface with several addresses
    // appears multiple times; ifa_flags is identical across those entries. Dedup by name
    // (first occurrence wins) so each interface is assessed once. A successful call with no
    // interfaces yields an empty list, which the caller distinguishes from the error above.
    std::vector<InterfaceInfo> interfaces;
    std::set<std::string> seen;
    for (struct ifaddrs* ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_name == nullptr)
        {
            continue;
        }
        std::string name(ifa->ifa_name);
        if (!seen.insert(name).second)
        {
            continue;
        }
        InterfaceInfo info;
        info.name = std::move(name);
        info.flags = ifa->ifa_flags;
        interfaces.push_back(std::move(info));
    }

    freeifaddrs(ifap);
    return interfaces;
}

} // namespace ComplianceEngine
