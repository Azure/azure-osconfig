// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Evaluator.h>
#include <Logging.h>
#include <NetworkInterfaceFlag.h>
#include <ProcedureMap.h>
#include <StringTools.h>
#include <cerrno>
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
} // namespace

Result<Status> AuditNetworkInterfaceFlag(const NetworkInterfaceFlagParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    const unsigned long bit = FlagBit(params.flag);

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
        return indicators.Compliant("Flag '" + std::to_string(params.flag) + "' is set on interface(s): " + list);
    }

    return indicators.NonCompliant("No interface has flag '" + std::to_string(params.flag) + "' set");
}

} // namespace ComplianceEngine
