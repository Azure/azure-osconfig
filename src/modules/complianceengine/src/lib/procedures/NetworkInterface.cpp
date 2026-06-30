// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Evaluator.h>
#include <Logging.h>
#include <NetworkInterface.h>
#include <ProcedureMap.h>
#include <StringTools.h>
#include <cerrno>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

namespace ComplianceEngine
{
namespace
{
// Map an interface flag to its IFF_* bit value (see <linux/if.h>).
unsigned long FlagBit(InterfaceFlag flag)
{
    switch (flag)
    {
        case InterfaceFlag::Up:
            return 0x1;
        case InterfaceFlag::Broadcast:
            return 0x2;
        case InterfaceFlag::Debug:
            return 0x4;
        case InterfaceFlag::Loopback:
            return 0x8;
        case InterfaceFlag::PointToPoint:
            return 0x10;
        case InterfaceFlag::NoTrailers:
            return 0x20;
        case InterfaceFlag::Running:
            return 0x40;
        case InterfaceFlag::NoArp:
            return 0x80;
        case InterfaceFlag::Promisc:
            return 0x100;
        case InterfaceFlag::AllMulti:
            return 0x200;
        case InterfaceFlag::Master:
            return 0x400;
        case InterfaceFlag::Slave:
            return 0x800;
        case InterfaceFlag::Multicast:
            return 0x1000;
        case InterfaceFlag::PortSel:
            return 0x2000;
        case InterfaceFlag::AutoMedia:
            return 0x4000;
        case InterfaceFlag::Dynamic:
            return 0x8000;
    }
    return 0;
}

// Extract interface names from /proc/net/dev (the token before ':' on each data line).
std::vector<std::string> ParseInterfaceNames(const std::string& procNetDev)
{
    std::vector<std::string> names;
    std::istringstream stream(procNetDev);
    std::string line;
    while (std::getline(stream, line))
    {
        auto colon = line.find(':');
        if (colon == std::string::npos)
        {
            continue; // header lines have no ':'
        }
        std::string name = TrimWhiteSpaces(line.substr(0, colon));
        if (name.empty())
        {
            continue;
        }
        names.push_back(name);
    }
    return names;
}
} // namespace

Result<Status> AuditNetworkInterfaceFlag(const NetworkInterfaceFlagParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    const unsigned long bit = FlagBit(params.flag);

    std::vector<std::string> interfaces;
    if (params.interfaceName.HasValue())
    {
        interfaces.push_back(params.interfaceName.Value());
    }
    else
    {
        auto procNetDev = context.GetFileContents("/proc/net/dev");
        if (!procNetDev.HasValue())
        {
            // /proc/net/dev is absent in some restricted container environments (e.g. no
            // procfs mounted). Without the interface list there is nothing to assess, so
            // the rule does not apply here rather than being reported as a failure.
            if (procNetDev.Error().code == ENOENT)
            {
                return indicators.NotApplicable("/proc/net/dev is not available; cannot enumerate network interfaces");
            }
            return Error("Failed to read /proc/net/dev: " + procNetDev.Error().message, procNetDev.Error().code);
        }
        interfaces = ParseInterfaceNames(procNetDev.Value());
    }

    // Scan every interface (rather than stopping at the first match) so the report can
    // name all interfaces carrying the flag; when wrapped in `not` this surfaces every
    // offending interface instead of just one.
    bool anyFlagsReadable = false;
    std::vector<std::string> matching;
    for (const auto& iface : interfaces)
    {
        auto flagsContents = context.GetFileContents("/sys/class/net/" + iface + "/flags");
        if (!flagsContents.HasValue())
        {
            // The interface may have disappeared between enumeration and read; skip it.
            continue;
        }
        anyFlagsReadable = true;

        const std::string trimmed = TrimWhiteSpaces(flagsContents.Value());
        unsigned long flags = 0;
        try
        {
            // The flags file holds a hex IFF_* bitmask, e.g. "0x1003".
            flags = std::stoul(trimmed, nullptr, 16);
        }
        catch (const std::exception& e)
        {
            // A malformed flags file is not expected. Log it and keep scanning the other
            // interfaces so one bad entry cannot mask the flag being set elsewhere.
            OsConfigLogError(context.GetLogHandle(), "NetworkInterface: unparseable flags value '%s' for interface '%s': %s", trimmed.c_str(),
                iface.c_str(), e.what());
            continue;
        }
        if ((flags & bit) != 0)
        {
            matching.push_back(iface);
        }
    }

    if (!interfaces.empty() && !anyFlagsReadable)
    {
        // We have interface names but cannot read /sys/class/net/<iface>/flags for any of
        // them (e.g. sysfs is not mounted in this container, or a named interface is
        // absent). The flag state is indeterminate, so the rule does not apply.
        return indicators.NotApplicable("/sys/class/net/<iface>/flags is not available; cannot read interface flags");
    }

    if (!matching.empty())
    {
        std::string list;
        for (size_t i = 0; i < matching.size(); ++i)
        {
            list += (i == 0 ? "" : ", ") + matching[i];
        }
        return indicators.Compliant("Flag '" + std::to_string(params.flag) + "' is set on interface(s): " + list);
    }

    return indicators.NonCompliant("No interface has flag '" + std::to_string(params.flag) + "' set");
}

} // namespace ComplianceEngine
