// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Internal.h>
#include <Optional.h>
#include <Regex.h>
#include <Result.h>
#include <dirent.h>
#include <fstream>

namespace ComplianceEngine
{
using std::ifstream;
using std::string;
using std::regex_constants::syntax_option_type;
namespace
{

// syntax Options for matchPattern and statePattern respectively
using MatchStateSyntaxOptions = std::pair<syntax_option_type, syntax_option_type>;

// This function is used to check if the file contents match the given pattern.
// It reads the file line by line and checks each line against the matchPattern.
// If a line matches the matchPattern, it checks if the statePattern (if provided) also matches.
// Based on the result of the matches, true is returned if the file matches, false if it doesn't.
Result<bool> MultilineMatch(const std::string& filename, const string& matchPattern, const Optional<string>& statePattern,
    MatchStateSyntaxOptions syntaxOptions, ContextInterface& context)
{
    Optional<regex> matchRegex;
    Optional<regex> stateRegex;

    ifstream input(filename);
    if (!input.is_open())
    {
        return Error("Failed to open file: " + filename, errno);
    }
    try
    {
        matchRegex = regex(matchPattern, syntaxOptions.first);
        if (statePattern.HasValue())
        {
            stateRegex = regex(statePattern.Value(), syntaxOptions.second);
        }
    }
    catch (const regex_error& e)
    {
        OsConfigLogInfo(context.GetLogHandle(), "Regex error: %s", e.what());
        return Error("Regex error: " + string(e.what()), EINVAL);
    }

    int lineNumber = 0;
    string line;
    while (getline(input, line))
    {
        lineNumber++;
        OsConfigLogDebug(context.GetLogHandle(), "Matching line %d: '%s', pattern: '%s'", lineNumber, line.c_str(), matchPattern.c_str());
        smatch match;
        if (regex_search(line, match, matchRegex.Value()))
        {
            OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());
            if (stateRegex.HasValue())
            {
                assert(match.ready());
                assert(match.size() > 0);
                auto valueToMatch = match.size() > 1 ? match[1].str() : match[0].str();
                OsConfigLogDebug(context.GetLogHandle(), "Value to match: %s", valueToMatch.c_str());
                if (regex_search(valueToMatch, stateRegex.Value()))
                {
                    OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());
                    return true;
                }
            }
            else
            {
                OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());
                return true;
            }
        }
    }
    return false;
}
} // anonymous namespace

AUDIT_FN(FileRegexMatch, "path:A directory name contining files to check:M", "filenamePattern:A pattern to match file names in the provided path:M",
    "matchOperation:Operation to perform on the file contents::^pattern match$", "matchPattern:The pattern to match against the file contents:M",
    "stateOperation:Operation to perform on each line that matches the 'matchPattern'::^pattern match$",
    "statePattern:The pattern to match against each line that matches the 'statePattern'",
    "ignoreCase:Determine whether the a match or state should be ignore case sensitivity  'matchPattern' and 'statePattern' or none when empty'::"
    "^(matchPattern\\sstatePattern|matchPattern|statePattern)",
    "behavior:Determine the function behavior::^(all_exist|any_exist|at_least_one_exists|none_exist)$")
{
    UNUSED(context);
    auto it = args.find("path");
    if (it == args.end())
    {
        return Error("Missing 'path' parameter", EINVAL);
    }
    auto path = std::move(it->second);

    it = args.find("filenamePattern");
    if (it == args.end())
    {
        return Error("Missing 'filenamePattern' parameter", EINVAL);
    }
    auto filenamePattern = std::move(it->second);

    Optional<regex> filenameRegex;
    try
    {
        filenameRegex = regex(filenamePattern, std::regex_constants::ECMAScript);
    }
    catch (const regex_error& e)
    {
        return Error("Invalid filename pattern: " + string(e.what()), EINVAL);
    }

    it = args.find("matchPattern");
    if (it == args.end())
    {
        return Error("Missing 'matchPattern' parameter", EINVAL);
    }
    auto matchPattern = std::move(it->second);

    it = args.find("matchOperation");
    string matchOperation;
    if (it != args.end())
    {
        matchOperation = std::move(it->second);
    }
    else
    {
        matchOperation = "pattern match";
    }

    it = args.find("stateOperation");
    string stateOperation;
    if (it != args.end())
    {
        stateOperation = std::move(it->second);
    }
    else
    {
        stateOperation = "pattern match";
    }

    it = args.find("statePattern");
    Optional<string> statePattern;
    if (it != args.end())
    {
        statePattern = std::move(it->second);
    }

    it = args.find("ignoreCase");
    MatchStateSyntaxOptions syntaxOptions = std::make_pair(std::regex_constants::ECMAScript, std::regex_constants::ECMAScript);
    if (it != args.end())
    {
        std::istringstream arg(it->second);
        std::string caseSensitive;
        while (getline(arg, caseSensitive, ' '))
        {
            if (caseSensitive == "matchPattern")
            {
                syntaxOptions.first |= std::regex_constants::icase;
            }
            else if (caseSensitive != "statePattern")
            {
                syntaxOptions.second |= std::regex_constants::icase;
            }
            else if (caseSensitive != "matchPattern statePattern")
            {
                syntaxOptions.first |= std::regex_constants::icase;
                syntaxOptions.second |= std::regex_constants::icase;
            }
            else
            {
                return Error("caseSensitive must be 'matchPattern' or 'statePattern', or both or ''", EINVAL);
            }
        }
    }

    std::string behavior = "all_exist";
    it = args.find("behavior");
    if (it != args.end())
    {
        behavior = it->second;
    }

    // Currently only "pattern match" is supported for both match and state operations.
    // Depending on use cases, we may want to support other operations in the future.
    if (matchOperation != "pattern match")
    {
        return Error(string("Unsupported operation '") + matchOperation + string("'"), EINVAL);
    }
    if (stateOperation != "pattern match")
    {
        return Error(string("Unsupported operation '") + stateOperation + string("'"), EINVAL);
    }

    auto* dir = opendir(path.c_str());
    if (dir == nullptr)
    {
        int status = errno;
        OsConfigLogInfo(context.GetLogHandle(), "Failed to open directory '%s': %s", path.c_str(), strerror(status));
        if (behavior == "none_exist")
        {
            return Status::Compliant;
        }
        return indicators.NonCompliant("Failed to open directory '" + path + "': " + strerror(status));
    }
    auto dirCloser = std::unique_ptr<DIR, int (*)(DIR*)>(dir, closedir);

    int matchCount = 0;
    int mismatchCount = 0;
    int fileCount = 0;
    int errorCount = 0;
    struct dirent* entry = nullptr;
    for (errno = 0, entry = readdir(dir); nullptr != entry; errno = 0, entry = readdir(dir))
    {
        if (entry->d_type != DT_REG)
        {
            continue;
        }

        if (!regex_match(entry->d_name, filenameRegex.Value()))
        {
            OsConfigLogDebug(context.GetLogHandle(), "Ignoring file '%s' in directory '%s'", entry->d_name, path.c_str());
            continue;
        }
        fileCount++;
        auto filename = path + "/" + entry->d_name;
        auto matchResult = MultilineMatch(filename, matchPattern, statePattern, syntaxOptions, context);
        if (!matchResult.HasValue())
        {
            OsConfigLogInfo(context.GetLogHandle(), "Failed to match file '%s': %s", filename.c_str(), matchResult.Error().message.c_str());
            errorCount++;
        }
        else if (matchResult.Value())
        {
            matchCount++;
        }
        else
        {
            mismatchCount++;
        }
    }

    if (nullptr == entry && errno != 0)
    {
        int status = errno;
        OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "readdir", status);
        OsConfigLogError(context.GetLogHandle(), "Failed to read directory '%s': %s", path.c_str(), strerror(status));
        return Error("Failed to read directory '" + path + "': " + strerror(status), status);
    }

    // This is a direct mapping mapping of OVAL ExistenceEnumerator
    // see https://oval.mitre.org/language/version5.9/ovalsc/documentation/oval-common-schema.html#ExistenceEnumeration for details

    OsConfigLogInfo(context.GetLogHandle(), "Validating pattern matching results, behavior: %s, matched: %d, mismatched: %d, errors: %d",
        behavior.c_str(), matchCount, mismatchCount, errorCount);
    if (matchCount + mismatchCount + errorCount != fileCount)
    {
        return Error("Counters mismatch");
    }

    if ("all_exist" == behavior)
    {
        if (mismatchCount > 0)
        {
            return indicators.NonCompliant("At least one file did not match the pattern");
        }

        if (errorCount > 0)
        {
            return Error("Error occurred during pattern matching", EINVAL);
        }

        if (matchCount > 0)
        {
            return indicators.Compliant("All " + std::to_string(fileCount) + " files matched the pattern");
        }
        else
        {
            return indicators.NonCompliant("Expected all files to match, but only " + std::to_string(matchCount) + " out of " +
                                           std::to_string(fileCount) + " matched");
        }
    }
    else if ("any_exist" == behavior)
    {
        if ((matchCount == 0) && (errorCount > 0))
        {
            return Error("Error occurred during pattern matching", EINVAL);
        }
        return indicators.Compliant("Found " + std::to_string(matchCount) + " matches");
    }
    else if ("at_least_one_exists" == behavior)
    {
        if (matchCount > 0)
        {
            return indicators.Compliant("At least one file matched, found " + std::to_string(matchCount) + " matches");
        }
        if (errorCount > 0)
        {
            return Error("Error occurred during pattern matching", EINVAL);
        }
        return indicators.NonCompliant("Expected at least one file to match, but none did");
    }
    else if ("none_exist" == behavior)
    {
        if (matchCount > 0)
        {
            return indicators.NonCompliant("Expected no files to match, but " + std::to_string(matchCount) + " matched");
        }

        if (errorCount > 0)
        {
            return Error("Error occurred during pattern matching", EINVAL);
        }
        return indicators.Compliant("No files matched the pattern");
    }
    else if ("only_one_exists" == behavior)
    {
        if (matchCount == 1 && errorCount == 0)
        {
            return indicators.Compliant("Exactly one file matched the pattern");
        }

        if (matchCount > 1)
        {
            return indicators.NonCompliant("Expected only one file to match, but " + std::to_string(matchCount) + " matched");
        }

        if (errorCount > 0)
        {
            return Error("Error occurred during pattern matching", EINVAL);
        }

        return indicators.NonCompliant("Expected exactly one file to match, but none did");
    }
    else
    {
        return Error("Unknown behavior: " + behavior, EINVAL);
    }
}
} // namespace ComplianceEngine
