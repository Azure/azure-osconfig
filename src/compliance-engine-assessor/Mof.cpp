#include <Mof.hpp>
#include <cassert>
#include <istream>

namespace mof
{
using compliance::Error;
using compliance::Result;
using std::istream;
using std::string;

namespace
{
string getValue(const std::string& line) {
    auto start = line.find('"');
    if (start == std::string::npos)
        return std::string();
    auto end = line.find('"', start + 1);
    if (end == std::string::npos)
        return std::string();
    return line.substr(start + 1, end - (start + 1));
};
} // anonymous namespace

Result<MofEntry> ParseSingleEntry(std::istream& stream)
{
    MofEntry result;
    string line;
    while (std::getline(stream, line))
    {
        if (line.find("ResourceID") != string::npos)
        {
            result.resourceID = getValue(line);
            continue;
        }

        if (line.find("PayloadKey") != string::npos)
        {
            result.payloadKey = getValue(line);
            continue;
        }

        if (line.find("ProcedureObjectValue") != std::string::npos)
        {
            result.procedure = getValue(line);
            continue;
        }

        if (line.find("InitObjectName") != std::string::npos)
        {
            auto value = getValue(line);
            if (value.find("init") != 0)
            {
                return Error("Invalid init object name");
            }
            result.hasInitAudit = true;
            continue;
        }

        if (line.find("ReportedObjectName") != std::string::npos)
        {
            auto value = getValue(line);
            if (value.find("audit") != 0)
            {
                return Error("Invalid reported object name");
            }
            result.ruleName = value.substr(strlen("audit"));
            continue;
        }

        if (line.find("DesiredObjectValue") != std::string::npos)
        {
            result.payload = getValue(line);
            continue;
        }

        if (line.find("};") != std::string::npos)
        {
            if (result.resourceID.empty())
            {
                return Error("Failed to parse MOF file: ResourceID must not be empty");
            }

            if(result.ruleName.empty())
            {
                return Error("Failed to parse MOF file: RuleName must not be empty");
            }

            if(result.payloadKey.empty())
            {
                return Error("Failed to parse MOF file: PayloadKey must not be empty");
            }

            return result;
        }
    }

    return Error("Failed to parse MOF file");
}
} // namespace mof
