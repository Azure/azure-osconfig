// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <sstream>
#include <string>

namespace compliance
{
AUDIT_FN(EnsureSshdOption, "optionName:Name of the SSH daemon option:M", "optionRegex:Regex that the option value has to match:M",
    "mode:P for positive (option must exists, default), N for negative mode")
{
    auto log = context.GetLogHandle();

    auto it = args.find("optionName");
    if (it == args.end())
    {
        return Error("Missing 'optionName' parameter", EINVAL);
    }
    auto optionName = std::move(it->second);

    it = args.find("optionRegex");
    if (it == args.end())
    {
        return Error("Missing 'optionRegex' parameter", EINVAL);
    }
    auto optionRegex = std::move(it->second);
    regex valueRegex;
    try
    {
        valueRegex = regex(optionRegex);
    }
    catch (const regex_error& e)
    {
        OsConfigLogError(log, "Regex error: %s", e.what());
        return Error("Failed to compile regex '" + optionRegex + "' error: " + e.what(), EINVAL);
    }

    bool positiveMode = true;
    it = args.find("mode");
    if (it != args.end())
    {
        auto mode = std::move(it->second);
        if (mode == "N")
        {
            positiveMode = false;
        }
        else if (mode != "P")
        {
            return Error("Invalid 'mode' parameter, expected 'P' or 'N'", EINVAL);
        }
    }

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
    bool optionFound = false;
    std::string optionValue;

    while (std::getline(configStream, line))
    {
        std::istringstream lineStream(line);
        std::string currentOption;

        if (lineStream >> currentOption)
        {
            if (currentOption == optionName)
            {
                optionFound = true;
                std::getline(lineStream, optionValue);
                optionValue.erase(0, optionValue.find_first_not_of(" \t"));
                break;
            }
        }
    }
    if (positiveMode)
    {
        if (!optionFound)
        {
            return indicators.NonCompliant("Option '" + optionName + "' not found in SSH daemon configuration");
        }

        if (regex_search(optionValue, valueRegex))
        {
            return indicators.Compliant("Option '" + optionName + "' has a compliant value '" + optionValue + "'");
        }
        else
        {
            return indicators.NonCompliant("Option '" + optionName + "' has value '" + optionValue + "' which does not match required pattern '" +
                                           optionRegex + "'");
        }
    }
    else
    {
        if (!optionFound)
        {
            return indicators.Compliant("Option '" + optionName + "' not found in SSH daemon configuration");
        }

        if (regex_search(optionValue, valueRegex))
        {
            return indicators.NonCompliant("Option '" + optionName + "' has value '" + optionValue + "' which matches the pattern '" + optionRegex +
                                           "'");
        }
        else
        {
            return indicators.Compliant("Option '" + optionName + "' has a compliant value '" + optionValue + "'");
        }
    }
}
} // namespace compliance
