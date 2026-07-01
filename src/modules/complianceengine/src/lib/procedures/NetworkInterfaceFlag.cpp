// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Evaluator.h>
#include <Logging.h>
#include <NetworkInterfaceFlag.h>
#include <ProcedureMap.h>
#include <StringTools.h>
#include <cerrno>
#include <net/if.h>
#include <string>
#include <vector>

namespace ComplianceEngine
{
namespace
{
// Map an interface flag to its IFF_* bit value (see <net/if.h>).
unsigned int FlagBit(InterfaceFlag flag)
{
    switch (flag)
    {
        case InterfaceFlag::Up:
            return IFF_UP;
        case InterfaceFlag::Broadcast:
            return IFF_BROADCAST;
        case InterfaceFlag::Debug:
            return IFF_DEBUG;
        case InterfaceFlag::Loopback:
            return IFF_LOOPBACK;
        case InterfaceFlag::PointToPoint:
            return IFF_POINTOPOINT;
        case InterfaceFlag::NoTrailers:
            return IFF_NOTRAILERS;
        case InterfaceFlag::Running:
            return IFF_RUNNING;
        case InterfaceFlag::NoArp:
            return IFF_NOARP;
        case InterfaceFlag::Promisc:
            return IFF_PROMISC;
        case InterfaceFlag::AllMulti:
            return IFF_ALLMULTI;
        case InterfaceFlag::Master:
            return IFF_MASTER;
        case InterfaceFlag::Slave:
            return IFF_SLAVE;
        case InterfaceFlag::Multicast:
            return IFF_MULTICAST;
        case InterfaceFlag::PortSel:
            return IFF_PORTSEL;
        case InterfaceFlag::AutoMedia:
            return IFF_AUTOMEDIA;
        case InterfaceFlag::Dynamic:
            return IFF_DYNAMIC;
    }
    return 0;
}
} // namespace

Result<Status> AuditNetworkInterfaceFlag(const NetworkInterfaceFlagParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    const unsigned int bit = FlagBit(params.flag);

    auto interfacesResult = context.GetNetworkInterfaces();
    if (!interfacesResult.HasValue())
    {
        // getifaddrs() is backed by netlink; in restricted sandboxes the netlink socket may
        // be unavailable (address family unsupported, blocked by seccomp, or not implemented).
        // The interface state is then indeterminate, so the rule does not apply rather than
        // being reported as a failure. Other errors are surfaced.
        const int code = interfacesResult.Error().code;
        if (code == EAFNOSUPPORT || code == EPERM || code == ENOSYS)
        {
            OsConfigLogInfo(context.GetLogHandle(), "Network interfaces cannot be enumerated in this environment: %s", interfacesResult.Error().message.c_str());
            return indicators.NotApplicable("Network interfaces cannot be enumerated in this environment: " + interfacesResult.Error().message);
        }
        return Error("Failed to enumerate network interfaces: " + interfacesResult.Error().message, code);
    }

    const std::vector<InterfaceInfo>& allInterfaces = interfacesResult.Value();

    // Scan every interface (rather than stopping at the first match) so the report can
    // name all interfaces carrying the flag; when wrapped in `not` this surfaces every
    // offending interface instead of just one. When interfaceName is set, restrict to it.
    // A successful enumeration that yields no matching interface (empty list, or a named
    // interface that is absent) is a determinate answer: no matching interface has the flag,
    // which is NonCompliant -- the same result as when interfaces exist but none carry it.
    // NotApplicable is reserved for the indeterminate case above where enumeration failed.
    std::vector<std::string> matching;
    for (const auto& iface : allInterfaces)
    {
        if (params.interfaceName.HasValue() && iface.name != params.interfaceName.Value())
        {
            continue;
        }
        if ((iface.flags & bit) != 0)
        {
            matching.push_back(iface.name);
        }
    }

    if (!matching.empty())
    {
        std::string list;
        for (size_t i = 0; i < matching.size(); ++i)
        {
            list += (i == 0 ? "" : ", ") + matching[i];
        }
        OsConfigLogDebug(context.GetLogHandle(), "Flag '%s' is set on interface(s): %s", std::to_string(params.flag).c_str(), list.c_str());
        return indicators.Compliant("Flag '" + std::to_string(params.flag) + "' is set on interface(s): " + list);
    }

    return indicators.NonCompliant("No interface has flag '" + std::to_string(params.flag) + "' set");
}

} // namespace ComplianceEngine
