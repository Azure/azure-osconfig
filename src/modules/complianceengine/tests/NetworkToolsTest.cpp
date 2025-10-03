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

TEST_F(NetworkToolsTest, GetOpenPorts_WildcardAddress_ConvertsToZeros)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128                *:80         0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))",
        "udp   UNCONN  0       0                  *:53         0.0.0.0:*      users:((\"dns\",pid=5678,fd=4))",
        "tcp   LISTEN  0       128                *:443        0.0.0.0:*      users:((\"nginx\",pid=9999,fd=5))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 3);

    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "0.0.0.0", 80);
    VerifyOpenPort(ports[1], AF_INET, SOCK_DGRAM, "0.0.0.0", 53);
    VerifyOpenPort(ports[2], AF_INET, SOCK_STREAM, "0.0.0.0", 443);
}

TEST_F(NetworkToolsTest, GetOpenPorts_InterfaceSpecificIPv4_ParsesInterfaceCorrectly)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128     192.168.1.100%eth0:22      0.0.0.0:*      users:((\"sshd\",pid=1234,fd=3))",
        "udp   UNCONN  0       0       10.0.0.1%wlan0:53         0.0.0.0:*      users:((\"dns\",pid=5678,fd=4))",
        "tcp   LISTEN  0       128     172.16.1.1%docker0:8080   0.0.0.0:*      users:((\"app\",pid=9999,fd=5))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 3);

    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "192.168.1.100", 22);
    EXPECT_EQ(ports[0].interface, "eth0");

    VerifyOpenPort(ports[1], AF_INET, SOCK_DGRAM, "10.0.0.1", 53);
    EXPECT_EQ(ports[1].interface, "wlan0");

    VerifyOpenPort(ports[2], AF_INET, SOCK_STREAM, "172.16.1.1", 8080);
    EXPECT_EQ(ports[2].interface, "docker0");
}

TEST_F(NetworkToolsTest, GetOpenPorts_InterfaceSpecificIPv6_ParsesInterfaceCorrectly)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128     [fe80::1%eth0]:22          [::]:*         users:((\"sshd\",pid=1234,fd=3))",
        "udp   UNCONN  0       0       [2001:db8::1%wlan0]:53     [::]:*         users:((\"dns\",pid=5678,fd=4))",
        "tcp   LISTEN  0       128     [::1%lo]:8080              [::]:*         users:((\"app\",pid=9999,fd=5))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 3);

    VerifyOpenPort(ports[0], AF_INET6, SOCK_STREAM, "fe80::1", 22);
    EXPECT_EQ(ports[0].interface, "eth0");

    VerifyOpenPort(ports[1], AF_INET6, SOCK_DGRAM, "2001:db8::1", 53);
    EXPECT_EQ(ports[1].interface, "wlan0");

    VerifyOpenPort(ports[2], AF_INET6, SOCK_STREAM, "::1", 8080);
    EXPECT_EQ(ports[2].interface, "lo");
}

TEST_F(NetworkToolsTest, GetOpenPorts_MixedWildcardAndInterface_ParsesBoth)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128                *:80           0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128     192.168.1.100%eth0:8080   0.0.0.0:*      users:((\"app\",pid=5678,fd=4))",
        "udp   UNCONN  0       0                  *:53           0.0.0.0:*      users:((\"dns\",pid=9999,fd=5))",
        "udp   UNCONN  0       0       10.0.0.1%wlan0:5353       0.0.0.0:*      users:((\"mdns\",pid=1111,fd=6))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 4);

    // Wildcard addresses should have empty interface
    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "0.0.0.0", 80);
    EXPECT_TRUE(ports[0].interface.empty());

    // Interface-specific addresses should have interface set
    VerifyOpenPort(ports[1], AF_INET, SOCK_STREAM, "192.168.1.100", 8080);
    EXPECT_EQ(ports[1].interface, "eth0");

    VerifyOpenPort(ports[2], AF_INET, SOCK_DGRAM, "0.0.0.0", 53);
    EXPECT_TRUE(ports[2].interface.empty());

    VerifyOpenPort(ports[3], AF_INET, SOCK_DGRAM, "10.0.0.1", 5353);
    EXPECT_EQ(ports[3].interface, "wlan0");
}

TEST_F(NetworkToolsTest, GetOpenPorts_IPv6WildcardWithInterface_ParsesCorrectly)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128              [::]:22           [::]:*         users:((\"sshd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128     [fe80::1%eth0]:8080        [::]:*         users:((\"app\",pid=5678,fd=4))",
        "udp   UNCONN  0       0                [::]:53           [::]:*         users:((\"dns\",pid=9999,fd=5))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 3);

    // IPv6 wildcard
    VerifyOpenPort(ports[0], AF_INET6, SOCK_STREAM, "::", 22);
    EXPECT_TRUE(ports[0].interface.empty());

    // IPv6 link-local with interface
    VerifyOpenPort(ports[1], AF_INET6, SOCK_STREAM, "fe80::1", 8080);
    EXPECT_EQ(ports[1].interface, "eth0");

    // IPv6 wildcard UDP
    VerifyOpenPort(ports[2], AF_INET6, SOCK_DGRAM, "::", 53);
    EXPECT_TRUE(ports[2].interface.empty());
}

TEST_F(NetworkToolsTest, GetOpenPorts_ComplexInterfaceNames_ParsesCorrectly)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128     192.168.1.1%br-docker0:80     0.0.0.0:*      users:((\"httpd\",pid=1234,fd=3))",
        "udp   UNCONN  0       0       10.0.0.1%veth12345ab:53       0.0.0.0:*      users:((\"dns\",pid=5678,fd=4))",
        "tcp   LISTEN  0       128     172.17.0.1%docker_gwbridge:8080  0.0.0.0:*   users:((\"app\",pid=9999,fd=5))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 3);

    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "192.168.1.1", 80);
    EXPECT_EQ(ports[0].interface, "br-docker0");

    VerifyOpenPort(ports[1], AF_INET, SOCK_DGRAM, "10.0.0.1", 53);
    EXPECT_EQ(ports[1].interface, "veth12345ab");

    VerifyOpenPort(ports[2], AF_INET, SOCK_STREAM, "172.17.0.1", 8080);
    EXPECT_EQ(ports[2].interface, "docker_gwbridge");
}

TEST_F(NetworkToolsTest, GetOpenPorts_NoInterfaceSpecified_InterfaceEmpty)
{
    std::vector<std::string> lines = {"tcp   LISTEN  0       128           0.0.0.0:22         0.0.0.0:*      users:((\"sshd\",pid=1234,fd=3))",
        "tcp   LISTEN  0       128         127.0.0.1:3306      0.0.0.0:*      users:((\"mysql\",pid=5678,fd=4))",
        "udp   UNCONN  0       0             ::1:53             [::]:*         users:((\"dns\",pid=9999,fd=5))"};
    std::string output = CreateSSOutput(lines);
    EXPECT_CALL(mockContext, ExecuteCommand("ss -ptuln")).WillOnce(Return(ComplianceEngine::Result<std::string>(output)));

    auto result = GetOpenPorts(mockContext);

    ASSERT_TRUE(result.HasValue());
    auto ports = result.Value();
    ASSERT_EQ(ports.size(), 3);

    // All should have empty interface names
    VerifyOpenPort(ports[0], AF_INET, SOCK_STREAM, "0.0.0.0", 22);
    EXPECT_TRUE(ports[0].interface.empty());

    VerifyOpenPort(ports[1], AF_INET, SOCK_STREAM, "127.0.0.1", 3306);
    EXPECT_TRUE(ports[1].interface.empty());

    VerifyOpenPort(ports[2], AF_INET6, SOCK_DGRAM, "::1", 53);
    EXPECT_TRUE(ports[2].interface.empty());
}

} // namespace ComplianceEngine
