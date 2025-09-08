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

            std::string directiveLower = directive;
            std::transform(directiveLower.begin(), directiveLower.end(), directiveLower.begin(), ::tolower);

            if (directiveLower == "include")
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
            else if (directiveLower == "match")
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
            options[currentOption] = optionValue;
        }
    }
    return options;
}

// Helper that evaluates a single sshd option against the provided operation/value.
// valueRegexPtr is only used (and must be non-null) when op is match or not_match.
static Result<Status> EvaluateSshdOption(const std::map<std::string, std::string>& sshdConfig, const std::string& option, const std::string& value,
    const std::string& op, const regex& valueRegex, bool useRegex, IndicatorsTree& indicators)
{
    auto itOptions = sshdConfig.find(option);
    if (itOptions == sshdConfig.end())
    {
        return indicators.NonCompliant("Option '" + option + "' not found in SSH daemon configuration");
    }

    auto realValue = itOptions->second; // copy (no modification needed)

    if (op == "match")
    {
        if (!useRegex)
        {
            return Error("Internal error: regex not prepared for match op", EINVAL);
        }
        if (regex_search(realValue, valueRegex))
        {
            return indicators.Compliant("Option '" + option + "' has a compliant value '" + realValue + "'");
        }
        return indicators.NonCompliant("Option '" + option + "' has value '" + realValue + "' which does not match required pattern '" + value + "'");
    }
    else if (op == "not_match")
    {
        if (!useRegex)
        {
            return Error("Internal error: regex not prepared for not_match op", EINVAL);
        }
        if (!regex_search(realValue, valueRegex))
        {
            return indicators.Compliant("Option '" + option + "' value '" + realValue + "' does not match forbidden pattern '" + value + "'");
        }
        return indicators.NonCompliant("Option '" + option + "' has value '" + realValue + "' which matches forbidden pattern '" + value + "'");
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

AUDIT_FN(EnsureSshdOption, "option:Name of the SSH daemon option:M", "value:Regex, string or integer threshold the option value is evaluated against:M",
    "op:Operation (match|not_match|lt|le|gt|ge) optional, defaults to match::^(match|not_match|lt|le|gt|ge)$")
{
    auto log = context.GetLogHandle();

    auto it = args.find("option");
    if (it == args.end())
    {
        return Error("Missing 'option' parameter", EINVAL);
    }
    auto option = std::move(it->second);
    std::transform(option.begin(), option.end(), option.begin(), ::tolower);

    it = args.find("value");
    if (it == args.end())
    {
        return Error("Missing 'value' parameter", EINVAL);
    }
    auto value = std::move(it->second);

    // Operation (default match)
    std::string op = "match";
    if ((it = args.find("op")) != args.end())
    {
        op = it->second;
        std::transform(op.begin(), op.end(), op.begin(), ::tolower);
    }

    // Pre-compile regex only for match / not_match operations
    regex valueRegex;
    bool useRegex = (op == "match" || op == "not_match");
    if (useRegex)
    {
        try
        {
            valueRegex = regex(value);
        }
        catch (const regex_error& e)
        {
            OsConfigLogError(log, "Regex error: %s", e.what());
            return Error("Failed to compile regex '" + value + "' error: " + e.what(), EINVAL);
        }
    }

    auto result = GetSshdOptions(context);
    if (!result.HasValue())
    {
        return result.Error();
    }
    auto& sshdConfig = result.Value();

    return EvaluateSshdOption(sshdConfig, option, value, op, valueRegex, useRegex, indicators);
}

AUDIT_FN(EnsureSshdOptionMatch, "option:Name of the SSH daemon option:M",
    "value:Regex, string or integer threshold the option value is evaluated against:M",
    "op:Operation (match|not_match|lt|le|gt|ge) optional, defaults to match::^(match|not_match|lt|le|gt|ge)$")
{
    auto log = context.GetLogHandle();

    auto it = args.find("option");
    if (it == args.end())
    {
        return Error("Missing 'option' parameter", EINVAL);
    }
    auto option = std::move(it->second);
    std::transform(option.begin(), option.end(), option.begin(), ::tolower);

    it = args.find("value");
    if (it == args.end())
    {
        return Error("Missing 'value' parameter", EINVAL);
    }
    auto value = std::move(it->second);

    std::string op = "match";
    if ((it = args.find("op")) != args.end())
    {
        op = it->second;
        std::transform(op.begin(), op.end(), op.begin(), ::tolower);
    }

    regex valueRegex;
    bool useRegex = (op == "match" || op == "not_match");
    if (useRegex)
    {
        try
        {
            valueRegex = regex(value);
        }
        catch (const regex_error& e)
        {
            OsConfigLogError(log, "Regex error: %s", e.what());
            return Error("Failed to compile regex '" + value + "' error: " + e.what(), EINVAL);
        }
    }
    auto matchModes = GetAllMatches(context);
    if (!matchModes.HasValue())
    {
        return matchModes.Error();
    }
    for (auto const& mode : matchModes.Value())
    {
        auto sshdConfig = GetSshdOptions(context, mode);
        if (!sshdConfig.HasValue())
        {
            return sshdConfig.Error();
        }
        auto result = EvaluateSshdOption(sshdConfig.Value(), option, value, op, valueRegex, useRegex, indicators);
        if (!result.HasValue())
        {
            return result.Error();
        }
        if (result.Value() == Status::NonCompliant)
        {
            return result.Value();
        }
    }
    return indicators.Compliant("All possible match options are compliant");
}

AUDIT_FN(EnsureSshdNoOption, "options:Name of the SSH daemon options, comma separated:M", "values:Comma separated list of regexes:M")
{
    auto log = context.GetLogHandle();

    auto it = args.find("options");
    if (it == args.end())
    {
        return Error("Missing 'options' parameter", EINVAL);
    }
    auto options = std::move(it->second);

    it = args.find("values");
    if (it == args.end())
    {
        return Error("Missing 'values' parameter", EINVAL);
    }
    auto values = std::move(it->second);

    auto result = GetSshdOptions(context);
    if (!result.HasValue())
    {
        return indicators.NonCompliant("Failed to execute sshd " + result.Error().message + " (code: " + std::to_string(result.Error().code) + ")");
    }
    auto& sshdConfig = result.Value();

    std::istringstream optionsStream(options);
    std::string optionName;
    while (std::getline(optionsStream, optionName, ','))
    {
        std::transform(optionName.begin(), optionName.end(), optionName.begin(), ::tolower);
        auto itConfig = sshdConfig.find(optionName);
        if (itConfig == sshdConfig.end())
        {
            indicators.Compliant("Option '" + optionName + "' not found in SSH daemon configuration");
            continue;
        }

        std::istringstream valueStream(values);
        std::string value;
        while (std::getline(valueStream, value, ','))
        {
            regex valueRegex;
            try
            {
                valueRegex = regex(value);
            }
            catch (const regex_error& e)
            {
                OsConfigLogError(log, "Regex error: %s", e.what());
                return Error("Failed to compile regex '" + value + "' error: " + e.what(), EINVAL);
            }
            if (regex_search(itConfig->second, valueRegex))
            {
                return indicators.NonCompliant("Option '" + optionName + "' has a compliant value '" + itConfig->second + "'");
            }
        }
        indicators.Compliant("Option '" + optionName + "' has no compliant value in SSH daemon configuration");
    }
    return Status::Compliant; // All options checked, no non-compliant found
}

} // namespace ComplianceEngine
