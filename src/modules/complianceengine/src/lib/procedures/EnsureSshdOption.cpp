// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "StringTools.h"

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <fnmatch.h>
#include <fts.h>
#include <sstream>
#include <string>

namespace ComplianceEngine
{
namespace
{

Result<std::vector<std::string>> GetAllMatches(ContextInterface& context)
{
    std::deque<std::string> configFiles;
    configFiles.push_back("/etc/ssh/sshd_config");
    std::vector<std::string> allMatches;

    while (!configFiles.empty())
    {
        std::string currentFile = configFiles.front();
        configFiles.pop_front();

        auto fileContent = context.GetFileContents(currentFile);
        if (!fileContent.HasValue())
        {
            continue;
        }

        std::istringstream fileStream(fileContent.Value());
        std::string line;

        while (std::getline(fileStream, line))
        {
            line.erase(0, line.find_first_not_of(" \t"));

            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            std::istringstream lineStream(line);
            std::string directive;
            lineStream >> directive;

            std::transform(directive.begin(), directive.end(), directive.begin(), ::tolower);

            if (directive == "include")
            {
                std::string includeFile;
                lineStream >> includeFile;
                if (!includeFile.empty())
                {
                    size_t lastSlash = includeFile.find_last_of('/');
                    if (lastSlash != std::string::npos && includeFile.find('*', lastSlash) != std::string::npos)
                    {
                        std::string directory = includeFile.substr(0, lastSlash);
                        std::string pattern = includeFile.substr(lastSlash + 1);

                        char* paths[] = {const_cast<char*>(directory.c_str()), nullptr};
                        FTS* ftsp = fts_open(paths, FTS_LOGICAL | FTS_NOCHDIR, nullptr);
                        if (ftsp != nullptr)
                        {
                            FTSENT* entry = nullptr;
                            while ((entry = fts_read(ftsp)) != nullptr)
                            {
                                if ((FTS_F == entry->fts_info) && (fnmatch(pattern.c_str(), entry->fts_name, 0) == 0))
                                {
                                    configFiles.push_back(entry->fts_path);
                                }
                            }
                            fts_close(ftsp);
                        }
                    }
                    else
                    {
                        configFiles.push_back(includeFile);
                    }
                }
            }
            else if (directive == "match")
            {
                std::string type, value;
                lineStream >> type >> value;
                std::transform(type.begin(), type.end(), type.begin(), ::tolower);
                std::transform(value.begin(), value.end(), value.begin(), ::tolower);
                if ((type == "address") || (type == "localaddress"))
                {
                    value = value.substr(0, value.find_first_of('/'));
                }
                if ((type == "user") || (type == "group") || (type == "host") || (type == "port") || (type == "address") || (type == "localaddress"))
                {
                    allMatches.push_back(type + "=" + value);
                }
            }
        }
    }

    return allMatches;
}
Result<std::map<std::string, std::string>> GetSshdOptions(ContextInterface& context, const std::string& matchContext = "")
{
    std::string sshdCommand;

    if (!matchContext.empty())
    {
        sshdCommand = "sshd -T -C " + matchContext;
    }
    else
    {
        auto sshdTestOutput = context.ExecuteCommand("sshd -T 2>&1");
        if (!sshdTestOutput.HasValue())
        {
            return Error("Failed to execute sshd -T command: " + sshdTestOutput.Error().message, sshdTestOutput.Error().code);
        }
        if (sshdTestOutput.Value().find("match group") != std::string::npos || sshdTestOutput.Value().find("Match group") != std::string::npos)
        {
            auto hostname = context.ExecuteCommand("hostname");
            if (!hostname.HasValue())
            {
                return Error("Failed to execute hostname command: " + hostname.Error().message, hostname.Error().code);
            }

            auto hostAddress = context.ExecuteCommand("hostname -I | cut -d ' ' -f1");
            if (!hostAddress.HasValue())
            {
                return Error("Failed to get host address: " + hostAddress.Error().message, hostAddress.Error().code);
            }
            auto hostnameStr = hostname.Value();
            auto hostAddrStr = hostAddress.Value();
            hostnameStr.erase(hostnameStr.find_last_not_of(" \n\r\t") + 1);
            hostAddrStr.erase(hostAddrStr.find_last_not_of(" \n\r\t") + 1);

            sshdCommand = "sshd -T -C user=root -C host=" + hostnameStr + " -C addr=" + hostAddrStr;
        }
        else
        {
            sshdCommand = "sshd -T";
        }
    }

    auto output = context.ExecuteCommand(sshdCommand);
    if (!output.HasValue())
    {
        return Error("Failed to execute " + sshdCommand + ": " + output.Error().message, output.Error().code);
    }

    auto configOutput = output.Value();
    std::istringstream configStream(configOutput);
    std::string line;
    std::map<std::string, std::string> options;

    while (std::getline(configStream, line))
    {
        std::istringstream lineStream(line);
        std::string currentOption;

        if (lineStream >> currentOption)
        {
            std::string optionValue;
            std::getline(lineStream, optionValue);
            optionValue.erase(0, optionValue.find_first_not_of(" \t"));
            std::transform(currentOption.begin(), currentOption.end(), currentOption.begin(), ::tolower);
            std::transform(optionValue.begin(), optionValue.end(), optionValue.begin(), ::tolower);
            options[currentOption] = optionValue;
        }
    }
    return options;
}

// Helper that evaluates a single sshd option against the provided operation/value.
// valueRegexes are only used (and must be valid) when op is regex, match, or not_match.
static Result<Status> EvaluateSshdOption(const std::map<std::string, std::string>& sshdConfig, const std::string& option, const std::string& value,
    const std::string& op, const std::vector<regex>& valueRegexes, IndicatorsTree& indicators)
{
    auto itOptions = sshdConfig.find(option);
    if (itOptions == sshdConfig.end())
    {
        // For not_match semantics, absence means the forbidden pattern is not present -> compliant
        if (op == "not_match")
        {
            return indicators.Compliant("Option '" + option + "' not found.");
        }
        return indicators.NonCompliant("Option '" + option + "' not found in SSH daemon configuration");
    }

    auto realValue = itOptions->second;

    if (option == "maxstartups")
    {
        // special case
        int val1 = 0, val2 = 0, val3 = 0, lim1 = 0, lim2 = 0, lim3 = 0;
        std::istringstream valueStream(realValue);
        valueStream >> val1 >> val2 >> val3;
        std::istringstream limitStream(value);
        limitStream >> lim1 >> lim2 >> lim3;

        if ((val1 > lim1) || (val2 > lim2) || (val3 > lim3))
        {
            return indicators.NonCompliant("Option '" + option + "' has value '" + realValue + "' which exceeds limits '" + value + "'");
        }
        return indicators.Compliant("Option '" + option + "' has a value '" + realValue + "' compliant with limits '" + value + "'");
    }

    if (op == "match" || op == "regex") // The only difference is in the valueRegexes preparation
    {
        if (valueRegexes.empty())
        {
            return Error("Internal error: regex not prepared for match op", EINVAL);
        }
        for (const auto& valueRegex : valueRegexes)
        {
            if (regex_search(realValue, valueRegex))
            {
                return indicators.Compliant("Option '" + option + "' has a compliant value '" + realValue + "'");
            }
        }
        return indicators.NonCompliant("Option '" + option + "' has value '" + realValue + "' which does not match required pattern '" + value + "'");
    }
    else if (op == "not_match")
    {
        if (valueRegexes.empty())
        {
            return Error("Internal error: regex not prepared for not_match op", EINVAL);
        }
        for (const auto& valueRegex : valueRegexes)
        {
            if (regex_search(realValue, valueRegex))
            {
                return indicators.NonCompliant("Option '" + option + "' has value '" + realValue + "' which matches forbidden pattern '" + value + "'");
            }
        }
        return indicators.Compliant("Option '" + option + "' has a compliant value '" + realValue + "'");
    }
    else if (op == "lt" || op == "le" || op == "gt" || op == "ge")
    {
        auto realIntRes = TryStringToInt(realValue);
        auto wantedIntRes = TryStringToInt(value);
        if (!realIntRes.HasValue() || !wantedIntRes.HasValue())
        {
            return indicators.NonCompliant("Option '" + option + "' has non-numeric value '" + realValue + "' or comparison target '" + value +
                                           "' (cannot apply numeric operation '" + op + "')");
        }
        long long realInt = realIntRes.Value();
        long long wantedInt = wantedIntRes.Value();

        bool pass = false;
        std::string expectation;
        if (op == "lt")
        {
            pass = realInt < wantedInt;
            expectation = "less than";
        }
        else if (op == "le")
        {
            pass = realInt <= wantedInt;
            expectation = "less than or equal to";
        }
        else if (op == "gt")
        {
            pass = realInt > wantedInt;
            expectation = "greater than";
        }
        else if (op == "ge")
        {
            pass = realInt >= wantedInt;
            expectation = "greater than or equal to";
        }

        if (pass)
        {
            return indicators.Compliant("Option '" + option + "' has a compliant numeric value '" + realValue + "' (" + expectation + " '" + value +
                                        "')");
        }
        return indicators.NonCompliant("Option '" + option + "' has numeric value '" + realValue + "' which is not " + expectation + " '" + value + "'");
    }
    else
    {
        return Error("Unsupported op '" + op + "'", EINVAL);
    }
}
} // namespace

AUDIT_FN(EnsureSshdOption, "option:Name of the SSH daemon option, might be a comma-separated list:M",
    "value:Regex, list of regexes, string or integer threshold the option value is evaluated against:M",
    "op:Operation (regex|match|not_match|lt|le|gt|ge) optional, defaults to regex::^(regex|match|not_match|lt|le|gt|ge)$",
    "mode:Mode, one of (regular|all_matches)::^(regular|all_matches)$")
{
    auto log = context.GetLogHandle();

    auto it = args.find("option");
    if (it == args.end())
    {
        return Error("Missing 'option' parameter", EINVAL);
    }

    auto optionList = std::move(it->second);
    std::transform(optionList.begin(), optionList.end(), optionList.begin(), ::tolower);
    std::vector<std::string> options;
    std::istringstream optionsStream(optionList);
    std::string optionName;
    while (std::getline(optionsStream, optionName, ','))
    {
        options.push_back(optionName);
    }

    std::string op = "regex";
    if ((it = args.find("op")) != args.end())
    {
        op = it->second;
        std::transform(op.begin(), op.end(), op.begin(), ::tolower);
    }

    it = args.find("value");
    if (it == args.end())
    {
        return Error("Missing 'value' parameter", EINVAL);
    }
    auto value = std::move(it->second);

    // Pre-compile regexes for regex / match / not_match operations
    std::vector<regex> valueRegexes;
    if (op == "regex")
    {
        try
        {
            valueRegexes.push_back(regex(value));
        }
        catch (const regex_error& e)
        {
            OsConfigLogError(log, "Regex error: %s", e.what());
            return Error("Failed to compile regex '" + value + "' error: " + e.what(), EINVAL);
        }
    }
    if (op == "match" || op == "not_match")
    {
        std::istringstream valueStream(value);
        std::string valuePart;
        while (std::getline(valueStream, valuePart, ','))
        {
            try
            {
                valueRegexes.push_back(regex(valuePart));
            }
            catch (const regex_error& e)
            {
                OsConfigLogError(log, "Regex error: %s", e.what());
                return Error("Failed to compile regex '" + valuePart + "' error: " + e.what(), EINVAL);
            }
        }
    }

    std::vector<std::string> matchModes;
    std::string mode = "regular";
    if ((it = args.find("mode")) != args.end())
    {
        auto modeValue = it->second;
        std::transform(modeValue.begin(), modeValue.end(), modeValue.begin(), ::tolower);
        if (modeValue == "all_matches" || modeValue == "regular")
        {
            mode = modeValue;
        }
        else
        {
            return Error("Invalid 'mode' parameter value '" + modeValue + "'", EINVAL);
        }
    }
    if (mode == "all_matches")
    {
        auto allMatches = GetAllMatches(context);
        if (!allMatches.HasValue())
        {
            return allMatches.Error();
        }
        if (allMatches.Value().empty())
        {
            return indicators.Compliant("No Match blocks in SSH daemon configuration, skipping Match evaluation");
        }
        matchModes = allMatches.Value();
    }
    else
    {
        matchModes.push_back(""); // regular
    }

    for (auto const& matchMode : matchModes)
    {
        auto sshdConfig = GetSshdOptions(context, matchMode);
        if (!sshdConfig.HasValue())
        {
            return indicators.NonCompliant("Failed to get sshd options: " + sshdConfig.Error().message);
        }
        for (auto const& option : options)
        {
            OsConfigLogInfo(log, "Evaluating SSH daemon option '%s' in mode '%s' with op '%s' against value '%s'", option.c_str(),
                matchMode.empty() ? "regular" : matchMode.c_str(), op.c_str(), value.c_str());
            auto result = EvaluateSshdOption(sshdConfig.Value(), option, value, op, valueRegexes, indicators);
            if (!result.HasValue())
            {
                return result.Error();
            }
            if (result.Value() == Status::NonCompliant)
            {
                return result.Value();
            }
        }
    }
    return indicators.Compliant("All options are compliant");
}
} // namespace ComplianceEngine
