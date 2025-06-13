// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <iostream>
#include <set>
#include <string>

namespace ComplianceEngine
{

static std::string EscapeForShell(const std::string& str)
{
    std::string escapedStr;
    for (char c : str)
    {
        switch (c)
        {
            case '\\':
            case '"':
            case '`':
            case '$':
                escapedStr += '\\';
                // fall through
            default:
                escapedStr += c;
        }
    }
    return escapedStr;
}

static const std::set<std::string> allowedCommands = {"nft list ruleset", "nft list chain", "nft list tables", "ip6tables -L -n",
    "ip6tables -L INPUT -v -n", "ip6tables -L OUTPUT -v -n", "iptables -L -n", "iptables -L INPUT -v -n", "iptables -L OUTPUT -v -n", "uname"};

AUDIT_FN(ExecuteCommandGrep, "command:Command to be executed:M", "awk:Awk transformation in the middle, optional", "regex:Regex to be matched:M",
    "type:Type of regex, P for Perl (default) or E for Extended")
{
    auto it = args.find("command");
    if (it == args.end())
    {
        return Error("No command name provided");
    }
    auto command = std::move(it->second);
    if (allowedCommands.find(command) == allowedCommands.end())
    {
        return Error("Command " + command + " is not allowed");
    }

    it = args.find("regex");
    if (it == args.end())
    {
        return Error("No regex provided");
    }
    auto regexStr = std::move(it->second);

    std::string awkStr;
    it = args.find("awk");
    if (it != args.end())
    {
        awkStr = std::move(it->second);
        awkStr = EscapeForShell(awkStr);
    }

    std::string type = "P";
    it = args.find("type");
    if (it != args.end())
    {
        type = std::move(it->second);
    }
    if (("P" != type) && ("E" != type))
    {
        return Error("Invalid regex type, only P(erl) and E(xtended) are allowed");
    }
    regexStr = EscapeForShell(regexStr);

    std::string fullCommand = command;
    if (awkStr != "")
    {
        fullCommand += " | awk \"" + awkStr + "\" ";
    }
    fullCommand += " | grep -" + type + " -- \"" + regexStr + "\" || (echo -n 'No match found'; exit 1)";
    Result<std::string> commandOutput = context.ExecuteCommand(fullCommand);
    if (!commandOutput.HasValue())
    {
        return indicators.NonCompliant(commandOutput.Error().message);
    }
    else
    {
        return indicators.Compliant("Output of command '" + command + "' matches regex '" + regexStr + "'");
    }
}

} // namespace ComplianceEngine
