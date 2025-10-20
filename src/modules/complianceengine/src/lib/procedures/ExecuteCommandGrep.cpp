// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ExecuteCommandGrep.h>
#include <ProcedureMap.h> // Used for to_string(enum)
#include <StringTools.h>
#include <set>
#include <string>

namespace ComplianceEngine
{

static const std::set<std::string> allowedCommands = {"nft list ruleset", "nft list chain", "nft list tables", "ip6tables -L -n",
    "ip6tables -L INPUT -v -n", "ip6tables -L OUTPUT -v -n", "iptables -L -n", "iptables -L INPUT -v -n", "iptables -L OUTPUT -v -n", "uname", "ps -ef",
    "ps -eZ", "sestatus", "journalctl", "arch", "grubby --info=ALL", "pam-config --query --pwhistory"};

Result<Status> AuditExecuteCommandGrep(const ExecuteCommandGrepParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.type.HasValue());
    if (allowedCommands.find(params.command) == allowedCommands.end())
    {
        return Error("Command " + params.command + " is not allowed");
    }

    std::string awkStr;
    if (params.awk.HasValue())
    {
        awkStr = EscapeForShell(params.awk.Value());
    }

    std::string fullCommand = params.command;
    if (!awkStr.empty())
    {
        fullCommand += " | awk -S \"" + awkStr + "\" ";
    }
    fullCommand +=
        " | grep -" + std::to_string(params.type.Value()) + " -- \"" + EscapeForShell(params.regex) + "\" || (echo -n 'No match found'; exit 1)";
    Result<std::string> commandOutput = context.ExecuteCommand(fullCommand);
    if (!commandOutput.HasValue())
    {
        return indicators.NonCompliant(commandOutput.Error().message);
    }
    else
    {
        return indicators.Compliant("Output of command '" + params.command + "' matches regex '" + params.regex + "'");
    }
}

} // namespace ComplianceEngine
