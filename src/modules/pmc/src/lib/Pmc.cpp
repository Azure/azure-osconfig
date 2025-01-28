// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <regex>
#include <string>
#include <CommonUtils.h>
#include <Mmi.h>
#include <Pmc.h>

static const std::string g_requiredTools[] = {"apt-get", "apt-cache", "dpkg-query", "curl", "gpg"};

constexpr const char* g_commandCheckToolPresence = "command -v $value";
constexpr const char* g_commandGetInstalledPackages = "dpkg-query --showformat='${Package} (=${Version})\n' --show";
constexpr const char* g_commandGetSourcesContent = "find $value -type f -name '*.list' -exec cat {} \\;";

Pmc::Pmc(unsigned int maxPayloadSizeBytes)
    : PmcBase(maxPayloadSizeBytes)
{
}

int Pmc::RunCommand(const char* command, std::string* textResult, bool isLongRunning)
{
    char* buffer = nullptr;
    const bool replaceEol = true;
    const bool forJson = false;
    int status = ExecuteCommand(nullptr, command, replaceEol, forJson, 0, isLongRunning ? TIMEOUT_LONG_RUNNING : 0, &buffer, nullptr, PmcLog::Get());

    if (status == PMC_0K)
    {
        if (buffer && textResult)
        {
            *textResult = buffer;
        }
    }

    FREE_MEMORY(buffer);

    return status;
}

std::string Pmc::GetPackagesFingerprint()
{
    char* hash = HashCommand(g_commandGetInstalledPackages, PmcLog::Get());
    std::string hashString = hash ? hash : "(failed)";
    FREE_MEMORY(hash)
    return hashString;
}

std::string Pmc::GetSourcesFingerprint(const char* sourcesDirectory)
{
    char* hash = nullptr;
    if (FileExists(sourcesDirectory))
    {
        std::string command = std::regex_replace(g_commandGetSourcesContent, std::regex("\\$value"), sourcesDirectory);
        hash = HashCommand(command.c_str(), PmcLog::Get());
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogError(PmcLog::Get(), "Unable to get the fingerprint of source files. Directory %s does not exist", sourcesDirectory);
    }

    std::string hashString = hash ? hash : "(failed)";
    FREE_MEMORY(hash)
    return hashString;
}

bool Pmc::CanRunOnThisPlatform()
{
    for (auto& tool : g_requiredTools)
    {
        std::string command = std::regex_replace(g_commandCheckToolPresence, std::regex("\\$value"), tool);
        if (RunCommand(command.c_str(), nullptr) != PMC_0K)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(PmcLog::Get(), "Cannot run on this platform, could not find required tool %s", tool.c_str());
            }

            return false;
        }
    }

    return true;
}
