// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <FactExistenceValidator.h>
#include <Optional.h>
#include <Regex.h>
#include <Result.h>
#include <dirent.h>
#include <fstream>

namespace compliance
{
using std::ifstream;
using std::string;
using std::regex_constants::syntax_option_type;
using Behavior = FactExistenceValidator::Behavior;
namespace
{

// syntax Options for matchPattern and statePattern respectively
using MatchStateSyntaxOptions = std::pair<syntax_option_type, syntax_option_type>;

// This function is used to check if the file contents match the given pattern.
// It reads the file line by line and checks each line against the matchPattern.
// If a line matches the matchPattern, it checks if the statePattern (if provided) also matches.
// Based on the result of the matches, fact existence validator is used to determine
// if the criteria is met or unmet based on the behavior specified in the arguments.
// in case of non existing file and Behavior::NoneExist success is expected
Result<Status> MultilineMatch(const std::string& filename, const string& matchPattern, const Optional<string>& statePattern,
    MatchStateSyntaxOptions syntaxOptions, Behavior behavior, IndicatorsTree& indicators, ContextInterface& context)
{
    Optional<regex> matchRegex;
    Optional<regex> stateRegex;
    FactExistenceValidator validator(behavior);

    ifstream input(filename);
    if (!input.is_open())
    {
        if (behavior == Behavior::NoneExist)
        {
            validator.CriteriaUnmet();
            return validator.Result();
        }
        return Error("Failed to open file: " + filename, ENOENT);
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
    while (!validator.Done() && getline(input, line))
    {
        lineNumber++;
        OsConfigLogDebug(context.GetLogHandle(), "Matching line %d: %s, pattern: %s", lineNumber, line.c_str(), matchPattern.c_str());
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
                    validator.CriteriaMet();
                    if (validator.Done())
                    {
                        indicators.AddIndicator("state pattern '" + statePattern.Value() + "' matched line " + std::to_string(lineNumber) +
                                                    " in file '" + filename + "'",
                            validator.Result());
                    }
                }
                else
                {
                    OsConfigLogDebug(context.GetLogHandle(), "Did not match line %d: %s", lineNumber, line.c_str());
                    validator.CriteriaUnmet();
                    if (validator.Done())
                    {
                        indicators.AddIndicator("state pattern '" + statePattern.Value() + "' did not match line " + std::to_string(lineNumber) +
                                                    " in file '" + filename + "'",
                            validator.Result());
                    }
                }
            }
            else
            {
                OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());
                validator.CriteriaMet();
                if (validator.Done())
                {
                    indicators.AddIndicator("pattern '" + matchPattern + "' matched line " + std::to_string(lineNumber) + " in file '" + filename + "'",
                        validator.Result());
                }
            }
        }
        else
        {
            OsConfigLogDebug(context.GetLogHandle(), "Did not match line %d: %s", lineNumber, line.c_str());
            validator.CriteriaUnmet();
            if (validator.Done())
            {
                indicators.AddIndicator("pattern '" + matchPattern + "' did not match any line in file '" + filename + "'", validator.Result());
            }
        }
    }
    if (!validator.Done())
    {
        auto finishResult = validator.Finish();
        indicators.AddIndicator(finishResult, validator.Result());
    }
    return validator.Result();
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

    auto behavior = Behavior::AllExist;
    it = args.find("behavior");
    if (it != args.end())
    {
        auto result = FactExistenceValidator::MapBehavior(it->second);
        if (!result.HasValue())
        {
            return Error("Invalid behavior value: " + it->second, result.Error().code);
        }

        behavior = result.Value();
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
        if (behavior == Behavior::NoneExist)
        {
            return Status::Compliant;
        }
        return indicators.NonCompliant("Failed to open directory '" + path + "': " + strerror(status));
    }
    auto dirCloser = std::unique_ptr<DIR, int (*)(DIR*)>(dir, closedir);

    auto matchedAnyFilename = false;
    struct dirent* entry = nullptr;
    for (errno = 0, entry = readdir(dir); nullptr != entry; errno = 0, entry = readdir(dir))
    {
        if (entry->d_type != DT_REG)
        {
            continue;
        }

        if (!regex_search(entry->d_name, filenameRegex.Value()))
        {
            OsConfigLogDebug(context.GetLogHandle(), "Ignoring file '%s' in directory '%s'", entry->d_name, path.c_str());
            continue;
        }
        matchedAnyFilename = true;

        auto filename = path + "/" + entry->d_name;
        auto matchResult = MultilineMatch(filename, matchPattern, statePattern, syntaxOptions, behavior, indicators, context);
        if (!matchResult.HasValue())
        {
            OsConfigLogInfo(context.GetLogHandle(), "Failed to match file '%s': %s", filename.c_str(), matchResult.Error().message.c_str());
            return matchResult.Error();
        }

        OsConfigLogDebug(context.GetLogHandle(), "Matched file '%s': %s", filename.c_str(), matchResult.Value() == Status::Compliant ? "Compliant" : "NonCompliant");
        if (matchResult.Value() == Status::NonCompliant)
        {
            return Status::NonCompliant;
        }
    }

    if (nullptr == entry && errno != 0)
    {
        int status = errno;
        OsConfigLogError(context.GetLogHandle(), "Failed to read directory '%s': %s", path.c_str(), strerror(status));
        return Error("Failed to read directory '" + path + "': " + strerror(status), status);
    }

    if (!matchedAnyFilename && (behavior == Behavior::NoneExist))
    {
        return indicators.Compliant("No files matched the filename pattern '" + filenamePattern + "'");
    }

    if (!matchedAnyFilename && (behavior == Behavior::AtLeastOneExists))
    {
        return indicators.NonCompliant("No files matched the filename pattern '" + filenamePattern + "'");
    }

    return Status::Compliant;
}
} // namespace compliance
