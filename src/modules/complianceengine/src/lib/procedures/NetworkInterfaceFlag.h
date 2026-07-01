// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PROCEDURES_NETWORK_INTERFACE_H
#define COMPLIANCEENGINE_PROCEDURES_NETWORK_INTERFACE_H

#include <Evaluator.h>
#include <Optional.h>
#include <string>

namespace ComplianceEngine
{
// Linux network interface flags (the IFF_* bits in <linux/if.h>). The label is the
// canonical flag name as it appears in the rule payload.
enum class InterfaceFlag
{
    /// label: UP
    Up,

    /// label: BROADCAST
    Broadcast,

    /// label: DEBUG
    Debug,

    /// label: LOOPBACK
    Loopback,

    /// label: POINTOPOINT
    PointToPoint,

    /// label: NOTRAILERS
    NoTrailers,

    /// label: RUNNING
    Running,

    /// label: NOARP
    NoArp,

    /// label: PROMISC
    Promisc,

    /// label: ALLMULTI
    AllMulti,

    /// label: MASTER
    Master,

    /// label: SLAVE
    Slave,

    /// label: MULTICAST
    Multicast,

    /// label: PORTSEL
    PortSel,

    /// label: AUTOMEDIA
    AutoMedia,

    /// label: DYNAMIC
    Dynamic,
};

struct NetworkInterfaceFlagParams
{
    /// Network interface flag to test for (e.g. PROMISC, UP, LOOPBACK)
    InterfaceFlag flag;

    /// Optional interface name to restrict the check to; default: all interfaces
    Optional<std::string> interfaceName;
};

// Audit is Compliant when at least one (matching) interface has the given flag set,
// and NonCompliant otherwise. It is read-only: it enumerates interfaces and their IFF_*
// flags through ContextInterface::GetNetworkInterfaces() (backed by getifaddrs()); it never
// shells out or reads procfs/sysfs directly. A successful enumeration that yields no matching
// interface (no interfaces at all, or a named interface that is absent) is a determinate
// NonCompliant result -- no matching interface carries the flag. NotApplicable is returned
// only when interfaces cannot be enumerated at all (e.g. the netlink socket getifaddrs()
// relies on is unavailable in a restricted sandbox), which is indeterminate. Typically
// composed under `not` to assert a flag (e.g. PROMISC) is NOT set on any interface.
Result<Status> AuditNetworkInterfaceFlag(const NetworkInterfaceFlagParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_NETWORK_INTERFACE_H
