// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Mof.hpp>
#include <algorithm>
#include <cstring>
#include <istream>
#include <string>

namespace ComplianceEngine
{
namespace MOF
{
using std::istream;
using std::string;

namespace
{

// Reads a single line (up to `\n`, which is discarded) into `out`. Returns true
// on success, false on EOF before any character was read. Sets `tooLong=true`
// when the line exceeds kMaxLineLength; in that case the remainder of the line
// is consumed and discarded so the caller can continue parsing safely.
bool ReadBoundedLine(istream& stream, string& out, bool& tooLong)
{
    out.clear();
    tooLong = false;
    int ch = stream.get();
    if (ch == std::char_traits<char>::eof())
    {
        return false;
    }

    while (ch != std::char_traits<char>::eof() && ch != '\n')
    {
        if (out.size() < kMaxLineLength)
        {
            out.push_back(static_cast<char>(ch));
        }
        else
        {
            tooLong = true;
        }
        ch = stream.get();
    }

    if (tooLong)
    {
        out.clear();
    }
    return true;
}

string GetValue(const string& line)
{
    const auto start = line.find('"');
    if (start == string::npos)
    {
        return string();
    }
    // Walk from the opening quote, honouring \" and \\ escape sequences so
    // that MOF string values like "{\"key\":\"val\"}" are returned
    // unescaped as {"key":"val"} instead of being truncated at the first
    // inner quote.
    string result;
    size_t pos = start + 1;
    while (pos < line.size())
    {
        if (line[pos] == '\\' && pos + 1 < line.size())
        {
            const char next = line[pos + 1];
            if (next == '"' || next == '\\')
            {
                result += next;
                pos += 2;
                continue;
            }
        }
        else if (line[pos] == '"')
        {
            break;
        }
        result += line[pos];
        ++pos;
    }
    return result;
}

} // anonymous namespace

Result<Resource> Resource::ParseSingleEntry(std::istream& stream)
{
    string line;
    Optional<string> resourceID;
    Optional<CISBenchmarkInfo> benchmarkInfo;
    Optional<string> procedure;
    bool hasInitAudit = false;
    Optional<string> ruleName;
    Optional<string> payload;

    bool tooLong = false;
    while (ReadBoundedLine(stream, line, tooLong))
    {
        if (tooLong)
        {
            return Error("MOF line exceeds maximum length", EFBIG);
        }

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

        if (line.find("ProcedureObjectValue") != string::npos)
        {
            procedure = GetValue(line);
            continue;
        }

        if (line.find("InitObjectName") != string::npos)
        {
            auto value = GetValue(line);
            if (value.find("init") != 0)
            {
                return Error("Invalid init object name");
            }
            hasInitAudit = true;
            continue;
        }

        if (line.find("ReportedObjectName") != string::npos)
        {
            auto value = GetValue(line);
            if (value.find("audit") != 0)
            {
                return Error("Invalid reported object name");
            }
            ruleName = value.substr(std::strlen("audit"));
            continue;
        }

        if (line.find("DesiredObjectValue") != string::npos)
        {
            payload = GetValue(line);
            continue;
        }

        if (line.find("};") != string::npos)
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

Optional<Error> ParseAll(std::istream& stream, const EntryCallback& callback)
{
    if (!callback)
    {
        return Error("ParseAll requires a non-null callback", EINVAL);
    }

    string line;
    std::size_t entries = 0;
    bool tooLong = false;
    while (ReadBoundedLine(stream, line, tooLong))
    {
        if (tooLong)
        {
            return Error("MOF line exceeds maximum length", EFBIG);
        }

        if (line.find("instance of OsConfigResource as") == string::npos)
        {
            continue;
        }

        if (entries >= kMaxEntriesPerStream)
        {
            return Error("MOF stream exceeds maximum number of entries", E2BIG);
        }

        auto parsed = Resource::ParseSingleEntry(stream);
        if (!parsed.HasValue())
        {
            return parsed.Error();
        }
        ++entries;

        auto cbErr = callback(std::move(parsed.Value()));
        if (cbErr.HasValue())
        {
            return cbErr;
        }
    }

    return Optional<Error>();
}

} // namespace MOF
} // namespace ComplianceEngine
