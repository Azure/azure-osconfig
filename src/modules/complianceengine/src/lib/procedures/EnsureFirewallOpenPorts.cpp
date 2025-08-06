// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "NetworkTools.h"

#include <CommonUtils.h>
#include <Evaluator.h>
#include <set>

namespace ComplianceEngine
{

namespace
{
enum class UfwStatus
{
    Active,
    Inactive
};
} // anonymous namespace
AUDIT_FN(EnsureIptablesOpenPorts)
{
    UNUSED(args);
    auto result = GetOpenPorts(context);
    if (!result.HasValue())
    {
        return result.Error();
    }
    auto& openPorts = result.Value();
    auto iptResult = context.ExecuteCommand("iptables -L INPUT -v -n");
    if (!iptResult.HasValue())
    {
        return Error("Failed to execute iptables command: " + iptResult.Error().message, iptResult.Error().code);
    }
    auto& iptResultR = iptResult.Value();
    for (const auto& port : openPorts)
    {
        if (!port.IsLocal() && (port.family == AF_INET) && (iptResultR.find("dpt:" + std::to_string(port.port)) == std::string::npos))
        {
            return indicators.NonCompliant("Port " + std::to_string(port.port) + " is open but not listed in iptables");
        }
    }
    return indicators.Compliant("All open ports are listed in iptables");
}

AUDIT_FN(EnsureIp6tablesOpenPorts)
{
    UNUSED(args);
    auto result = GetOpenPorts(context);
    if (!result.HasValue())
    {
        return result.Error();
    }
    auto& openPorts = result.Value();
    auto iptResult = context.ExecuteCommand("ip6tables -L INPUT -v -n");
    if (!iptResult.HasValue())
    {
        return Error("Failed to execute ip6tables command: " + iptResult.Error().message, iptResult.Error().code);
    }
    auto& iptResultR = iptResult.Value();
    for (const auto& port : openPorts)
    {
        if (!port.IsLocal() && (port.family == AF_INET6) && (iptResultR.find("dpt:" + std::to_string(port.port)) == std::string::npos))
        {
            return indicators.NonCompliant("Port " + std::to_string(port.port) + " is open but not listed in iptables");
        }
    }
    return indicators.Compliant("All open ports are listed in iptables");
}

AUDIT_FN(EnsureUfwOpenPorts)
{
    UNUSED(args);
    auto result = GetOpenPorts(context);
    if (!result.HasValue())
    {
        return result.Error();
    }
    auto& openPorts = result.Value();
    auto ufwResult = context.ExecuteCommand("ufw status verbose");
    if (!ufwResult.HasValue())
    {
        return Error("Failed to execute ufw command: " + ufwResult.Error().message, ufwResult.Error().code);
    }
    std::istringstream ufwStream(ufwResult.Value());
    std::string line;
    bool foundSeparator = false;
    Optional<UfwStatus> status;
    while (std::getline(ufwStream, line))
    {
        static const std::string statusPrefix = "Status: ";
        if (0 == line.find(statusPrefix))
        {
            if (statusPrefix.size() == line.find("active", statusPrefix.size()))
            {
                status = UfwStatus::Active;
            }
            else if (statusPrefix.size() == line.find("inactive", statusPrefix.size()))
            {
                status = UfwStatus::Inactive;
            }
            else
            {
                return Error("Invalid output from ufw command, unrecognized status section '" + line + "'", EINVAL);
            }
            continue;
        }

        if (line == "--")
        {
            foundSeparator = true;
            break;
        }
    }

    if (!status.HasValue())
    {
        return Error("Invalid output from ufw command, missing status section", EINVAL);
    }

    if (status.Value() == UfwStatus::Inactive)
    {
        return indicators.NonCompliant("UFW is inactive");
    }

    if (!foundSeparator)
    {
        return Error("Invalid output from ufw command, expected separator '--' not found");
    }
    std::set<short> v4ports;
    std::set<short> v6ports;
    while (std::getline(ufwStream, line))
    {
        if (line.empty() || line[0] == '\n' || line[0] == '#')
        {
            continue;
        }
        // ufw status verbose output is hard to parse, the possibilities are:
        // port/tcp ....
        // port .....
        // dest port/tcp ...
        // dest port
        // And for IPv6 the "From" column at the end will end with '(v6)'...

        std::istringstream iss(line);
        std::string first, second;
        if (!(iss >> first >> second))
        {
            continue; // Skip lines that do not have at least two fields
        }
        int port = 0;
        size_t endpos = 0;
        bool validPort = false;

        // case 1/2: try parsing first field as port
        try
        {
            port = std::stoi(first, &endpos);
            if (endpos == first.length() || first[endpos] == '/' || first[endpos] == ' ')
            {
                validPort = true;
            }
        }
        catch (const std::exception&)
        {
            // first field is not a valid number, continue to try second field
        }

        if (!validPort)
        {
            // case 3/4: try parsing second field as port
            try
            {
                port = std::stoi(second, &endpos);
                if (endpos == second.length() || second[endpos] == '/' || second[endpos] == ' ')
                {
                    validPort = true;
                }
            }
            catch (const std::exception&)
            {
                // neither field is a valid number, skip this line
            }
        }

        if (!validPort)
        {
            continue; // Skip if neither part is a valid port number
        }
        if (port < 1 || port > 65535)
        {
            continue; // Skip invalid port numbers
        }
        if (line.find("(v6)") == line.length() - 4)
        {
            v6ports.insert(port);
        }
        else
        {
            v4ports.insert(port);
        }
    }

    for (const auto& port : openPorts)
    {
        if (!port.IsLocal())
        {
            if ((port.family == AF_INET) && (v4ports.find(port.port) == v4ports.end()))
            {
                return indicators.NonCompliant("Port " + std::to_string(port.port) + " is open but not listed in ufw for ipv4");
            }
            if ((port.family == AF_INET6) && (v6ports.find(port.port) == v6ports.end()))
            {
                return indicators.NonCompliant("Port " + std::to_string(port.port) + " is open but not listed in ufw for ipv6");
            }
        }
    }
    return indicators.Compliant("All open ports are listed in ufw");
}

} // namespace ComplianceEngine
