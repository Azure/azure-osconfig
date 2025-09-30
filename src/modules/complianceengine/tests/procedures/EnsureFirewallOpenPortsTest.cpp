// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"

#include <EnsureFirewallOpenPorts.h>
#include <arpa/inet.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/socket.h>

using ComplianceEngine::AuditEnsureIp6tablesOpenPorts;
using ComplianceEngine::AuditEnsureIptablesOpenPorts;
using ComplianceEngine::AuditEnsureUfwOpenPorts;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ::testing::Return;

class EnsureFirewallOpenPortsTest : public ::testing::Test
{
protected:
    MockContext mockContext;
    IndicatorsTree indicators;

    void SetUp() override
    {
        indicators.Push("EnsureFirewallOpenPorts");
    }

    std::string CreateSSOutput(const std::vector<std::string>& lines)
    {
        std::string output = "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n";
        for (const auto& line : lines)
        {
            output += line + "\n";
        }
        return output;
    }
};

// ===== EnsureIptablesOpenPorts Tests =====

TEST_F(EnsureFirewallOpenPortsTest, IptablesOpenPorts_GetOpenPortsFails_ReturnsError)
{
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Error("Command failed", 1)));

    auto result = AuditEnsureIptablesOpenPorts(indicators, mockContext);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, 1);
}

TEST_F(EnsureFirewallOpenPortsTest, IptablesOpenPorts_IptablesCommandFails_ReturnsError)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))"});

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("iptables -L INPUT -v -n")).WillOnce(Return(Error("iptables: command not found", 127)));

    auto result = AuditEnsureIptablesOpenPorts(indicators, mockContext);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, 127);
}

TEST_F(EnsureFirewallOpenPortsTest, IptablesOpenPorts_NoOpenPorts_ReturnsCompliant)
{
    std::string ssOutput = "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n";
    std::string iptablesOutput = "Chain INPUT (policy ACCEPT 0 packets, 0 bytes)\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("iptables -L INPUT -v -n")).WillOnce(Return(Result<std::string>(iptablesOutput)));

    auto result = AuditEnsureIptablesOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, IptablesOpenPorts_OnlyLocalPorts_ReturnsCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128         127.0.0.1:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128         127.0.0.1:443      0.0.0.0:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string iptablesOutput = "Chain INPUT (policy ACCEPT 0 packets, 0 bytes)\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("iptables -L INPUT -v -n")).WillOnce(Return(Result<std::string>(iptablesOutput)));

    auto result = AuditEnsureIptablesOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, IptablesOpenPorts_AllPortsInIptables_ReturnsCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128           0.0.0.0:443      0.0.0.0:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string iptablesOutput =
        "Chain INPUT (policy ACCEPT 0 packets, 0 bytes)\n"
        "    0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp dpt:80\n"
        "    0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp dpt:443\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("iptables -L INPUT -v -n")).WillOnce(Return(Result<std::string>(iptablesOutput)));

    auto result = AuditEnsureIptablesOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, IptablesOpenPorts_PortNotInIptables_ReturnsNonCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128           0.0.0.0:443      0.0.0.0:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string iptablesOutput =
        "Chain INPUT (policy ACCEPT 0 packets, 0 bytes)\n"
        "    0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp dpt:80\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("iptables -L INPUT -v -n")).WillOnce(Return(Result<std::string>(iptablesOutput)));

    auto result = AuditEnsureIptablesOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewallOpenPortsTest, IptablesOpenPorts_IgnoresIPv6Ports_ReturnsCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128              [::]:80          [::]:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128           0.0.0.0:443      0.0.0.0:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string iptablesOutput =
        "Chain INPUT (policy ACCEPT 0 packets, 0 bytes)\n"
        "    0     0 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0            tcp dpt:443\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("iptables -L INPUT -v -n")).WillOnce(Return(Result<std::string>(iptablesOutput)));

    auto result = AuditEnsureIptablesOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// ===== EnsureIp6tablesOpenPorts Tests =====

TEST_F(EnsureFirewallOpenPortsTest, Ip6tablesOpenPorts_GetOpenPortsFails_ReturnsError)
{
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Error("Command failed", 1)));

    auto result = AuditEnsureIp6tablesOpenPorts(indicators, mockContext);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, 1);
}

TEST_F(EnsureFirewallOpenPortsTest, Ip6tablesOpenPorts_Ip6tablesCommandFails_ReturnsError)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128              [::]:80          [::]:*      users:((\"httpd\",pid=1234,fd=3))"});

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ip6tables -L INPUT -v -n")).WillOnce(Return(Error("ip6tables: command not found", 127)));

    auto result = AuditEnsureIp6tablesOpenPorts(indicators, mockContext);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, 127);
}

TEST_F(EnsureFirewallOpenPortsTest, Ip6tablesOpenPorts_AllIPv6PortsInIp6tables_ReturnsCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128              [::]:80          [::]:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128              [::]:443         [::]:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string ip6tablesOutput =
        "Chain INPUT (policy ACCEPT 0 packets, 0 bytes)\n"
        "    0     0 ACCEPT     tcp      *      *       ::/0                 ::/0                 tcp dpt:80\n"
        "    0     0 ACCEPT     tcp      *      *       ::/0                 ::/0                 tcp dpt:443\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ip6tables -L INPUT -v -n")).WillOnce(Return(Result<std::string>(ip6tablesOutput)));

    auto result = AuditEnsureIp6tablesOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, Ip6tablesOpenPorts_IPv6PortNotInIp6tables_ReturnsNonCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128              [::]:80          [::]:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128              [::]:443         [::]:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string ip6tablesOutput =
        "Chain INPUT (policy ACCEPT 0 packets, 0 bytes)\n"
        "    0     0 ACCEPT     tcp      *      *       ::/0                 ::/0                 tcp dpt:80\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ip6tables -L INPUT -v -n")).WillOnce(Return(Result<std::string>(ip6tablesOutput)));

    auto result = AuditEnsureIp6tablesOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewallOpenPortsTest, Ip6tablesOpenPorts_IgnoresIPv4Ports_ReturnsCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128              [::]:443         [::]:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string ip6tablesOutput =
        "Chain INPUT (policy ACCEPT 0 packets, 0 bytes)\n"
        "    0     0 ACCEPT     tcp      *      *       ::/0                 ::/0                 tcp dpt:443\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ip6tables -L INPUT -v -n")).WillOnce(Return(Result<std::string>(ip6tablesOutput)));

    auto result = AuditEnsureIp6tablesOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// ===== EnsureUfwOpenPorts Tests =====

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_GetOpenPortsFails_ReturnsError)
{
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Error("Command failed", 1)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, 1);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_UfwCommandFails_ReturnsError)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))"});

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Error("ufw: command not found", 127)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, 127);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_NoSeparatorInOutput_ReturnsError)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))"});
    std::string ufwOutput =
        "Status: active\n"
        "Logging: on (low)\n"
        "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
        "New profiles: skip\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_FALSE(result.HasValue());
    EXPECT_TRUE(result.Error().message.find("Invalid") != std::string::npos);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_NoOpenPorts_ReturnsCompliant)
{
    std::string ssOutput = "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n";
    std::string ufwOutput =
        "Status: active\n"
        "Logging: on (low)\n"
        "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
        "New profiles: skip\n"
        "\n"
        "--\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_AllPortsInUfw_ReturnsCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128           0.0.0.0:443      0.0.0.0:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string ufwOutput =
        "Status: active\n"
        "Logging: on (low)\n"
        "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
        "New profiles: skip\n"
        "\n"
        "--\n"
        "80/tcp                     ALLOW IN    Anywhere\n"
        "443/tcp                    ALLOW IN    Anywhere\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_IPv4PortNotInUfw_ReturnsNonCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128           0.0.0.0:443      0.0.0.0:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string ufwOutput =
        "Status: active\n"
        "Logging: on (low)\n"
        "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
        "New profiles: skip\n"
        "\n"
        "--\n"
        "80/tcp                     ALLOW IN    Anywhere\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_IPv6PortsHandledCorrectly_ReturnsCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128              [::]:80          [::]:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128              [::]:443         [::]:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string ufwOutput =
        "Status: active\n"
        "Logging: on (low)\n"
        "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
        "New profiles: skip\n"
        "\n"
        "--\n"
        "80/tcp                     ALLOW IN    Anywhere (v6)\n"
        "443/tcp                    ALLOW IN    Anywhere (v6)\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_IPv6PortNotInUfw_ReturnsNonCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128              [::]:80          [::]:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128              [::]:443         [::]:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string ufwOutput =
        "Status: active\n"
        "Logging: on (low)\n"
        "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
        "New profiles: skip\n"
        "\n"
        "--\n"
        "80/tcp                     ALLOW IN    Anywhere (v6)\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_PortWithoutProtocol_ParsedCorrectly)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:22       0.0.0.0:*      users:((\"sshd\",pid=1234,fd=3))"});
    std::string ufwOutput =
        "Status: active\n"
        "Logging: on (low)\n"
        "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
        "New profiles: skip\n"
        "\n"
        "--\n"
        "22                         ALLOW IN    Anywhere\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_DestinationPortFormat_ParsedCorrectly)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:3306     0.0.0.0:*      users:((\"mysqld\",pid=1234,fd=3))"});
    std::string ufwOutput =
        "Status: active\n"
        "Logging: on (low)\n"
        "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
        "New profiles: skip\n"
        "\n"
        "--\n"
        "192.168.1.100 3306/tcp    ALLOW IN    192.168.1.0/24\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_OnlyLocalPorts_ReturnsCompliant)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128         127.0.0.1:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128              [::1]:443         [::]:*      users:((\"httpd\",pid=1234,fd=4))"});
    std::string ufwOutput =
        "Status: active\n"
        "Logging: on (low)\n"
        "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
        "New profiles: skip\n"
        "\n"
        "--\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_MalformedUfwLines_SkippedGracefully)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))"});
    std::string ufwOutput =
        "Status: active\n"
        "Logging: on (low)\n"
        "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
        "New profiles: skip\n"
        "\n"
        "--\n"
        "invalid line\n"
        "80/tcp                     ALLOW IN    Anywhere\n"
        "another invalid line\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_UFW_Inactive)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))"});
    std::string ufwOutput = "Status: inactive\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewallOpenPortsTest, UfwOpenPorts_UFW_InvalidStatus)
{
    std::string ssOutput = CreateSSOutput({"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))"});
    std::string ufwOutput = "Status: ?\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(Result<std::string>(ssOutput)));
    EXPECT_CALL(mockContext, ExecuteCommand("ufw status verbose")).WillOnce(Return(Result<std::string>(ufwOutput)));

    auto result = AuditEnsureUfwOpenPorts(indicators, mockContext);
    ASSERT_FALSE(result.HasValue());
}
