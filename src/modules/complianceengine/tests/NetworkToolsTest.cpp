#include <MockContext.h>
#include <NetworkTools.h>
#include <arpa/inet.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;

namespace ComplianceEngine
{

class NetworkToolsTest : public ::testing::Test
{
protected:
    MockContext mockContext;

    std::string CreateSSOutput(const std::vector<std::string>& lines)
    {
        std::string output = "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n";
        for (const auto& line : lines)
        {
            output += line + "\n";
        }
        return output;
    }
    void VerifyOpenPort(const OpenPort& port, int expectedFamily, int expectedType, const std::string& expectedIP, unsigned short expectedPort)
    {
        EXPECT_EQ(port.family, expectedFamily);
        EXPECT_EQ(port.type, expectedType);
        EXPECT_EQ(port.port, expectedPort);

        if (expectedFamily == AF_INET)
        {
            struct in_addr expectedAddr;
            inet_pton(AF_INET, expectedIP.c_str(), &expectedAddr);
            EXPECT_EQ(memcmp(&port.ip4, &expectedAddr, sizeof(expectedAddr)), 0);
        }
        else if (expectedFamily == AF_INET6)
        {
            struct in6_addr expectedAddr;
            inet_pton(AF_INET6, expectedIP.c_str(), &expectedAddr);
            EXPECT_EQ(memcmp(&port.ip6, &expectedAddr, sizeof(expectedAddr)), 0);
        }
    }
};

TEST_F(NetworkToolsTest, GetOpenPorts_ValidTCPPorts_ReturnsCorrectPorts)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128           0.0.0.0:22       0.0.0.0:*      users:((\"sshd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128         127.0.0.1:3306     0.0.0.0:*      users:((\"mysqld\",pid=5678,fd=10))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 2);

    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "0.0.0.0", 22);
    VerifyOpenPort(ports[1], AF_INET, SOCK_STREAM, "127.0.0.1", 3306);
}

TEST_F(NetworkToolsTest, GetOpenPorts_ValidUDPPorts_ReturnsCorrectPorts)
{
    std::vector<std::string> lines = {"udp   UNCONN  0       0             0.0.0.0:53       0.0.0.0:*      users:((\"systemd-resolve\",pid=910,fd=12))",
        "udp   UNCONN  0       0           127.0.0.1:323      0.0.0.0:*      users:((\"chronyd\",pid=1122,fd=5))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 2);

    VerifyOpenPort(ports[0], AF_INET, SOCK_DGRAM, "0.0.0.0", 53);
    VerifyOpenPort(ports[1], AF_INET, SOCK_DGRAM, "127.0.0.1", 323);
}

TEST_F(NetworkToolsTest, GetOpenPorts_IPv6Addresses_ReturnsCorrectPorts)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128              [::]:22          [::]:*      users:((\"sshd\",pid=1234,fd=4))",
        "tcp   LISTEN  0       128         [::1]:3306          [::]:*      users:((\"mysqld\",pid=5678,fd=11))",
        "udp   UNCONN  0       0               [::]:53          [::]:*      users:((\"systemd-resolve\",pid=910,fd=13))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 3);

    VerifyOpenPort(ports[0], AF_INET6, SOCK_STREAM, "::", 22);
    VerifyOpenPort(ports[1], AF_INET6, SOCK_STREAM, "::1", 3306);
    VerifyOpenPort(ports[2], AF_INET6, SOCK_DGRAM, "::", 53);
}

TEST_F(NetworkToolsTest, GetOpenPorts_MixedIPv4AndIPv6_ReturnsAllPorts)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128           0.0.0.0:80       0.0.0.0:*      users:((\"nginx\",pid=2000,fd=6))",
        "tcp   LISTEN  0       128              [::]:80          [::]:*      users:((\"nginx\",pid=2000,fd=7))",
        "udp   UNCONN  0       0           127.0.0.1:53       0.0.0.0:*      users:((\"dnsmasq\",pid=3000,fd=4))",
        "udp   UNCONN  0       0               [::1]:53          [::]:*      users:((\"dnsmasq\",pid=3000,fd=5))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 4);

    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "0.0.0.0", 80);
    VerifyOpenPort(ports[1], AF_INET6, SOCK_STREAM, "::", 80);
    VerifyOpenPort(ports[2], AF_INET, SOCK_DGRAM, "127.0.0.1", 53);
    VerifyOpenPort(ports[3], AF_INET6, SOCK_DGRAM, "::1", 53);
}

TEST_F(NetworkToolsTest, GetOpenPorts_CommandExecutionFails_ReturnsError)
{
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Error("Command not found", 127)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, 127);
    EXPECT_TRUE(result.Error().message.find("Failed to execute ss command") != std::string::npos);
}

TEST_F(NetworkToolsTest, GetOpenPorts_EmptyOutput_ReturnsEmptyVector)
{
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>("")));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value().empty());
}

TEST_F(NetworkToolsTest, GetOpenPorts_OnlyHeaderLine_ReturnsEmptyVector)
{
    std::string output = "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n";
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_TRUE(result.Value().empty());
}

TEST_F(NetworkToolsTest, GetOpenPorts_MalformedLines_SkipsInvalidLines)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128           0.0.0.0:22       0.0.0.0:*      users:((\"sshd\",pid=1234,fd=3))",
        "invalid line with insufficient fields", "", "# This is a comment",
        "tcp   LISTEN  0       128         127.0.0.1:3306     0.0.0.0:*      users:((\"mysqld\",pid=5678,fd=10))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 2);

    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "0.0.0.0", 22);
    VerifyOpenPort(ports[1], AF_INET, SOCK_STREAM, "127.0.0.1", 3306);
}

TEST_F(NetworkToolsTest, GetOpenPorts_UnsupportedProtocols_SkipsUnsupportedProtocols)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128           0.0.0.0:22       0.0.0.0:*      users:((\"sshd\",pid=1234,fd=3))",
        "sctp  LISTEN  0       128           0.0.0.0:9999     0.0.0.0:*      users:((\"sctp_app\",pid=9999,fd=1))",
        "raw   UNCONN  0       0             0.0.0.0:1        0.0.0.0:*      users:((\"ping\",pid=8888,fd=2))",
        "udp   UNCONN  0       0             0.0.0.0:53       0.0.0.0:*      users:((\"dns\",pid=7777,fd=4))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 2);

    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "0.0.0.0", 22);
    VerifyOpenPort(ports[1], AF_INET, SOCK_DGRAM, "0.0.0.0", 53);
}

TEST_F(NetworkToolsTest, GetOpenPorts_InvalidIPAddresses_SkipsInvalidAddresses)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128           0.0.0.0:22       0.0.0.0:*      users:((\"sshd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128        invalid.ip:80      0.0.0.0:*      users:((\"httpd\",pid=2345,fd=5))",
        "tcp   LISTEN  0       128         256.256.256.256:443  0.0.0.0:*   users:((\"nginx\",pid=3456,fd=6))",
        "udp   UNCONN  0       0             0.0.0.0:53       0.0.0.0:*      users:((\"dns\",pid=4567,fd=7))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 2);

    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "0.0.0.0", 22);
    VerifyOpenPort(ports[1], AF_INET, SOCK_DGRAM, "0.0.0.0", 53);
}

TEST_F(NetworkToolsTest, GetOpenPorts_PortsWithoutColon_SkipsInvalidFormat)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128           0.0.0.0:22       0.0.0.0:*      users:((\"sshd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128           0.0.0.0          0.0.0.0:*      users:((\"invalid\",pid=2345,fd=5))",
        "udp   UNCONN  0       0             127.0.0.1:53     0.0.0.0:*      users:((\"dns\",pid=3456,fd=7))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 2);

    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "0.0.0.0", 22);
    VerifyOpenPort(ports[1], AF_INET, SOCK_DGRAM, "127.0.0.1", 53);
}

TEST_F(NetworkToolsTest, GetOpenPorts_HighPortNumbers_HandlesCorrectly)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128           0.0.0.0:65535    0.0.0.0:*      users:((\"app1\",pid=1234,fd=3))",
        "udp   UNCONN  0       0             127.0.0.1:32768  0.0.0.0:*      users:((\"app2\",pid=5678,fd=4))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 2);

    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "0.0.0.0", 65535);
    VerifyOpenPort(ports[1], AF_INET, SOCK_DGRAM, "127.0.0.1", 32768);
}

TEST_F(NetworkToolsTest, GetOpenPorts_RealWorldSSOutput_ParsesCorrectly)
{
    std::string output =
        "Netid  State   Recv-Q Send-Q  Local Address:Port  Peer Address:Port  Process\n"
        "udp    UNCONN  0      0             127.0.0.53:53        0.0.0.0:*     users:((\"systemd-resolve\",pid=910,fd=12))\n"
        "udp    UNCONN  0      0            127.0.0.1:323        0.0.0.0:*     users:((\"chronyd\",pid=1122,fd=5))\n"
        "tcp    LISTEN  0      128           0.0.0.0:22         0.0.0.0:*     users:((\"sshd\",pid=1234,fd=3))\n"
        "tcp    LISTEN  0      128              [::]:22            [::]:*     users:((\"sshd\",pid=1234,fd=4))\n"
        "tcp    LISTEN  0      80            127.0.0.1:3306       0.0.0.0:*     users:((\"mysqld\",pid=5678,fd=10))\n";

    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 5);

    VerifyOpenPort(ports[0], AF_INET, SOCK_DGRAM, "127.0.0.53", 53);
    VerifyOpenPort(ports[1], AF_INET, SOCK_DGRAM, "127.0.0.1", 323);
    VerifyOpenPort(ports[2], AF_INET, SOCK_STREAM, "0.0.0.0", 22);
    VerifyOpenPort(ports[3], AF_INET6, SOCK_STREAM, "::", 22);
    VerifyOpenPort(ports[4], AF_INET, SOCK_STREAM, "127.0.0.1", 3306);
}

} // namespace ComplianceEngine
