// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Optional.h>
#include <Regex.h>
#include <Result.h>
#include <fstream>
namespace compliance
{
namespace
{
// In single pattern mode, we check if the pattern is present in the file.
// The function returns true if the pattern matches any line in the file, false otherwise.
Result<Status> SinglePatternMatchMode(std::ifstream& input, const std::string& matchPattern, std::regex_constants::syntax_option_type syntaxOptions,
    IndicatorsTree& indicators)
{
    try
    {
        int lineNumber = 1;
        auto matchRegex = regex(matchPattern, syntaxOptions);
        std::string line;
        while (std::getline(input, line))
        {
            if (regex_search(line, matchRegex))
            {
                return indicators.Compliant("pattern '" + matchPattern + "' matched line " + std::to_string(lineNumber));
            }

            lineNumber++;
        }
    }
    catch (const regex_error& e)
    {
        OsConfigLogError(nullptr, "Regex error: %s", e.what());
        return Error("Regex error: " + std::string(e.what()), EINVAL);
    }

    return indicators.NonCompliant("pattern '" + matchPattern + "' did not match any line");
}

// In state-pattern match mode, we check each line of the input file if it matches the matchPattern regexp.
// For each line that matches the main pattern, we check if statePattern regexp matches line matched by matchPattern.
// The function returns true if the statePattern regexp matches all the lines that match the matchPattern regexp, false otherwise.
Result<Status> StatePatternMatchMode(std::ifstream& input, const std::string& matchPattern, const std::string& statePattern,
    std::regex_constants::syntax_option_type syntaxOptions, IndicatorsTree& indicators)
{
    try
    {
        int lineNumber = 1;
        auto matchRegex = regex(matchPattern, syntaxOptions);
        auto stateRegex = regex(statePattern, syntaxOptions);
        std::string line;
        while (std::getline(input, line))
        {
            if (!regex_search(line, matchRegex))
            {
                lineNumber++;
                continue;
            }

            if (!regex_search(line, stateRegex))
            {
                return indicators.NonCompliant("state pattern '" + statePattern + "' not found in line " + std::to_string(lineNumber));
            }

            lineNumber++;
        }
    }
    catch (const regex_error& e)
    {
        OsConfigLogError(nullptr, "Regex error: %s", e.what());
        return Error("Regex error: " + std::string(e.what()), EINVAL);
    }

    return indicators.Compliant("state pattern '" + statePattern + "' matched all lines");
}
} // anonymous namespace

AUDIT_FN(FileRegexMatch, "filename:Path to the file to check:M", "matchOperation:Operation to perform on the file contents:M:^pattern match$",
    "matchPattern:The pattern to match against the file contents:M",
    "stateOperation:Operation to perform on each line that matches the 'matchPattern'::^pattern match$",
    "statePattern:The pattern to match against each line that matches the 'statePattern'",
    "caseSensitive:Determine whether the match should be case sensitive, applies to both 'matchPattern' and 'statePattern'::^true|false$")
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
    if (it == args.end())
    {
        return Error("Missing 'matchOperation' parameter", EINVAL);
    }
    auto matchOperation = std::move(it->second);

    it = args.find("stateOperation");
    Optional<std::string> stateOperation;
    if (it != args.end())
    {
        stateOperation = std::move(it->second);
    }

    it = args.find("statePattern");
    Optional<std::string> statePattern;
    if (it != args.end())
    {
        statePattern = std::move(it->second);
    }
    if (stateOperation.HasValue() && !statePattern.HasValue())
    {
        return Error("stateOperation field requires statePattern field", EINVAL);
    }
    if (!stateOperation.HasValue() && statePattern.HasValue())
    {
        return Error("statePattern field requires stateOperation field", EINVAL);
    }

    it = args.find("caseSensitive");
    std::regex_constants::syntax_option_type syntaxOptions = std::regex_constants::extended;
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

    std::ifstream file(path);
    if (!file.is_open())
    {
        return indicators.NonCompliant("Failed to open file: " + path);
    }

    if (matchOperation != "pattern match")
    {
        return Error(std::string("Unsupported operation '") + matchOperation + std::string("'"), EINVAL);
    }

    if (stateOperation.HasValue() && stateOperation.Value() != "pattern match")
    {
        return Error(std::string("Unsupported operation '") + stateOperation.Value() + std::string("'"), EINVAL);
    }

    if (!statePattern.HasValue())
    {
        return SinglePatternMatchMode(file, matchPattern, syntaxOptions, indicators);
    }
    else
    {
        return StatePatternMatchMode(file, matchPattern, statePattern.Value(), syntaxOptions, indicators);
    }
}
} // namespace compliance
