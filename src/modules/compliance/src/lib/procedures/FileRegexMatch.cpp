// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <FactExistenceValidator.h>
#include <Optional.h>
#include <Regex.h>
#include <Result.h>
#include <fstream>

namespace compliance
{
using std::ifstream;
using std::string;
using std::regex_constants::syntax_option_type;
using Behavior = FactExistenceValidator::Behavior;
namespace
{
// This function is used to check if the file contents match the given pattern.
// It reads the file line by line and checks each line against the matchPattern.
// If a line matches the matchPattern, it checks if the statePattern (if provided) also matches.
// Based on the result of the matches, fact existence validator is used to determine
// if the criteria is met or unmet based on the behavior specified in the arguments.
Result<Status> MultilineMatch(ifstream& input, const string& matchPattern, const Optional<string>& statePattern, syntax_option_type syntaxOptions,
    Behavior behavior, IndicatorsTree& indicators, ContextInterface& context)
{
    Optional<regex> matchRegex;
    Optional<regex> stateRegex;

    try
    {
        matchRegex = regex(matchPattern, syntaxOptions);
        if (statePattern.HasValue())
        {
            stateRegex = regex(statePattern.Value(), syntaxOptions);
        }
    }
    catch (const regex_error& e)
    {
        OsConfigLogInfo(context.GetLogHandle(), "Regex error: %s", e.what());
        return Error("Regex error: " + string(e.what()), EINVAL);
    }

    FactExistenceValidator validator(behavior);
    int lineNumber = 0;
    string line;
    while (!validator.Done() && getline(input, line))
    {
        lineNumber++;
        OsConfigLogDebug(context.GetLogHandle(), "Matching line %d: %s, pattern: %s", lineNumber, line.c_str(), matchPattern.c_str());
        if (regex_search(line, matchRegex.Value()))
        {
            OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());
            if (stateRegex.HasValue())
            {
                if (regex_search(line, stateRegex.Value()))
                {
                    OsConfigLogDebug(context.GetLogHandle(), "Matched line %d: %s", lineNumber, line.c_str());
                    validator.CriteriaMet();
                    if (validator.Done())
                    {
                        indicators.AddIndicator("state pattern '" + statePattern.Value() + "' matched line " + std::to_string(lineNumber), validator.Result());
                    }
                }
                else
                {
                    OsConfigLogDebug(context.GetLogHandle(), "Did not match line %d: %s", lineNumber, line.c_str());
                    validator.CriteriaUnmet();
                    if (validator.Done())
                    {
                        indicators.AddIndicator("state pattern '" + statePattern.Value() + "' did not match line " + std::to_string(lineNumber),
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
                    indicators.AddIndicator("pattern '" + matchPattern + "' matched line " + std::to_string(lineNumber), validator.Result());
                }
            }
        }
        else
        {
            OsConfigLogDebug(context.GetLogHandle(), "Did not match line %d: %s", lineNumber, line.c_str());
            validator.CriteriaUnmet();
            if (validator.Done())
            {
                indicators.AddIndicator("pattern '" + matchPattern + "' did not match line " + std::to_string(lineNumber), validator.Result());
            }
        }
    }

    validator.Finish();
    return validator.Result();
}
} // anonymous namespace

AUDIT_FN(FileRegexMatch, "filename:Path to the file to check:M", "matchOperation:Operation to perform on the file contents::^pattern match$",
    "matchPattern:The pattern to match against the file contents:M",
    "stateOperation:Operation to perform on each line that matches the 'matchPattern'::^pattern match$",
    "statePattern:The pattern to match against each line that matches the 'statePattern'",
    "caseSensitive:Determine whether the match should be case sensitive, applies to both 'matchPattern' and 'statePattern'::^true|false$",
    "behavior:Determine the function behavior::^(all_exist|any_exist|at_least_one_exists|none_exist)$")
{
    UNUSED(context);
    auto it = args.find("filename");
    if (it == args.end())
    {
        return Error("Missing 'filename' parameter", EINVAL);
    }
    auto path = std::move(it->second);

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

    it = args.find("caseSensitive");
    syntax_option_type syntaxOptions = std::regex_constants::ECMAScript;
    if (it != args.end())
    {
        if (it->second == "false")
        {
            syntaxOptions |= std::regex_constants::icase;
        }
        else if (it->second != "true")
        {
            return Error("caseSensitive must be 'true' or 'false'", EINVAL);
        }
    }

    auto behavior = Behavior::NoneExist;
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

    ifstream file(path);
    if (!file.is_open())
    {
        return indicators.NonCompliant("Failed to open file: " + path);
    }

    // Currently only "pattern match" is supported for both match and state operations.
    if (matchOperation != "pattern match")
    {
        return Error(string("Unsupported operation '") + matchOperation + string("'"), EINVAL);
    }
    if (stateOperation != "pattern match")
    {
        return Error(string("Unsupported operation '") + stateOperation + string("'"), EINVAL);
    }

    return MultilineMatch(file, matchPattern, statePattern, syntaxOptions, behavior, indicators, context);
}
} // namespace compliance
