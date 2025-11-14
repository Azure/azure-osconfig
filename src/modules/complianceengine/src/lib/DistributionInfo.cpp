// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <DistributionInfo.h>
#include <InputStream.h>
#include <Logging.h>
#include <Result.h>
#include <RevertMap.h>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <sys/utsname.h>

namespace ComplianceEngine
{
using std::map;
using std::string;

const map<string, OSType>& GetOSTypeMap()
{
    static const map<string, OSType> sOSTypeMap = {
        {"Linux", OSType::Linux},
        // Add more OS types as needed
    };
    return sOSTypeMap;
}

const map<string, Architecture>& GetArchitectureMap()
{
    static const map<string, Architecture> sArchitectureMap = {
        {"x86_64", Architecture::x86_64},
        // Add more architectures as needed
    };
    return sArchitectureMap;
}

const map<string, LinuxDistribution>& GetDistributionMap()
{
    static const map<string, LinuxDistribution> sDistributionMap = {
        {"ubuntu", LinuxDistribution::Ubuntu}, {"centos", LinuxDistribution::Centos}, {"rhel", LinuxDistribution::RHEL},
        {"sles", LinuxDistribution::SUSE}, {"ol", LinuxDistribution::OracleLinux}, {"mariner", LinuxDistribution::Mariner},
        {"debian", LinuxDistribution::Debian}, {"azurelinux", LinuxDistribution::AzureLinux}, {"amzn", LinuxDistribution::AmazonLinux},
        {"almalinux", LinuxDistribution::AlmaLinux}, {"rocky", LinuxDistribution::RockyLinux},
        // Add more distributions as needed
    };
    return sDistributionMap;
}

namespace
{
using FileIterator = std::istream_iterator<char>;

Result<string> ParseKey(FileIterator& input)
{
    string result;
    bool hasSpaces = false;
    const auto end = FileIterator();
    for (; input != end; ++input)
    {
        const auto value = *input;
        if (value == '=')
        {
            if (result.empty())
            {
                return Error("Unexpected '=' at the start of a key", EINVAL);
            }

            ++input; // Move past '='
            return result;
        }

        if (value == '#')
        {
            if (!result.empty())
            {
                return Error("Unexpected comment character '#' in a key", EINVAL);
            }
            for (++input; input != FileIterator() && *input != '\n'; ++input)
            {
                // Skip the rest of the line
            }
            continue;
        }

        if (isspace(value))
        {
            if (!result.empty())
            {
                hasSpaces = true;
            }

            continue; // Skip spaces in key, unless they are in the middle of a key
        }

        if (hasSpaces)
        {
            // Space in the middle of a key is not allowed
            return Error("Unexpected space in a key", EINVAL);
        }

        result += value;
    }

    if (result.empty())
    {
        return result;
    }
    return Error("Unexpected end of input while parsing a key", EINVAL);
}

Result<string> ParseValue(FileIterator& input)
{
    string result;
    bool quoted = false;
    const auto end = FileIterator();
    for (; input != end; ++input)
    {
        const auto value = *input;
        if (value == '"')
        {
            if (quoted)
            {
                // End of quoted value
                ++input; // Move past '"'
                return result;
            }
            else
            {
                if (!result.empty())
                {
                    // Quoted value cannot start within a value
                    return Error("Unexpected quote character past the start of value", EINVAL);
                }

                // Start of quoted value
                quoted = true;
                continue;
            }
        }

        if (value == '#' && !quoted)
        {
            for (++input; input != FileIterator() && *input != '\n'; ++input)
            {
                // Skip the rest of the line
            }
            ++input;       // Move past the newline character
            return result; // Return the value parsed so far
        }

        if (isspace(value))
        {
            if (!quoted)
            {
                ++input;
                return result; // Return the value parsed so far
            }

            continue; // Skip spaces in unquoted value
        }

        result += value;
    }

    if (quoted)
    {
        return Error("Unexpected end of input while parsing a quoted value", EINVAL);
    }
    return result;
}

// Shared function to parse the /etc/os-release file and the override file
Result<map<string, string>> ParseDistributionInfoFile(const string& etcOsReleasePath)
{
    map<string, string> result;
    std::ifstream file(etcOsReleasePath);
    if (!file.is_open())
    {
        return Error("Failed to open " + etcOsReleasePath, ENOENT);
    }
    file >> std::noskipws; // Don't skip whitespace characters

    auto begin = FileIterator(file);
    const auto end = FileIterator();
    while (begin != end)
    {
        auto key = ParseKey(begin);
        if (!key.HasValue())
        {
            return key.Error();
        }

        if (key->empty())
        {
            break;
        }

        auto value = ParseValue(begin);
        if (!value.HasValue())
        {
            return value.Error();
        }

        result[std::move(key).Value()] = std::move(value).Value();
    }

    return result;
}

Result<OSType> ParseOSType(const string& osTypeStr)
{
    const auto& osTypeMap = GetOSTypeMap();
    auto it = osTypeMap.find(osTypeStr);
    if (it != osTypeMap.end())
    {
        return it->second;
    }

    return Error("Unsupported OS type: " + osTypeStr, EINVAL);
}

Result<Architecture> ParseArchitecture(const string& architectureStr)
{
    const auto& architectureMap = GetArchitectureMap();
    auto it = architectureMap.find(architectureStr);
    if (it != architectureMap.end())
    {
        return it->second;
    }

    return Error("Unsupported architecture: " + architectureStr, EINVAL);
}
} // namespace

Result<LinuxDistribution> DistributionInfo::ParseLinuxDistribution(const string& distributionStr)
{
    const auto& distributionMap = GetDistributionMap();
    auto it = distributionMap.find(distributionStr);
    if (it != distributionMap.end())
    {
        return it->second;
    }

    // Add more distributions as needed
    return Error("Unsupported Linux distribution: " + distributionStr, EINVAL);
}

Result<DistributionInfo> DistributionInfo::ParseEtcOsRelease(const string& etcOsReleasePath)
{
    const auto osReleaseInfo = ParseDistributionInfoFile(etcOsReleasePath);
    if (!osReleaseInfo.HasValue())
    {
        return osReleaseInfo.Error();
    }

    auto it = osReleaseInfo.Value().find("ID");
    if (it == osReleaseInfo.Value().end())
    {
        return Error(etcOsReleasePath + " does not contain 'ID' field", EINVAL);
    }

    const auto distribution = ParseLinuxDistribution(it->second);
    if (!distribution.HasValue())
    {
        return distribution.Error();
    }

    DistributionInfo result;
    result.distribution = distribution.Value();

    it = osReleaseInfo.Value().find("VERSION_ID");
    if (it == osReleaseInfo.Value().end())
    {
        return Error(etcOsReleasePath + " does not contain 'VERSION_ID' field", EINVAL);
    }
    result.version = std::move(it->second);

    utsname unameData;
    if (0 != uname(&unameData))
    {
        int status = errno;
        return Error("Failed to get system information: " + std::string(strerror(status)), status);
    }

    const auto osType = ParseOSType(unameData.sysname);
    if (!osType.HasValue())
    {
        return osType.Error();
    }
    result.osType = osType.Value();

    const auto architecture = ParseArchitecture(unameData.machine);
    if (!architecture.HasValue())
    {
        return architecture.Error();
    }
    result.architecture = architecture.Value();

    return result;
}

Result<DistributionInfo> DistributionInfo::ParseOverrideFile(const string& overrideFilePath)
{
    const auto osReleaseInfo = ParseDistributionInfoFile(overrideFilePath);
    if (!osReleaseInfo.HasValue())
    {
        return osReleaseInfo.Error();
    }

    DistributionInfo result;
    // Defines the OS type, e.g., Linux, Windows
    auto it = osReleaseInfo.Value().find("OS");
    if (it == osReleaseInfo.Value().end())
    {
        return Error(overrideFilePath + " file does not contain 'OS' field", EINVAL);
    }
    const auto osType = ParseOSType(it->second);
    if (!osType.HasValue())
    {
        return osType.Error();
    }
    result.osType = osType.Value();

    // Defines the system architecture, e.g., x86_64, arm64
    it = osReleaseInfo.Value().find("ARCH");
    if (it == osReleaseInfo.Value().end())
    {
        return Error(overrideFilePath + " file does not contain 'ARCH' field", EINVAL);
    }
    const auto architecture = ParseArchitecture(it->second);
    if (!architecture.HasValue())
    {
        return architecture.Error();
    }
    result.architecture = architecture.Value();

    // Defines the Linux distribution
    it = osReleaseInfo.Value().find("DISTRO");
    if (it == osReleaseInfo.Value().end())
    {
        return Error(overrideFilePath + " file does not contain 'DISTRO' field", EINVAL);
    }
    const auto distribution = ParseLinuxDistribution(it->second);
    if (!distribution.HasValue())
    {
        return distribution.Error();
    }
    result.distribution = distribution.Value();

    // Defines the version of the Linux distribution, e.g., 20.04, 8
    it = osReleaseInfo.Value().find("VERSION");
    if (it == osReleaseInfo.Value().end())
    {
        return Error(overrideFilePath + " file does not contain 'VERSION' field", EINVAL);
    }
    result.version = std::move(it->second);

    return result;
}

} // namespace ComplianceEngine

namespace std
{
using ComplianceEngine::Architecture;
using ComplianceEngine::LinuxDistribution;
using ComplianceEngine::OSType;
using ComplianceEngine::RevertMap;

string to_string(LinuxDistribution distribution) noexcept(false)
{
    static const auto distributionMap = RevertMap(ComplianceEngine::GetDistributionMap());
    auto it = distributionMap.find(distribution);
    if (it != distributionMap.end())
    {
        return it->second;
    }

    throw invalid_argument("Unsupported Linux distribution");
}

string to_string(OSType osType) noexcept(false)
{
    static const auto osTypeMap = RevertMap(ComplianceEngine::GetOSTypeMap());
    auto it = osTypeMap.find(osType);
    if (it != osTypeMap.end())
    {
        return it->second;
    }

    throw invalid_argument("Unsupported OS type");
}

string to_string(Architecture architecture) noexcept(false)
{
    static const auto architectureMap = RevertMap(ComplianceEngine::GetArchitectureMap());
    auto it = architectureMap.find(architecture);
    if (it != architectureMap.end())
    {
        return it->second;
    }

    throw invalid_argument("Unsupported architecture");
}

string to_string(const ComplianceEngine::DistributionInfo& distributionInfo) noexcept(false)
{
    ostringstream oss;
    oss << "OS=\"" << to_string(distributionInfo.osType) << "\"";
    oss << " ARCH=\"" << to_string(distributionInfo.architecture) << "\"";
    oss << " DISTRO=\"" << to_string(distributionInfo.distribution) << "\"";
    oss << " VERSION=\"" << distributionInfo.version << "\"";
    return oss.str();
}
} // namespace std
