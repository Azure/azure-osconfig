// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"

#include <NetworkInterfaceFlag.h>
#include <cerrno>
#include <gtest/gtest.h>
#include <net/if.h>
#include <string>
#include <vector>

using ComplianceEngine::AuditNetworkInterfaceFlag;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::InterfaceFlag;
using ComplianceEngine::InterfaceInfo;
using ComplianceEngine::NetworkInterfaceFlagParams;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ::testing::Return;

namespace
{
// Build an InterfaceInfo from a name and an IFF_* flags bitmask.
InterfaceInfo Iface(const std::string& name, unsigned int flags)
{
    InterfaceInfo info;
    info.name = name;
    info.flags = flags;
    return info;
}
} // namespace

class NetworkInterfaceFlagTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree indicators;

    // A typical enumeration: loopback (UP|LOOPBACK) plus a broadcast ethernet interface.
    const std::vector<InterfaceInfo> typicalInterfaces = {
        Iface("lo", 0x9),      // UP|LOOPBACK
        Iface("eth0", 0x1003), // UP|BROADCAST|MULTICAST
    };

    void SetUp() override
    {
        indicators.Push("NetworkInterface");
    }
};

TEST_F(NetworkInterfaceFlagTest, AuditPromiscuousInterfaceDetected)
{
    std::vector<InterfaceInfo> interfaces = {
        Iface("lo", 0x9),      // UP|LOOPBACK
        Iface("eth0", 0x1103), // UP|BROADCAST|PROMISC|MULTICAST
    };
    EXPECT_CALL(mContext, GetNetworkInterfaces()).WillRepeatedly(Return(Result<std::vector<InterfaceInfo>>(interfaces)));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(NetworkInterfaceFlagTest, AuditNoPromiscuousInterface)
{
    EXPECT_CALL(mContext, GetNetworkInterfaces()).WillRepeatedly(Return(Result<std::vector<InterfaceInfo>>(typicalInterfaces)));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(NetworkInterfaceFlagTest, AuditSpecificInterfaceOnly)
{
    // The seam returns all interfaces; the procedure restricts the assessment to eth0.
    std::vector<InterfaceInfo> interfaces = {
        Iface("lo", 0x9),     // UP|LOOPBACK, no PROMISC
        Iface("eth0", 0x100), // PROMISC
    };
    EXPECT_CALL(mContext, GetNetworkInterfaces()).WillRepeatedly(Return(Result<std::vector<InterfaceInfo>>(interfaces)));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;
    params.interfaceName = std::string("eth0");

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(NetworkInterfaceFlagTest, AuditEnumerationFailureIsError)
{
    // An unexpected enumeration failure (not an "unavailable environment" errno) is surfaced.
    EXPECT_CALL(mContext, GetNetworkInterfaces()).WillRepeatedly(Return(Result<std::vector<InterfaceInfo>>(Error("boom", EIO))));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(NetworkInterfaceFlagTest, AuditEnumerationUnavailableIsNotApplicable)
{
    // getifaddrs() is backed by netlink; where the netlink socket is unavailable (e.g. blocked
    // by a restrictive sandbox) the interface state is indeterminate and the rule does not apply.
    EXPECT_CALL(mContext, GetNetworkInterfaces()).WillRepeatedly(Return(Result<std::vector<InterfaceInfo>>(Error("no netlink", EAFNOSUPPORT))));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NotApplicable);
}

TEST_F(NetworkInterfaceFlagTest, AuditNoInterfacesIsNonCompliant)
{
    // A successful enumeration reporting no interfaces (e.g. an isolated network namespace) is a
    // determinate answer: no interface has the flag, i.e. NonCompliant -- not indeterminate.
    EXPECT_CALL(mContext, GetNetworkInterfaces()).WillRepeatedly(Return(Result<std::vector<InterfaceInfo>>(std::vector<InterfaceInfo>{})));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(NetworkInterfaceFlagTest, AuditNamedInterfaceNotFoundIsNonCompliant)
{
    // The caller named an interface that is not present: no matching interface carries the flag,
    // which is a determinate NonCompliant result.
    EXPECT_CALL(mContext, GetNetworkInterfaces()).WillRepeatedly(Return(Result<std::vector<InterfaceInfo>>(typicalInterfaces)));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;
    params.interfaceName = std::string("wlan0");

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(NetworkInterfaceFlagTest, AuditEnumerationEpermIsNotApplicable)
{
    // EPERM means the caller lacks permission to query netlink (e.g. a restrictive seccomp
    // filter); the interface state is indeterminate so the rule does not apply.
    EXPECT_CALL(mContext, GetNetworkInterfaces()).WillRepeatedly(Return(Result<std::vector<InterfaceInfo>>(Error("permission denied", EPERM))));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NotApplicable);
}

TEST_F(NetworkInterfaceFlagTest, AuditEnumerationEnosysIsNotApplicable)
{
    // ENOSYS means the kernel does not implement the required netlink operation;
    // same indeterminate treatment as EAFNOSUPPORT.
    EXPECT_CALL(mContext, GetNetworkInterfaces()).WillRepeatedly(Return(Result<std::vector<InterfaceInfo>>(Error("not implemented", ENOSYS))));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NotApplicable);
}

TEST_F(NetworkInterfaceFlagTest, AuditMultipleMatchingInterfacesAreCompliant)
{
    // Both interfaces carry the flag; the procedure must aggregate all names and return Compliant.
    std::vector<InterfaceInfo> interfaces = {
        Iface("eth0", IFF_PROMISC),
        Iface("eth1", IFF_PROMISC),
    };
    EXPECT_CALL(mContext, GetNetworkInterfaces()).WillRepeatedly(Return(Result<std::vector<InterfaceInfo>>(interfaces)));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
