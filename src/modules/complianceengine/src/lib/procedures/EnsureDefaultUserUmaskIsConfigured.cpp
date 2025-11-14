#include <CommonUtils.h>
#include <Evaluator.h>
#include <PasswordEntriesIterator.h>
#include <Regex.h>
#include <ScopeGuard.h>
#include <StringTools.h>
#include <Telemetry.h>
#include <algorithm>
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
        return regex(pattern, std::regex_constants::ECMAScript | std::regex_constants::icase);
    }
    catch (const regex_error& e)
    {
        OsConfigLogInfo(context.GetLogHandle(), "Regex error: %s", e.what());
        return Error("Regex error: " + string(e.what()), EINVAL);
    }
}

enum class MatchResult
{
    Correct,
    Incorrect,
    NotFound
};

mode_t ParseSymbolicValue(const std::string& user, const std::string& group, const std::string& other)
{
    mode_t value = 0;
    if (user.find("r") != string::npos)
    {
        value |= S_IRUSR;
    }
    if (user.find("w") != string::npos)
    {
        value |= S_IWUSR;
    }
    if (user.find("x") != string::npos)
    {
        value |= S_IXUSR;
    }

    if (group.find("r") != string::npos)
    {
        value |= S_IRGRP;
    }
    if (group.find("w") != string::npos)
    {
        value |= S_IWGRP;
    }
    if (group.find("x") != string::npos)
    {
        value |= S_IXGRP;
    }

    if (other.find("r") != string::npos)
    {
        value |= S_IROTH;
    }
    if (other.find("w") != string::npos)
    {
        value |= S_IWOTH;
    }
    if (other.find("x") != string::npos)
    {
        value |= S_IXOTH;
    }

    return ~value;
}

Result<MatchResult> MultilineMatch(const std::string& filename, const regex& valuePattern, const regex& pamPattern, ContextInterface& context)
{
    ifstream input(filename);
    if (!input.is_open())
    {
        return Error("Failed to open file: " + filename, errno);
    }

    int lineNumber = 0;
    string line;
    while (getline(input, line))
    {
        const mode_t expected = 027;
        lineNumber++;
        smatch match;

        if (regex_search(line, match, valuePattern))
        {
            OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());
            assert(match.ready());

            if (match.size() != 7)
            {
                return Error("Unexpected capture list size", EINVAL);
            }

            auto mask = match[2].str();
            if (!mask.empty())
            {
                auto octal = TryStringToInt(mask, 8);
                if (!octal.HasValue())
                {
                    // We expect an octal number as it's been sucessfully matched by the regex
                    return octal.Error();
                }

                if ((expected & octal.Value()) != expected)
                {
                    return MatchResult::Incorrect;
                }

                return MatchResult::Correct;
            }

            // We have a match, but not on the numeric result - we need to parse the symbolic mask
            auto symbolic = ParseSymbolicValue(match[3].str(), match[4].str(), match[5].str());
            if ((expected & symbolic) != expected)
            {
                return MatchResult::Incorrect;
            }
            return MatchResult::Correct;
        }

        if (regex_search(line, match, pamPattern))
        {
            OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());

            if (match.size() != 4)
            {
                return Error("Unexpected capture list size", EINVAL);
            }

            auto octal = TryStringToInt(match[3].str(), 8);
            if (!octal.HasValue())
            {
                // We expect an octal number as it's been sucessfully matched by the regex
                return octal.Error();
            }

            if ((expected & octal.Value()) != expected)
            {
                return MatchResult::Incorrect;
            }
            return MatchResult::Correct;
        }
    }

    return MatchResult::NotFound;
}
} // anonymous namespace

Result<Status> AuditEnsureDefaultUserUmaskIsConfigured(IndicatorsTree& indicators, ContextInterface& context)
{
    static const auto fileUmaskPattern = R"(^[ \t]*umask[ \t]+(([0-7]{3,4})|u=([rwx]{0,3}),g=([rwx]{0,3}),o=([rwx]{0,3}))([ \t]*#.*)?$)";
    static const auto pamPattern = R"(^[ \t]*session[ \t]+([^#\n\r]+[ \t]+)?pam_umask\.so[ \t]+([^#\n\r]+[ \t]+)?umask=([0-7]{3,4})\b)";

    static const auto valueRegex = CompileRegex(fileUmaskPattern, context);
    if (!valueRegex.HasValue())
    {
        return valueRegex.Error();
    }

    static const auto pamRegex = CompileRegex(pamPattern, context);
    if (!pamRegex.HasValue())
    {
        return pamRegex.Error();
    }

    vector<string> umaskLocations;
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
            OSConfigTelemetryStatusTrace("opendir", status);
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

            auto filePath = profiledPath + "/" + entry->d_name;
            if (filePath.size() >= 3 && filePath.rfind(".sh") == filePath.size() - 3)
            {
                umaskLocations.push_back(filePath);
            }
        }
    }

    const std::array<string, 6> standardLocations = {
        "/etc/profile", "/etc/bashrc", "/etc/bash.bashrc", "/etc/pam.d/postlogin", "/etc/login.defs", "/etc/default/login"};
    std::transform(standardLocations.begin(), standardLocations.end(), std::back_inserter(umaskLocations),
        [&context](const string& path) { return context.GetSpecialFilePath(path); });

    bool found = false;
    for (const auto& location : umaskLocations)
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

        auto result = MultilineMatch(location, valueRegex.Value(), pamRegex.Value(), context);
        if (!result.HasValue())
        {
            return result.Error();
        }

        if (result.Value() == MatchResult::NotFound)
        {
            continue;
        }
        found = true;

        if (result.Value() == MatchResult::Correct)
        {
            return indicators.Compliant("umask is correctly set in " + location);
        }

        if (result.Value() == MatchResult::Incorrect)
        {
            // We follow CIS guidance here and don't break early on incompmliance
            // to keep consistence with precedence.
            indicators.NonCompliant("umask is incorrectly set in " + location);
        }
    }

    if (!found)
    {
        // At least one umask setting must be found
        return indicators.NonCompliant("umask is not set");
    }

    // When this line is reached, it means we have already found umask setting(s),
    // but all of them were incompliant. This is true because we break early
    // on Compliant status.
    assert(!indicators.Back().indicators.empty());
    assert(indicators.Back().indicators.back().status == Status::NonCompliant);
    return Status::NonCompliant;
}
} // namespace ComplianceEngine
