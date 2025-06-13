// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"
#include "NetworkTools.h"
#include "ProcedureMap.h"

#include <arpa/inet.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/socket.h>

using ComplianceEngine::AuditEnsureMTAsLocalOnly;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::OpenPort;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ::testing::Return;

class EnsureMTAsLocalOnlyTest : public ::testing::Test
{
protected:
    MockContext mockContext;
    IndicatorsTree indicators;

    void SetUp() override
    {
        indicators.Push("EnsureMTAsLocalOnly");
    }

    OpenPort CreateOpenPort(int family, int type, const std::string& ip, unsigned short port)
    {
        OpenPort openPort;
        openPort.family = family;
        openPort.type = type;
        openPort.port = port;

        if (family == AF_INET)
        {
            inet_pton(AF_INET, ip.c_str(), &openPort.ip4);
        }
        else if (family == AF_INET6)
        {
            inet_pton(AF_INET6, ip.c_str(), &openPort.ip6);
        }

        return openPort;
    }
};

TEST_F(EnsureMTAsLocalOnlyTest, GetOpenPortsFails_ReturnsError)
{
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Error("Command failed", 1)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, 1);
}

TEST_F(EnsureMTAsLocalOnlyTest, NoOpenPorts_ReturnsCompliant)
{
    std::vector<OpenPort> emptyPorts;
    std::string output = "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n";
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, OnlyNonMTAPorts_ReturnsCompliant)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "tcp    LISTEN  0      128           0.0.0.0:22         0.0.0.0:*     users:((\"sshd\",pid=1234,fd=3))\n"
        "tcp    LISTEN  0      128           0.0.0.0:80         0.0.0.0:*     users:((\"httpd\",pid=2345,fd=5))\n"
        "udp    UNCONN  0      0             0.0.0.0:53         0.0.0.0:*     users:((\"dns\",pid=3456,fd=7))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, MTAPortsOnLoopback_ReturnsCompliant)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "tcp    LISTEN  0      128         127.0.0.1:25         0.0.0.0:*     users:((\"postfix\",pid=1234,fd=3))\n"
        "tcp    LISTEN  0      128         127.0.0.1:587        0.0.0.0:*     users:((\"postfix\",pid=1234,fd=4))\n"
        "tcp    LISTEN  0      128         127.0.0.1:465        0.0.0.0:*     users:((\"postfix\",pid=1234,fd=5))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, MTAPortsOnIPv6Loopback_ReturnsCompliant)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "tcp    LISTEN  0      128              [::1]:25            [::]:*     users:((\"postfix\",pid=1234,fd=3))\n"
        "tcp    LISTEN  0      128              [::1]:587           [::]:*     users:((\"postfix\",pid=1234,fd=4))\n"
        "tcp    LISTEN  0      128              [::1]:465           [::]:*     users:((\"postfix\",pid=1234,fd=5))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, SMTPPort25OnPublicInterface_ReturnsNonCompliant)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "tcp    LISTEN  0      128           0.0.0.0:25         0.0.0.0:*     users:((\"postfix\",pid=1234,fd=3))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, SMTPPort587OnPublicInterface_ReturnsNonCompliant)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "tcp    LISTEN  0      128           0.0.0.0:587        0.0.0.0:*     users:((\"postfix\",pid=1234,fd=3))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, SMTPSPort465OnPublicInterface_ReturnsNonCompliant)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "tcp    LISTEN  0      128           0.0.0.0:465        0.0.0.0:*     users:((\"postfix\",pid=1234,fd=3))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, MTAPortsOnIPv6PublicInterface_ReturnsNonCompliant)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "tcp    LISTEN  0      128              [::]:25            [::]:*     users:((\"postfix\",pid=1234,fd=3))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, MixedLocalAndPublicPorts_ReturnsNonCompliant)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "tcp    LISTEN  0      128         127.0.0.1:25         0.0.0.0:*     users:((\"postfix\",pid=1234,fd=3))\n"
        "tcp    LISTEN  0      128           0.0.0.0:587        0.0.0.0:*     users:((\"postfix\",pid=1234,fd=4))\n"
        "tcp    LISTEN  0      128         127.0.0.1:465        0.0.0.0:*     users:((\"postfix\",pid=1234,fd=5))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, UDPMTAPortOnPublicInterface_ReturnsNonCompliant)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "udp    UNCONN  0      0             0.0.0.0:25         0.0.0.0:*     users:((\"postfix\",pid=1234,fd=3))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, SpecificPrivateIPAddress_ReturnsNonCompliant)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "tcp    LISTEN  0      128        192.168.1.100:25       0.0.0.0:*     users:((\"postfix\",pid=1234,fd=3))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureMTAsLocalOnlyTest, MultipleNonCompliantPorts_ReturnsNonCompliantForFirst)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "tcp    LISTEN  0      128           0.0.0.0:25         0.0.0.0:*     users:((\"postfix\",pid=1234,fd=3))\n"
        "tcp    LISTEN  0      128           0.0.0.0:587        0.0.0.0:*     users:((\"postfix\",pid=1234,fd=4))\n"
        "tcp    LISTEN  0      128           0.0.0.0:465        0.0.0.0:*     users:((\"postfix\",pid=1234,fd=5))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(output)));

    std::map<std::string, std::string> args;
    auto result = AuditEnsureMTAsLocalOnly(args, indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}
