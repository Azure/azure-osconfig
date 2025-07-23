#include <CommonUtils.h>
#include <Evaluator.h>
#include <PasswordEntriesIterator.h>
#include <Regex.h>
#include <ScopeGuard.h>
#include <StringTools.h>
#include <array>
#include <dirent.h>
#include <fstream>
#include <pwd.h>
#include <shadow.h>
#include <sys/stat.h>
#include <vector>

using std::ifstream;
using std::map;
using std::string;
using std::vector;

namespace ComplianceEngine
{
namespace
{
Result<regex> CompileRegex(const std::string& pattern, const ContextInterface& context)
{
    try
    {
        return regex(pattern, std::regex_constants::ECMAScript);
    }
    catch (const regex_error& e)
    {
        OsConfigLogInfo(context.GetLogHandle(), "Regex error: %s", e.what());
        return Error("Regex error: " + string(e.what()), EINVAL);
    }
}

struct MatchResult
{
    bool found = false;
    bool multiple = false;
    bool correct = false;
    bool readonly = false;
    bool exported = false;
};

Result<MatchResult> MultilineMatch(const std::string& filename, const regex& valuePattern, const regex& readonlyPattern, const regex& exportPattern,
    ContextInterface& context)
{
    ifstream input(filename);
    if (!input.is_open())
    {
        return Error("Failed to open file: " + filename, errno);
    }

    int lineNumber = 0;
    MatchResult result;
    string line;
    while (getline(input, line))
    {
        lineNumber++;
        smatch match;

        if (regex_search(line, match, valuePattern))
        {
            OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());
            assert(match.ready());

            if (result.found)
            {
                result.multiple = true;
                return result;
            }
            result.found = true;

            if (match.size() != 3)
            {
                return Error("Invalid size of regex capture list", EINVAL);
            }

            auto value = TryStringToInt(match[2].str());
            if (!value.HasValue())
            {
                return value.Error();
            }

            if (value.Value() <= 900)
            {
                result.correct = true;
            }
        }

        if (regex_search(line, readonlyPattern))
        {
            OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());
            result.readonly = true;
        }

        if (regex_search(line, exportPattern))
        {
            OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());
            result.exported = true;
        }
    }

    return result;
}
} // anonymous namespace

AUDIT_FN(EnsureDefaultShellTimeoutIsConfigured, "")
{
    UNUSED(args);
    static const auto valuePattern = R"(^[ \\t]*([^#\n\r]+[ \\t]+)?TMOUT=([[:alnum:]]+)\b)";
    static const auto readonlyPattern = R"(^[ \\t]*([^#\n]+[ \\t]+)?readonly[ \\t]+TMOUT\b)";
    static const auto exportPattern = R"(^([ \\t]*|[ \\t]*[^#\n]+[ \\t]*;[ \\t]*)export[ \\t]+TMOUT\b)";

    static const auto valueRegex = CompileRegex(valuePattern, context);
    if (!valueRegex.HasValue())
    {
        return valueRegex.Error();
    }

    static const auto readonlyRegex = CompileRegex(readonlyPattern, context);
    if (!readonlyRegex.HasValue())
    {
        return readonlyRegex.Error();
    }

    static const auto exportRegex = CompileRegex(exportPattern, context);
    if (!exportRegex.HasValue())
    {
        return exportRegex.Error();
    }

    vector<string> locations = {
        context.GetSpecialFilePath("/etc/bashrc"),
        context.GetSpecialFilePath("/etc/bash.bashrc"),
        context.GetSpecialFilePath("/etc/profile"),
    };

    const auto& profiledPath = context.GetSpecialFilePath("/etc/profile.d/");
    auto* dir = opendir(profiledPath.c_str());
    ScopeGuard guard([dir]() {
        if (dir)
        {
            closedir(dir);
        }
    });
    if (!dir)
    {
        int status = errno;
        if (ENOENT != status)
        {
            OsConfigLogError(context.GetLogHandle(), "Failed to open directory '%s': %s", profiledPath.c_str(), strerror(status));
            return Error(string("Failed to open directory '") + profiledPath + "': " + strerror(status), status);
        }
    }
    else
    {
        for (dirent* entry = readdir(dir); entry != nullptr; entry = readdir(dir))
        {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            auto filePath = profiledPath + entry->d_name;
            if (filePath.size() >= 3 && filePath.rfind(".sh") == filePath.size() - 3)
            {
                locations.push_back(filePath);
            }
        }
    }

    bool found = false;
    for (const auto& location : locations)
    {
        struct stat st;
        if (0 != stat(location.c_str(), &st))
        {
            int status = errno;
            if (ENOENT == status)
            {
                continue;
            }

            return Error(string("Failed to stat ") + location + ": " + strerror(status), status);
        }

        auto result = MultilineMatch(location, valueRegex.Value(), readonlyRegex.Value(), exportRegex.Value(), context);
        if (!result.HasValue())
        {
            return result.Error();
        }

        if (result->found)
        {
            if (result->correct)
            {
                indicators.Compliant(string("TMOUT is set to a correct value in ") + location);
            }
            else
            {
                return indicators.NonCompliant(string("TMOUT is set to an incorrect value in ") + location);
            }

            if (result->multiple)
            {
                return indicators.NonCompliant(string("TMOUT is set multiple times in ") + location);
            }

            if (result->readonly)
            {
                indicators.Compliant(string("TMOUT is set readonly in ") + location);
            }
            else
            {
                return indicators.NonCompliant(string("TMOUT is not readonly in ") + location);
            }

            if (result->exported)
            {
                indicators.Compliant(string("TMOUT is exported in ") + location);
            }
            else
            {
                return indicators.NonCompliant(string("TMOUT is not exported in ") + location);
            }

            if (found)
            {
                return indicators.NonCompliant("TMOUT is set in multiple locations");
            }
            found = true;
        }
    }

    if (!found)
    {
        return indicators.NonCompliant("TMOUT is not set");
    }

    return indicators.Compliant("TMOUT variable is properly defined");
}
} // namespace ComplianceEngine
