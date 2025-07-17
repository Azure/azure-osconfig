// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_BENCHMARK_INFO_H
#define COMPLIANCEENGINE_BENCHMARK_INFO_H

#include <DistributionInfo.h>
#include <Result.h>
#include <string>

namespace ComplianceEngine
{
// Defines the type of the benchmark, e.g., CIS
enum class BenchmarkType
{
    CIS,
};

// Defines CIS benchmark information
// Note: For now only CIS is supported, but when new benchmark types are added,
// intention is to make this struct generic and use a variant type,
// which needs to be implemented for this purpose.
struct CISBenchmarkInfo
{
    // Defines the Linux distribution, e.g., Ubuntu, CentOS
    LinuxDistribution distribution;

    // Defines the version of the Linux distribution, e.g., 20.04, 8
    std::string version;

    // Defines the version of the benchmark, e.g., v1.0.0
    std::string benchmarkVersion;

    // Defines the benchmark section, e.g. 1.1.1
    std::string section;

    // Parses payload key and converts it to the benchmark information
    static Result<CISBenchmarkInfo> Parse(const std::string& payloadKey);

    // Match the benchmark information against detected distribution information.
    // Returns true in case of a match.
    bool Match(const DistributionInfo& distributionInfo) const;
};

} // namespace ComplianceEngine

namespace std
{
std::string to_string(ComplianceEngine::BenchmarkType benchmarkType);           // NOLINT(*-identifier-naming)
std::string to_string(const ComplianceEngine::CISBenchmarkInfo& benchmarkInfo); // NOLINT(*-identifier-naming)
} // namespace std

#endif // COMPLIANCEENGINE_BENCHMARK_INFO_H
