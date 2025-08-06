// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <NetworkTools.h>
#include <sstream>

namespace ComplianceEngine
{
bool OpenPort::IsLocal() const
{
    if ("lo" == interface)
    {
        return true;
    }
    if (AF_INET == family)
    {
        return ((ip4.s_addr & htonl(0xff000000)) == htonl(0x7f000000));
    }
    if (AF_INET6 == family)
    {
        return (!memcmp(&ip6, &in6addr_loopback, sizeof(ip6)));
    }
    return false;
}

Result<std::vector<OpenPort>> GetOpenPorts(ContextInterface& context)
{
    auto result = context.ExecuteCommand("ss -ptuln");
    if (!result.HasValue())
    {
        return Error("Failed to execute ss command: " + result.Error().message, result.Error().code);
    }
    std::vector<OpenPort> openPorts;
    std::istringstream stream(result.Value());
    OsConfigLogDebug(context.GetLogHandle(), "ss command output: %s ", result.Value().c_str());
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.empty() || line[0] == '\n' || line[0] == '#')
        {
            continue; // Skip empty lines and comments
        }
        std::istringstream iss(line);
        std::string netid, state, recvq, sendq, local, peer, process;
        if (!(iss >> netid >> state >> recvq >> sendq >> local >> peer >> process))
        {
            continue; // Skip lines that don't match expected format
        }
        OpenPort openPort;
        if (netid == "tcp")
        {
            openPort.type = SOCK_STREAM;
        }
        else if (netid == "udp")
        {
            openPort.type = SOCK_DGRAM;
        }
        else
        {
            OsConfigLogInfo(context.GetLogHandle(), "Unsupported netid: %s", netid.c_str());
            continue;
        }
        size_t pos = local.rfind(':');
        if (pos == std::string::npos)
        {
            OsConfigLogError(context.GetLogHandle(), "Invalid local address format: %s", local.c_str());
            continue;
        }
        try
        {
            openPort.port = static_cast<unsigned short>(std::stoi(local.substr(pos + 1)));
        }
        catch (const std::exception& e)
        {
            OsConfigLogError(context.GetLogHandle(), "Invalid port number: %s", local.substr(pos + 1).c_str());
            continue;
        }

        std::string ip = local.substr(0, pos);
        if (ip == "*")
        {
            ip = "0.0.0.0";
        }

        // Handle IPv6 addresses wrapped in brackets
        if (ip.size() >= 2 && ip[0] == '[' && ip[ip.size() - 1] == ']')
        {
            ip = ip.substr(1, ip.size() - 2);
        }

        pos = ip.find('%');
        if (pos != std::string::npos)
        {
            openPort.interface = ip.substr(pos + 1);
            ip = ip.substr(0, pos);
        }

        int r = inet_pton(AF_INET, ip.c_str(), &openPort.ip4);
        if (r <= 0)
        {
            r = inet_pton(AF_INET6, ip.c_str(), &openPort.ip6);
            if (r <= 0)
            {
                OsConfigLogError(context.GetLogHandle(), "Invalid IP address: %s", ip.c_str());
                continue;
            }
            openPort.family = AF_INET6;
        }
        else
        {
            openPort.family = AF_INET;
        }
        openPorts.push_back(std::move(openPort));
    }
    return openPorts;
}
} // namespace ComplianceEngine
