// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <sstream>
#include <string>

namespace compliance
{
namespace
{
Result<std::map<std::string, std::string>> GetSshdOptions(ContextInterface& context)
{
    auto sshdTestOutput = context.ExecuteCommand("sshd -T 2>&1");
    if (!sshdTestOutput.HasValue())
    {
        return Error("Failed to execute sshd -T command: " + sshdTestOutput.Error().message, sshdTestOutput.Error().code);
    }

    std::string sshdCommand;
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
} // namespace

AUDIT_FN(EnsureSshdOption, "option:Name of the SSH daemon option:M", "value:Regex that the option value has to match:M")
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

    auto result = GetSshdOptions(context);
    if (!result.HasValue())
    {
        return indicators.NonCompliant("Failed to execute sshd " + result.Error().message + " (code: " + std::to_string(result.Error().code) + ")");
    }
    auto& sshdConfig = result.Value();

    auto itOptions = sshdConfig.find(option);
    if (itOptions == sshdConfig.end())
    {
        return indicators.NonCompliant("Option '" + option + "' not found in SSH daemon configuration");
    }

    auto realValue = std::move(itOptions->second);
    if (regex_search(realValue, valueRegex))
    {
        return indicators.Compliant("Option '" + option + "' has a compliant value '" + realValue + "'");
    }
    else
    {
        return indicators.NonCompliant("Option '" + option + "' has value '" + realValue + "' which does not match required pattern '" + value + "'");
    }
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

} // namespace compliance
