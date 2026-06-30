// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"

#include <NetworkInterface.h>
#include <cerrno>
#include <gtest/gtest.h>
#include <string>

using ComplianceEngine::AuditNetworkInterfaceFlag;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::InterfaceFlag;
using ComplianceEngine::NetworkInterfaceFlagParams;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ::testing::Return;

class NetworkInterfaceTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree indicators;

    // /proc/net/dev lists every registered interface; the first two lines are headers
    // (no ':'), data lines are "  <name>: <stats...>".
    const std::string procNetDev =
        "Inter-|   Receive                                                |  Transmit\n"
        " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
        "    lo:  12345     100    0    0    0     0          0         0   12345     100    0    0    0     0       0          0\n"
        "  eth0:  67890     200    0    0    0     0          0         0   67890     200    0    0    0     0       0          0\n";

    void SetUp() override
    {
        indicators.Push("NetworkInterface");
    }
};

TEST_F(NetworkInterfaceTest, AuditPromiscuousInterfaceDetected)
{
    EXPECT_CALL(mContext, GetFileContents("/proc/net/dev")).WillRepeatedly(Return(Result<std::string>(procNetDev)));
    EXPECT_CALL(mContext, GetFileContents("/sys/class/net/lo/flags")).WillRepeatedly(Return(Result<std::string>("0x9\n"))); // UP|LOOPBACK
    EXPECT_CALL(mContext, GetFileContents("/sys/class/net/eth0/flags")).WillRepeatedly(Return(Result<std::string>("0x1103\n"))); // UP|BROADCAST|PROMISC|MULTICAST

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(NetworkInterfaceTest, AuditNoPromiscuousInterface)
{
    EXPECT_CALL(mContext, GetFileContents("/proc/net/dev")).WillRepeatedly(Return(Result<std::string>(procNetDev)));
    EXPECT_CALL(mContext, GetFileContents("/sys/class/net/lo/flags")).WillRepeatedly(Return(Result<std::string>("0x9\n")));
    EXPECT_CALL(mContext, GetFileContents("/sys/class/net/eth0/flags")).WillRepeatedly(Return(Result<std::string>("0x1003\n"))); // UP|BROADCAST|MULTICAST, no PROMISC

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(NetworkInterfaceTest, AuditSpecificInterfaceOnly)
{
    // With interfaceName set, /proc/net/dev is not consulted.
    EXPECT_CALL(mContext, GetFileContents("/proc/net/dev")).Times(0);
    EXPECT_CALL(mContext, GetFileContents("/sys/class/net/eth0/flags")).WillRepeatedly(Return(Result<std::string>("0x100\n"))); // PROMISC

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;
    params.interfaceName = std::string("eth0");

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(NetworkInterfaceTest, AuditProcNetDevReadFailureIsError)
{
    EXPECT_CALL(mContext, GetFileContents("/proc/net/dev")).WillRepeatedly(Return(Result<std::string>(Error("No such file", -1))));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(NetworkInterfaceTest, AuditProcNetDevMissingIsNotApplicable)
{
    // Some restricted containers do not expose /proc/net/dev (no procfs); the rule cannot be assessed.
    EXPECT_CALL(mContext, GetFileContents("/proc/net/dev")).WillRepeatedly(Return(Result<std::string>(Error("No such file", ENOENT))));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NotApplicable);
}

TEST_F(NetworkInterfaceTest, AuditSysfsUnavailableIsNotApplicable)
{
    // Interfaces enumerate, but /sys/class/net/<iface>/flags is not readable for any of them
    // (e.g. sysfs not mounted in the container): the flag state is indeterminate.
    EXPECT_CALL(mContext, GetFileContents("/proc/net/dev")).WillRepeatedly(Return(Result<std::string>(procNetDev)));
    EXPECT_CALL(mContext, GetFileContents("/sys/class/net/lo/flags")).WillRepeatedly(Return(Result<std::string>(Error("No such file", ENOENT))));
    EXPECT_CALL(mContext, GetFileContents("/sys/class/net/eth0/flags")).WillRepeatedly(Return(Result<std::string>(Error("No such file", ENOENT))));

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NotApplicable);
}

TEST_F(NetworkInterfaceTest, AuditMalformedFlagsValueIsSkipped)
{
    // A malformed flags value is logged and skipped; it must not mask the flag being set elsewhere.
    EXPECT_CALL(mContext, GetFileContents("/proc/net/dev")).WillRepeatedly(Return(Result<std::string>(procNetDev)));
    EXPECT_CALL(mContext, GetFileContents("/sys/class/net/lo/flags")).WillRepeatedly(Return(Result<std::string>("not-a-number\n")));
    EXPECT_CALL(mContext, GetFileContents("/sys/class/net/eth0/flags")).WillRepeatedly(Return(Result<std::string>("0x1103\n"))); // PROMISC

    NetworkInterfaceFlagParams params;
    params.flag = InterfaceFlag::Promisc;

    auto result = AuditNetworkInterfaceFlag(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant); // eth0 still detected despite lo being malformed
}
