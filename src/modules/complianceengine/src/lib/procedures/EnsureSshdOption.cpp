// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <EnsureSshdOption.h>
#include <Regex.h>
#include <sstream>
#include <string>

namespace ComplianceEngine
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

Result<Status> AuditEnsureSshdOption(const EnsureSshdOptionParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto option = params.option;
    std::transform(option.begin(), option.end(), option.begin(), ::tolower);

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
    if (regex_search(realValue, params.value))
    {
        return indicators.Compliant("Option '" + option + "' has a compliant value '" + realValue + "'");
    }
    else
    {
        return indicators.NonCompliant("Option '" + option + "' has value '" + realValue + "' which does not match required pattern");
    }
}

Result<Status> AuditEnsureSshdNoOption(const EnsureSshdNoOptionParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto result = GetSshdOptions(context);
    if (!result.HasValue())
    {
        return indicators.NonCompliant("Failed to execute sshd " + result.Error().message + " (code: " + std::to_string(result.Error().code) + ")");
    }
    auto& sshdConfig = result.Value();

    for (auto optionName : params.options.items)
    {
        std::transform(optionName.begin(), optionName.end(), optionName.begin(), ::tolower);
        auto itConfig = sshdConfig.find(optionName);
        if (itConfig == sshdConfig.end())
        {
            indicators.Compliant("Option '" + optionName + "' not found in SSH daemon configuration");
            continue;
        }

        for (const auto& value : params.values.items)
        {
            if (regex_search(itConfig->second, value))
            {
                return indicators.NonCompliant("Option '" + optionName + "' has a compliant value '" + itConfig->second + "'");
            }
        }
        indicators.Compliant("Option '" + optionName + "' has no compliant value in SSH daemon configuration");
    }
    return Status::Compliant; // All options checked, no non-compliant found
}

} // namespace ComplianceEngine
