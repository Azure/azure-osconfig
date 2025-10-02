// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_DISTRIBUTION_INFO_H
#define COMPLIANCEENGINE_DISTRIBUTION_INFO_H

#include <Result.h>
#include <string>

namespace ComplianceEngine
{
// Defines operating system type, e.g., Linux, Windows
enum class OSType
{
    Linux,
};

// Defines the Linux distribution, e.g., Ubuntu, CentOS
enum class LinuxDistribution
{
    Ubuntu,
    Centos,
    RHEL,
    SUSE,
    OracleLinux,
    Mariner,
    Debian,
    AzureLinux,
    AmazonLinux,
    AlmaLinux,
    RockyLinux,
};

// Defines the system architecture, e.g., x86_64, arm64
enum class Architecture
{
    x86_64,
};

struct DistributionInfo
{
    static constexpr const char* cDefaultEtcOsReleasePath = "/etc/os-release";
    static constexpr const char* cDefaultOverrideFilePath = "/etc/osconfig/system_id.override";

    OSType osType = OSType::Linux;

    // Defines the system architecture, e.g., x86_64, arm64
    Architecture architecture = Architecture::x86_64;

    // Defines the Linux distribution, e.g., Ubuntu, CentOS
    LinuxDistribution distribution = LinuxDistribution::Ubuntu;

    // Defines the version of the Linux distribution, e.g., 20.04, 8, 15*
    // The value is a globbing pattern and fnmatch is used for comparison.
    std::string version;

    static Result<DistributionInfo> ParseEtcOsRelease(const std::string& etcOsReleasePath);
    static Result<DistributionInfo> ParseOverrideFile(const std::string& overrideFilePath);
    static Result<LinuxDistribution> ParseLinuxDistribution(const std::string& distributionStr);
};

} // namespace ComplianceEngine

namespace std
{
std::string to_string(ComplianceEngine::LinuxDistribution distribution) noexcept(false);           // NOLINT(*-identifier-naming)
std::string to_string(ComplianceEngine::OSType osType) noexcept(false);                            // NOLINT(*-identifier-naming)
std::string to_string(ComplianceEngine::Architecture architecture) noexcept(false);                // NOLINT(*-identifier-naming)
std::string to_string(const ComplianceEngine::DistributionInfo& distributionInfo) noexcept(false); // NOLINT(*-identifier-naming)
} // namespace std
#endif // COMPLIANCEENGINE_DISTRIBUTION_INFO_H
