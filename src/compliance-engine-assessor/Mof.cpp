#include <Mof.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <istream>
#include <sstream>

namespace mof
{
using ComplianceEngine::Error;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using std::array;
using std::istream;
using std::map;
using std::string;

namespace
{
string getValue(const std::string& line)
{
    auto start = line.find('"');
    if (start == std::string::npos)
        return std::string();
    auto end = line.find('"', start + 1);
    if (end == std::string::npos)
        return std::string();
    return line.substr(start + 1, end - (start + 1));
};

ComplianceEngine::Result<MofEntry::BenchmarkType> ParseBenchmarkType(const std::string& key)
{
    static const map<string, MofEntry::BenchmarkType> benchmarkTypes = {
        {"cis", MofEntry::BenchmarkType::CIS},
    };

    auto it = benchmarkTypes.find(key);
    if (it != benchmarkTypes.end())
    {
        return it->second;
    }

    return Error("Unsupported benchmark type");
}

ComplianceEngine::Result<MofEntry::DistributionType> ParseDistributionType(const std::string& key)
{
    static const map<string, MofEntry::DistributionType> benchmarkTypes = {
        {"ubuntu", MofEntry::DistributionType::Ubuntu},
    };

    auto it = benchmarkTypes.find(key);
    if (it != benchmarkTypes.end())
    {
        return it->second;
    }

    return Error("Unsupported distribution type");
}

ComplianceEngine::Result<array<string, 5>> SplitPayloadKey(const string& input)
{
    array<string, 5> tokens;
    string token;
    std::istringstream tokenStream(input);
    std::size_t index = 0;
    while (getline(tokenStream, token, '/'))
    {
        if (token.empty() && index == 0)
        {
            continue; // Skip leading empty segment
        }

        tokens[index++] = std::move(token);
        if (index == 4)
        {
            // The remainder of the string is considered as a single token
            tokens[4] = tokenStream.str().substr(tokenStream.tellg());
            std::replace(tokens[4].begin(), tokens[4].end(), '/', '.');
            return tokens;
        }
    }

    return Error("Invalid payload key format: must contain at least 5 segments");
}
} // anonymous namespace

ComplianceEngine::Result<MofEntry::SemVer> MofEntry::SemVer::Parse(const std::string& version)
{
    if (version.find("v") != 0)
    {
        return Error("Invalid version format: must start with 'v' prefix");
    }

    size_t pos1 = version.find('.', 1);
    if (pos1 == std::string::npos)
    {
        return Error("Invalid version format: missing minor version");
    }

    size_t pos2 = version.find('.', pos1 + 1);
    if (pos2 == std::string::npos)
    {
        return Error("Invalid version format: missing patch version");
    }

    try
    {
        auto major = std::stoi(version.substr(1, pos1));
        auto minor = std::stoi(version.substr(pos1 + 1, pos2 - pos1 - 1));
        auto patch = std::stoi(version.substr(pos2 + 1));

        return SemVer{major, minor, patch};
    }
    catch (std::exception& e)
    {
        return Error(string("Invalid version format: ") + e.what());
    }
}

ComplianceEngine::Result<MofEntry::PayloadKey> MofEntry::PayloadKey::Parse(const std::string& input)
{
    if (input.find("/") != 0)
    {
        return Error("Invalid payload key format: must start with '/'");
    }

    auto tokens = SplitPayloadKey(input);
    if (!tokens.HasValue())
    {
        return tokens.Error();
    }
    auto benchmarkType = ParseBenchmarkType(tokens.Value()[0]);
    if (!benchmarkType.HasValue())
    {
        return benchmarkType.Error();
    }

    auto distributionType = ParseDistributionType(tokens.Value()[1]);
    if (!distributionType.HasValue())
    {
        return distributionType.Error();
    }

    auto benchmarkVersion = MofEntry::SemVer::Parse(tokens.Value()[3]);
    if (!benchmarkVersion.HasValue())
    {
        return benchmarkVersion.Error();
    }

    return MofEntry::PayloadKey{
        benchmarkType.Value(), distributionType.Value(),
        tokens.Value()[2], // distributionVersion
        benchmarkVersion.Value(),
        tokens.Value()[4] // section
    };
}

Result<MofEntry> ParseSingleEntry(std::istream& stream)
{
    string line;
    Optional<std::string> resourceID;
    Optional<MofEntry::PayloadKey> payloadKey;
    Optional<std::string> procedure;
    bool hasInitAudit = false;
    Optional<std::string> ruleName;
    Optional<std::string> payload;
    while (std::getline(stream, line))
    {
        if (line.find("ResourceID") != string::npos)
        {
            resourceID = getValue(line);
            continue;
        }

        if (line.find("PayloadKey") != string::npos)
        {
            auto result = MofEntry::PayloadKey::Parse(getValue(line));
            if (!result.HasValue())
            {
                return Error("Failed to parse PayloadKey: " + result.Error().message);
            }
            payloadKey = std::move(result.Value());
            continue;
        }

        if (line.find("ProcedureObjectValue") != std::string::npos)
        {
            procedure = getValue(line);
            continue;
        }

        if (line.find("InitObjectName") != std::string::npos)
        {
            auto value = getValue(line);
            if (value.find("init") != 0)
            {
                return Error("Invalid init object name");
            }
            hasInitAudit = true;
            continue;
        }

        if (line.find("ReportedObjectName") != std::string::npos)
        {
            auto value = getValue(line);
            if (value.find("audit") != 0)
            {
                return Error("Invalid reported object name");
            }
            ruleName = value.substr(strlen("audit"));
            continue;
        }

        if (line.find("DesiredObjectValue") != std::string::npos)
        {
            payload = getValue(line);
            continue;
        }

        if (line.find("};") != std::string::npos)
        {
            // End of MOF entry, validate and return
            if (!resourceID.HasValue())
            {
                return Error("Failed to parse MOF file: ResourceID is missing");
            }

            if (!payloadKey.HasValue())
            {
                return Error("Failed to parse MOF file: PayloadKey is missing");
            }

            if (!ruleName.HasValue())
            {
                return Error("Failed to parse MOF file: ReportedObjectName is missing");
            }

            if (!procedure.HasValue())
            {
                return Error("Failed to parse MOF file: ProcedureObjectValue is missing");
            }

            return MofEntry{std::move(resourceID.Value()), std::move(payloadKey.Value()), std::move(procedure.Value()), std::move(payload),
                std::move(ruleName.Value()), hasInitAudit};
        }
    }

    return Error("Failed to parse MOF file: unexpected end of file");
}
} // namespace mof
