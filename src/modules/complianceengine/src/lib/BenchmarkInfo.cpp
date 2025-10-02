// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <BenchmarkInfo.h>
#include <Optional.h>
#include <Result.h>
#include <RevertMap.h>
#include <cstring>
#include <fnmatch.h>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>

namespace ComplianceEngine
{
using std::map;
using std::string;

static const map<string, BenchmarkType> sBenchmarkTypeMap = {
    {"cis", BenchmarkType::CIS},
    // Add more benchmark types as needed
};

namespace
{
Result<BenchmarkType> ParseBenchmarkType(const string& benchmarkType)
{
    auto it = sBenchmarkTypeMap.find(benchmarkType);
    if (it != sBenchmarkTypeMap.end())
    {
        return it->second;
    }
    return Error("Unsupported benchmark type: '" + benchmarkType + "'", EINVAL);
}

Optional<Error> ValidateGlobbing(const string& benchmarkVersion)
{
    for (auto c : benchmarkVersion)
    {
        if (strchr("[]{}", c) != nullptr)
        {
            return Error("Invalid benchmark version: " + benchmarkVersion + ". Globbing characters [ ] { } are not allowed.", EINVAL);
        }
    }

    return Optional<Error>();
}
} // namespace

Result<CISBenchmarkInfo> CISBenchmarkInfo::Parse(const string& payloadKey)
{
    CISBenchmarkInfo result;
    string token;
    std::stringstream ss(payloadKey);
    // skip the first token which is expected to be empty due to leading '/'
    if (!std::getline(ss, token, '/') || !token.empty())
    {
        return Error("Invalid payload key format: must start with '/'", EINVAL);
    }

    // Get the benchmark type
    if (!std::getline(ss, token, '/'))
    {
        return Error("Invalid payload key format: missing benchmark type", EINVAL);
    }
    const auto benchmarkType = ParseBenchmarkType(token);
    if (!benchmarkType.HasValue())
    {
        return benchmarkType.Error();
    }

    if (!std::getline(ss, token, '/'))
    {
        return Error("Invalid CIS benchmark payload key format: missing distribution", EINVAL);
    }
    const auto distribution = DistributionInfo::ParseLinuxDistribution(token);
    if (!distribution.HasValue())
    {
        return distribution.Error();
    }
    result.distribution = distribution.Value();

    if (!std::getline(ss, result.version, '/') || result.version.empty())
    {
        return Error("Invalid CIS benchmark payload key format: missing distribution version", EINVAL);
    }
    auto error = ValidateGlobbing(result.version);
    if (error.HasValue())
    {
        return error.Value();
    }

    if (!std::getline(ss, result.benchmarkVersion, '/') || result.benchmarkVersion.empty())
    {
        return Error("Invalid CIS benchmark payload key format: missing benchmark version", EINVAL);
    }

    if (!std::getline(ss, result.section) || result.section.empty())
    {
        return Error("Invalid CIS benchmark payload key format: missing benchmark section", EINVAL);
    }
    return result;
}

bool CISBenchmarkInfo::Match(const DistributionInfo& distributionInfo) const
{
    if (distributionInfo.distribution != distribution)
    {
        return false;
    }

    const int status = fnmatch(version.c_str(), distributionInfo.version.c_str(), 0);
    if (0 != status)
    {
        return false;
    }

    return true;
}
} // namespace ComplianceEngine

namespace std
{
using ComplianceEngine::RevertMap;

string to_string(ComplianceEngine::BenchmarkType benchmarkType)
{
    static const auto benchmarkTypeMap = RevertMap(ComplianceEngine::sBenchmarkTypeMap);
    auto it = benchmarkTypeMap.find(benchmarkType);
    if (it != benchmarkTypeMap.end())
    {
        return it->second;
    }
    throw invalid_argument("Unsupported benchmark type");
}

string to_string(const ComplianceEngine::CISBenchmarkInfo& benchmarkInfo)
{
    ostringstream oss;
    oss << "/" << to_string(ComplianceEngine::BenchmarkType::CIS) << "/" << to_string(benchmarkInfo.distribution) << "/" << benchmarkInfo.version << "/"
        << benchmarkInfo.benchmarkVersion << "/" << benchmarkInfo.section;
    return oss.str();
}
} // namespace std
