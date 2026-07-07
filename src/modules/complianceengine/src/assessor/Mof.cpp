// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Mof.hpp"

#include "InputSecurity.hpp"

#include <StringTools.h>
#include <algorithm>
#include <cerrno>
#include <ext/stdio_filebuf.h>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace ComplianceEngine
{
namespace MOF
{
using std::map;
using std::string;

namespace
{
// Upper bound on a single MOF line. MOF values (notably the base64-encoded
// ProcedureObjectValue) can be long, but a multi-megabyte line indicates a
// malformed or hostile input rather than a real benchmark entry. The line read
// stops as soon as this many bytes have accumulated, so a newline-free input
// cannot exhaust memory while we run as root.
constexpr size_t kMaxLineLength = static_cast<size_t>(4) * 1024 * 1024;

// Upper bound on the total number of input bytes consumed across all entries.
constexpr size_t kMaxInputBytes = static_cast<size_t>(8) * 1024 * 1024;

// Upper bound on the number of MOF entries processed from a single input.
constexpr size_t kMaxMofEntries = 100000;

// Header that introduces each resource block, e.g.
// "instance of OsConfigResource as $OsConfigResource0ref".
constexpr char kHeaderPrefix[] = "instance of OsConfigResource as $OsConfigResource";
constexpr char kHeaderSuffix[] = "ref";

// The complete, fixed set of field keys the Compliance Augmentation Engine
// emits for every resource. The parser is strict: it rejects any key not in
// this set (unknown/extra fields) and requires every key in this set to be
// present (missing fields). This is the single source of truth for both checks.
const std::set<string>& KnownMofKeys()
{
    static const std::set<string> keys = {"ResourceID", "PayloadKey", "RuleId", "ComponentName", "ProcedureObjectName", "ProcedureObjectValue",
        "InitObjectName", "ReportedObjectName", "ExpectedObjectValue", "DesiredObjectName", "DesiredObjectValue", "ModuleName", "ModuleVersion",
        "ConfigurationName", "SourceInfo"};
    return keys;
}

// Returns the suffix of `value` after `prefix`, or an empty Optional when
// `value` does not start with `prefix`.
Optional<string> StripPrefix(const string& value, const string& prefix)
{
    if (value.size() < prefix.size() || value.compare(0, prefix.size(), prefix) != 0)
    {
        return Optional<string>();
    }
    return value.substr(prefix.size());
}

// Extracts the double-quoted value beginning at or after `from`, honouring \"
// and \\ escape sequences so that MOF string values like "{\"key\":\"val\"}"
// are returned unescaped as {"key":"val"}. Requires both an opening and a
// closing quote; a missing quote is a parse error.
Result<string> ParseQuoted(const string& line, size_t from)
{
    const auto open = line.find('"', from);
    if (open == string::npos)
    {
        return Error("MOF field value is not quoted: '" + line + "'", EINVAL);
    }

    string result;
    size_t pos = open + 1;
    bool closed = false;
    while (pos < line.size())
    {
        const char c = line[pos];
        if (c == '\\' && pos + 1 < line.size())
        {
            const char next = line[pos + 1];
            if (next == '"' || next == '\\')
            {
                result += next;
                pos += 2;
                continue;
            }
        }
        if (c == '"')
        {
            closed = true;
            break;
        }
        result += c;
        ++pos;
    }

    if (!closed)
    {
        return Error("MOF field value is missing a closing quote: '" + line + "'", EINVAL);
    }

    const string tail = TrimWhiteSpaces(line.substr(pos + 1));
    if (tail != ";")
    {
        return Error("MOF field line must end with ';': '" + line + "'", EINVAL);
    }
    return result;
}

// Parses a single `Key = "value";` field line into its key and unescaped value.
Result<std::pair<string, string>> ParseFieldLine(const string& line)
{
    const auto eq = line.find('=');
    if (eq == string::npos)
    {
        return Error("MOF field line is missing '=': '" + line + "'", EINVAL);
    }

    const string key = TrimWhiteSpaces(line.substr(0, eq));
    if (key.empty())
    {
        return Error("MOF field line has an empty key: '" + line + "'", EINVAL);
    }

    auto value = ParseQuoted(line, eq + 1);
    if (!value.HasValue())
    {
        return value.Error();
    }
    return std::make_pair(key, std::move(value.Value()));
}

// Validates a fully-collected field map and assembles the Resource. Enforces
// the constant fields, the required-field set, and rule-name consistency across
// the four object-name fields. The base64 ProcedureObjectValue is stored as-is;
// decoding and JSON validation are the ComplianceEngine's responsibility, which
// keeps the parser/engine boundary explicit.
Result<Resource> BuildResource(const map<string, string>& fields)
{
    for (const auto& key : KnownMofKeys())
    {
        if (fields.find(key) == fields.end())
        {
            return Error("MOF entry is missing required field: '" + key + "'", EINVAL);
        }
    }

    if (fields.at("ComponentName") != "ComplianceEngine")
    {
        return Error("MOF entry has unexpected ComponentName: '" + fields.at("ComponentName") + "'", EINVAL);
    }
    if (fields.at("ConfigurationName") != "ComplianceEngine")
    {
        return Error("MOF entry has unexpected ConfigurationName: '" + fields.at("ConfigurationName") + "'", EINVAL);
    }
    if (fields.at("ExpectedObjectValue") != "PASS")
    {
        return Error("MOF entry has unexpected ExpectedObjectValue: '" + fields.at("ExpectedObjectValue") + "'", EINVAL);
    }

    const auto procedureName = StripPrefix(fields.at("ProcedureObjectName"), "procedure");
    if (!procedureName.HasValue())
    {
        return Error("ProcedureObjectName must start with 'procedure': '" + fields.at("ProcedureObjectName") + "'", EINVAL);
    }
    const auto initName = StripPrefix(fields.at("InitObjectName"), "init");
    if (!initName.HasValue())
    {
        return Error("InitObjectName must start with 'init': '" + fields.at("InitObjectName") + "'", EINVAL);
    }
    const auto auditName = StripPrefix(fields.at("ReportedObjectName"), "audit");
    if (!auditName.HasValue())
    {
        return Error("ReportedObjectName must start with 'audit': '" + fields.at("ReportedObjectName") + "'", EINVAL);
    }
    const auto remediateName = StripPrefix(fields.at("DesiredObjectName"), "remediate");
    if (!remediateName.HasValue())
    {
        return Error("DesiredObjectName must start with 'remediate': '" + fields.at("DesiredObjectName") + "'", EINVAL);
    }

    const string& ruleName = procedureName.Value();
    if (ruleName.empty())
    {
        return Error("MOF entry has an empty rule name", EINVAL);
    }
    if (initName.Value() != ruleName || auditName.Value() != ruleName || remediateName.Value() != ruleName)
    {
        return Error("MOF entry object names disagree on the rule name '" + ruleName + "'", EINVAL);
    }

    if (fields.at("ResourceID").empty())
    {
        return Error("MOF entry has an empty ResourceID", EINVAL);
    }
    if (fields.at("ProcedureObjectValue").empty())
    {
        return Error("MOF entry has an empty ProcedureObjectValue", EINVAL);
    }

    auto benchmarkInfo = CISBenchmarkInfo::Parse(fields.at("PayloadKey"));
    if (!benchmarkInfo.HasValue())
    {
        return Error("Failed to parse PayloadKey: " + benchmarkInfo.Error().message, benchmarkInfo.Error().code);
    }

    Resource resource;
    resource.resourceID = fields.at("ResourceID");
    resource.benchmarkInfo = std::move(benchmarkInfo.Value());
    // The section in the payload key is '/'-separated (e.g. "1/1/1/1"); the rest
    // of the assessor expects dotted notation (e.g. "1.1.1.1").
    std::replace(resource.benchmarkInfo.section.begin(), resource.benchmarkInfo.section.end(), '/', '.');
    resource.procedure = fields.at("ProcedureObjectValue");
    resource.ruleName = ruleName;
    resource.hasInitAudit = true; // InitObjectName is required and validated above.

    // An empty DesiredObjectValue (emitted for every rule today) is modelled as
    // an absent payload; a non-empty value is carried through.
    const string& desired = fields.at("DesiredObjectValue");
    if (!desired.empty())
    {
        resource.payload = desired;
    }

    return resource;
}
} // anonymous namespace

MofResourceRange::MofResourceRange(std::istream& stream, OsConfigLogHandle logHandle) noexcept
    : mStream(&stream),
      mLog(logHandle),
      mOwnedBuf(),
      mOwnedStream()

{
}

MofResourceRange::MofResourceRange(MofResourceRange&& other) noexcept
    : mStream(other.mStream),
      mLog(other.mLog),
      mOwnedBuf(std::move(other.mOwnedBuf)),
      mOwnedStream(std::move(other.mOwnedStream)),
      mBytesConsumed(other.mBytesConsumed),
      mEntryCount(other.mEntryCount)
{
    other.mStream = nullptr;
}

Result<MofResourceRange> MofResourceRange::Make(const string& path, OsConfigLogHandle logHandle)
{
    // Encapsulate the full input-hardening posture so callers never open the
    // file themselves: reject path traversal, require a root-owned non-writable
    // parent directory, and open with O_NOFOLLOW plus regular-file/ownership/
    // mode checks on the resulting fd.
    if (Assessor::RefusePathTraversal(path, logHandle))
    {
        return Error("Refusing to open MOF input with an unsafe path: '" + path + "'", EACCES);
    }
    if (Assessor::RefuseWritableParentDir(path, logHandle))
    {
        return Error("Refusing to open MOF input in a writable parent directory: '" + path + "'", EACCES);
    }
    auto fdResult = Assessor::OpenVerifiedInput(path, logHandle);
    if (!fdResult.HasValue())
    {
        return fdResult.Error();
    }

    // Bridge the verified fd into a std::istream for streaming. stdio_filebuf
    // takes ownership of the fd and closes it when destroyed.
    std::unique_ptr<__gnu_cxx::stdio_filebuf<char>> buffer(new __gnu_cxx::stdio_filebuf<char>(fdResult.Value(), std::ios_base::in));
    std::unique_ptr<std::istream> stream(new std::istream(buffer.get()));

    MofResourceRange range(*stream, logHandle);
    range.mOwnedBuf = std::move(buffer);
    range.mOwnedStream = std::move(stream);
    range.mStream = range.mOwnedStream.get();
    return range;
}

Result<MofResourceRange> MofResourceRange::MakeFromStream(std::istream& stream, OsConfigLogHandle logHandle)
{
    return MofResourceRange(stream, logHandle);
}

Result<MofResourceRange> MofResourceRange::Make(std::istream& stream, OsConfigLogHandle logHandle)
{
    return MofResourceRange(stream, logHandle);
}

MofResourceIterator MofResourceRange::begin() // NOLINT(*-identifier-naming)
{
    return MofResourceIterator(*this);
}

MofResourceIterator MofResourceRange::end() // NOLINT(*-identifier-naming)
{
    return MofResourceIterator();
}

Result<Optional<Resource>> MofResourceRange::ParseNext()
{
    // Reads a single line from the stream.  Returns:
    //   Error              — a resource cap was exceeded (line too long, total bytes too large)
    //   Optional<string>() — clean end of input (EOF with nothing buffered)
    //   Optional<string>(line) — one complete line (newline consumed but not included)
    //
    // A non-empty partial line at EOF (no trailing newline) is returned as an
    // error: the augmentation engine always terminates every block with '};'
    // followed by a newline, so a line without a terminator means the input was
    // truncated.
    const auto readLine = [this]() -> Result<Optional<string>> {
        string line;
        std::istream& stream = *mStream;
        while (true)
        {
            const std::istream::int_type ch = stream.get();
            if (ch == std::istream::traits_type::eof())
            {
                if (stream.bad())
                {
                    return Error("I/O error reading MOF input", EIO);
                }
                if (line.empty())
                {
                    return Optional<string>(); // Clean EOF.
                }
                return Error("Truncated MOF input: last line has no newline terminator", EIO);
            }
            if (ch == '\n')
            {
                // Strip a trailing '\r' to handle CRLF line endings.
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                // Count the line terminator alongside the content.
                if (++mBytesConsumed > kMaxInputBytes)
                {
                    return Error("MOF input exceeds the maximum size of " + std::to_string(kMaxInputBytes) + " bytes", E2BIG);
                }
                return Optional<string>(std::move(line));
            }
            if (line.size() >= kMaxLineLength)
            {
                return Error("MOF line exceeds the maximum length of " + std::to_string(kMaxLineLength) + " bytes", E2BIG);
            }
            line.push_back(static_cast<char>(ch));
            if (++mBytesConsumed > kMaxInputBytes)
            {
                return Error("MOF input exceeds the maximum size of " + std::to_string(kMaxInputBytes) + " bytes", E2BIG);
            }
        }
    };

    // Locate the next entry header, skipping blank lines between entries. A
    // clean end of input here means there are no more entries.
    string header;
    while (true)
    {
        auto read = readLine();
        if (!read.HasValue())
        {
            return read.Error();
        }
        if (!read.Value().HasValue())
        {
            return Optional<Resource>(); // Clean EOF — no more entries.
        }
        header = TrimWhiteSpaces(read.Value().Value());
        if (!header.empty())
        {
            break;
        }
    }
    const size_t prefixLen = sizeof(kHeaderPrefix) - 1;
    const size_t suffixLen = sizeof(kHeaderSuffix) - 1;
    if (header.size() < prefixLen + suffixLen || header.compare(0, prefixLen, kHeaderPrefix) != 0 ||
        header.compare(header.size() - suffixLen, suffixLen, kHeaderSuffix) != 0)
    {
        return Error("Malformed MOF entry header: '" + header + "'", EINVAL);
    }

    if (++mEntryCount > kMaxMofEntries)
    {
        return Error("MOF input exceeds the maximum of " + std::to_string(kMaxMofEntries) + " entries", E2BIG);
    }

    // The header must be followed by an opening brace on its own line.
    {
        auto read = readLine();
        if (!read.HasValue())
        {
            return read.Error();
        }
        if (!read.Value().HasValue() || TrimWhiteSpaces(read.Value().Value()) != "{")
        {
            return Error("Expected '{' after MOF entry header", EINVAL);
        }
    }

    // Collect the field lines up to the closing '};', rejecting unknown and
    // duplicate keys.
    map<string, string> fields;
    bool closed = false;
    while (true)
    {
        auto read = readLine();
        if (!read.HasValue())
        {
            return read.Error();
        }
        if (!read.Value().HasValue())
        {
            break; // EOF before '};'
        }

        const string trimmed = TrimWhiteSpaces(read.Value().Value());
        if (trimmed.empty())
        {
            continue;
        }
        if (trimmed == "};")
        {
            closed = true;
            break;
        }

        auto field = ParseFieldLine(trimmed);
        if (!field.HasValue())
        {
            return field.Error();
        }
        if (KnownMofKeys().find(field.Value().first) == KnownMofKeys().end())
        {
            return Error("Unknown MOF field key: '" + field.Value().first + "'", EINVAL);
        }
        if (!fields.emplace(field.Value().first, std::move(field.Value().second)).second)
        {
            return Error("Duplicate MOF field key: '" + field.Value().first + "'", EINVAL);
        }
    }

    if (!closed)
    {
        return Error("Truncated MOF entry: missing closing '};'", EIO);
    }

    auto resource = BuildResource(fields);
    if (!resource.HasValue())
    {
        return resource.Error();
    }
    return Optional<Resource>(std::move(resource.Value()));
}

MofResourceIterator::MofResourceIterator(MofResourceRange& range)
    : mRange(&range)
{
    Advance();
}

void MofResourceIterator::Advance()
{
    auto result = mRange->ParseNext();
    if (!result.HasValue())
    {
        // Surface the parse error once; the next increment terminates iteration
        // because a desynchronized stream cannot be resumed reliably.
        mCurrent = Result<Resource>(result.Error());
        mErrored = true;
        return;
    }

    Optional<Resource>& entry = result.Value();
    if (!entry.HasValue())
    {
        mRange = nullptr; // Clean end of input.
        return;
    }
    mCurrent = Result<Resource>(std::move(entry.Value()));
}

MofResourceIterator& MofResourceIterator::operator++()
{
    if (mErrored)
    {
        mRange = nullptr;
        return *this;
    }
    if (mRange != nullptr)
    {
        Advance();
    }
    return *this;
}

MofResourceIterator::reference MofResourceIterator::operator*() const
{
    return mCurrent;
}

MofResourceIterator::pointer MofResourceIterator::operator->() const
{
    return &mCurrent;
}

bool MofResourceIterator::operator==(const MofResourceIterator& other) const
{
    return mRange == other.mRange;
}

bool MofResourceIterator::operator!=(const MofResourceIterator& other) const
{
    return mRange != other.mRange;
}
} // namespace MOF
} // namespace ComplianceEngine
