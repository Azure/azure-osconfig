#include <Mof.hpp>
#include <JsonWrapper.h>
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
Result<string> GetValue(const std::string& line)
{
    const auto start = line.find('"');
    if (start == std::string::npos)
    {
        return Error("MOF string value is missing opening quote");
    }

    auto end = std::string::npos;
    auto escaped = false;
    for (auto index = start + 1; index < line.size(); ++index)
    {
        const auto current = line[index];
        if (escaped)
        {
            escaped = false;
            continue;
        }

        if ('\\' == current)
        {
            escaped = true;
            continue;
        }

        if ('"' == current)
        {
            end = index;
            break;
        }
    }

    if (end == std::string::npos)
    {
        return Error("MOF string value is missing closing quote");
    }

    const auto quotedValue = line.substr(start, end - start + 1);
    auto parsedValue = JsonWrapper::FromString(quotedValue);
    if (!parsedValue.HasValue())
    {
        return Error("Failed to parse MOF string value: " + parsedValue.Error().message);
    }

    if (JSONString != json_value_get_type(parsedValue.Value().get()))
    {
        return Error("Failed to parse MOF string value: parsed value is not a string");
    }

    const auto* value = json_value_get_string(parsedValue.Value().get());
    if (nullptr == value)
    {
        return Error("Failed to parse MOF string value: parsed string is null");
    }

    return string(value);
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
            auto value = GetValue(line);
            if (!value.HasValue())
            {
                return value.Error();
            }
            resourceID = std::move(value.Value());
            continue;
        }

        if (line.find("PayloadKey") != string::npos)
        {
            auto value = GetValue(line);
            if (!value.HasValue())
            {
                return Error("Failed to parse PayloadKey: " + value.Error().message);
            }

            auto result = CISBenchmarkInfo::Parse(value.Value());
            if (!result.HasValue())
            {
                return Error("Failed to parse PayloadKey: " + result.Error().message);
            }
            benchmarkInfo = std::move(result.Value());
            continue;
        }

        if (line.find("ProcedureObjectValue") != std::string::npos)
        {
            auto value = GetValue(line);
            if (!value.HasValue())
            {
                return Error("Failed to parse ProcedureObjectValue: " + value.Error().message);
            }
            procedure = std::move(value.Value());
            continue;
        }

        if (line.find("InitObjectName") != std::string::npos)
        {
            auto value = GetValue(line);
            if (!value.HasValue())
            {
                return Error("Failed to parse InitObjectName: " + value.Error().message);
            }
            if (value.Value().find("init") != 0)
            {
                return Error("Invalid init object name");
            }
            hasInitAudit = true;
            continue;
        }

        if (line.find("ReportedObjectName") != std::string::npos)
        {
            auto value = GetValue(line);
            if (!value.HasValue())
            {
                return Error("Failed to parse ReportedObjectName: " + value.Error().message);
            }
            if (value.Value().find("audit") != 0)
            {
                return Error("Invalid reported object name");
            }
            ruleName = value.Value().substr(strlen("audit"));
            continue;
        }

        if (line.find("DesiredObjectValue") != std::string::npos)
        {
            auto value = GetValue(line);
            if (!value.HasValue())
            {
                return Error("Failed to parse DesiredObjectValue: " + value.Error().message);
            }
            payload = std::move(value.Value());
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
