// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <BenchmarkInfo.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Result.h>
#include <RevertMap.h>
#include <fstream>
#include <iostream>
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
using FileIterator = std::istream_iterator<char>;

Result<BenchmarkType> ParseBenchmarkType(const string& benchmarkType)
{
    auto it = sBenchmarkTypeMap.find(benchmarkType);
    if (it != sBenchmarkTypeMap.end())
    {
        return it->second;
    }
    return Error("Unsupported benchmark type: '" + benchmarkType + "'", EINVAL);
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
    return distributionInfo.distribution == distribution && distributionInfo.version == version;
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
