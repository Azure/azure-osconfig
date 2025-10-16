#include <Mof.hpp>
#include <algorithm>
#include <array>

namespace ComplianceEngine
{
namespace MOF
{
using std::array;
using std::istream;
using std::map;
using std::string;

namespace
{
string GetValue(const std::string& line)
{
    const auto start = line.find('"');
    if (start == std::string::npos)
    {
        return std::string();
    }
    const auto end = line.find('"', start + 1);
    if (end == std::string::npos)
    {
        return std::string();
    }
    return line.substr(start + 1, end - (start + 1));
};
} // anonymous namespace

Result<SemVer> SemVer::Parse(const std::string& version)
{
    if (version.find("v") != 0)
    {
        return Error("Invalid version format: must start with 'v' prefix");
    }

    const auto pos1 = version.find('.', 1);
    if (pos1 == std::string::npos)
    {
        return Error("Invalid version format: missing minor version");
    }

    const auto pos2 = version.find('.', pos1 + 1);
    if (pos2 == std::string::npos)
    {
        return Error("Invalid version format: missing patch version");
    }

    try
    {
        const auto major = std::stoi(version.substr(1, pos1));
        const auto minor = std::stoi(version.substr(pos1 + 1, pos2 - pos1 - 1));
        const auto patch = std::stoi(version.substr(pos2 + 1));

        return SemVer{major, minor, patch};
    }
    catch (std::exception& e)
    {
        return Error(string("Invalid version format: ") + e.what());
    }
}

Result<Resource> Resource::ParseSingleEntry(std::istream& stream)
{
    string line;
    Optional<std::string> resourceID;
    Optional<CISBenchmarkInfo> benchmarkInfo;
    Optional<std::string> procedure;
    bool hasInitAudit = false;
    Optional<std::string> ruleName;
    Optional<std::string> payload;
    while (std::getline(stream, line))
    {
        if (line.find("ResourceID") != string::npos)
        {
            resourceID = GetValue(line);
            continue;
        }

        if (line.find("PayloadKey") != string::npos)
        {
            auto result = CISBenchmarkInfo::Parse(GetValue(line));
            if (!result.HasValue())
            {
                return Error("Failed to parse PayloadKey: " + result.Error().message);
            }
            benchmarkInfo = std::move(result.Value());
            continue;
        }

        if (line.find("ProcedureObjectValue") != std::string::npos)
        {
            procedure = GetValue(line);
            continue;
        }

        if (line.find("InitObjectName") != std::string::npos)
        {
            auto value = GetValue(line);
            if (value.find("init") != 0)
            {
                return Error("Invalid init object name");
            }
            hasInitAudit = true;
            continue;
        }

        if (line.find("ReportedObjectName") != std::string::npos)
        {
            auto value = GetValue(line);
            if (value.find("audit") != 0)
            {
                return Error("Invalid reported object name");
            }
            ruleName = value.substr(strlen("audit"));
            continue;
        }

        if (line.find("DesiredObjectValue") != std::string::npos)
        {
            payload = GetValue(line);
            continue;
        }

        if (line.find("};") != std::string::npos)
        {
            // End of MOF entry, validate and return
            break;
        }
    }

    Resource resource;
    if (!resourceID.HasValue())
    {
        return Error("Failed to parse MOF file: ResourceID is missing");
    }
    resource.resourceID = std::move(resourceID.Value());

    if (!benchmarkInfo.HasValue())
    {
        return Error("Failed to parse MOF file: PayloadKey is missing");
    }
    resource.benchmarkInfo = std::move(benchmarkInfo.Value());
    auto& section = resource.benchmarkInfo.section;
    std::transform(section.begin(), section.end(), section.begin(), [](char c) {
        if (c == '/')
        {
            return '.';
        }
        return c;
    });

    if (!ruleName.HasValue())
    {
        return Error("Failed to parse MOF file: ReportedObjectName is missing");
    }
    resource.ruleName = std::move(ruleName.Value());

    if (!procedure.HasValue())
    {
        return Error("Failed to parse MOF file: ProcedureObjectValue is missing");
    }
    resource.procedure = std::move(procedure.Value());
    resource.payload = std::move(payload);
    resource.hasInitAudit = hasInitAudit;

    return resource;
}
} // namespace MOF
} // namespace ComplianceEngine
