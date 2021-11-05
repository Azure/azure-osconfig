// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <cstdio>
#include <gtest/gtest.h>
#include <string>
#include <CommonUtils.h>
#include <Mmi.h>
#include <Networking.h>
#include <Logging.h>

class NetworkingObjectTest : public NetworkingObjectBase
{
public:
    NetworkingObjectTest(unsigned int maxPayloadSizeBytes);
    ~NetworkingObjectTest();
    std::vector <std::string> returnValues;
    unsigned int runCommandCount = 0;
    std::string RunCommand(const char* command);
    bool isTestWriteJsonElement = false;
    int WriteJsonElement(rapidjson::Writer<rapidjson::StringBuffer>* writer, const char* key, const char* value);
};

NetworkingObjectTest::NetworkingObjectTest(unsigned int maxPayloadSizeBytes)
{
    m_maxPayloadSizeBytes = maxPayloadSizeBytes;
}

NetworkingObjectTest::~NetworkingObjectTest() {}

std::string NetworkingObjectTest::RunCommand(const char* command)
{
    UNUSED(command);
    std::string commandResult = "";
    if (runCommandCount < (unsigned int)returnValues.size())
    {
        commandResult = returnValues[runCommandCount++];
    }

    return commandResult;
}

int NetworkingObjectTest::WriteJsonElement(rapidjson::Writer<rapidjson::StringBuffer>* writer, const char* key, const char* value)
{
    int result = MMI_OK;
    if (false == isTestWriteJsonElement)
    {
        if (false == writer->Key(key))
        {
            result = ENODATA;
        }

        if (false == writer->String(value))
        {
            result = ENODATA;
        }
    }
    else
    {
        writer->Key(key);
        writer->String(value);
        // return test error ENODATA indicating errors in writing key and value
        result = ENODATA;
    }

    return result;
}

namespace OSConfig::Platform::Tests
{
    unsigned int g_maxPayloadSizeBytes = 4000;
    const char* g_clientName = "ClientName";

    std::string g_testCommandOutputNames = "docker0\neth0";

    std::string g_testCommandOutputInterfaceTypesNmcli =
    "GENERAL.DEVICE:                         docker0\n"
    "GENERAL.TYPE:                           bridge\n"
    "GENERAL.HWADDR:                         02:42:65:B3:AC:5A\n"
    "GENERAL.MTU:                            1500\n"
    "GENERAL.STATE:                          100 (connected)\n"
    "GENERAL.CONNECTION:                     docker0\n"

    "GENERAL.DEVICE:                         eth0\n"
    "GENERAL.TYPE:                           ethernet\n"
    "GENERAL.HWADDR:                         00:15:5D:26:CF:AB\n"
    "GENERAL.MTU:                            1500\n"
    "GENERAL.STATE:                          100 (connected)\n"
    "GENERAL.CONNECTION:                     Wired connection 1\n";

    std::string g_testCommandOutputInterfaceTypesNetworkctl =
    "1 docker0          bridge             no-carrier  unmanaged\n"
    "2 eth0             ether              no-carrier  configuring\n";

    std::string g_testIpData =
	"[{\"ifindex\":1,"
        "\"ifname\":\"docker0\","
        "\"flags\":[\"BROADCAST\",\"UP\",\"LOWER_UP\"],"
        "\"mtu\":65536,"
        "\"qdisc\":\"noqueue\","
        "\"operstate\":\"UP\","
        "\"group\":\"default\","
        "\"txqlen\":1000,"
        "\"link_type\":\"bridge\","
        "\"address\":\"0a:25:3g:6v:2f:89\","
        "\"broadcast\":\"00:00:00:00:00:00\","
        "\"addr_info\":[{"
            "\"family\":\"inet\","
            "\"local\":\"172.32.233.234\","
            "\"prefixlen\":8,"
            "\"scope\":\"host\","
            "\"dynamic\":true,"
            "\"label\":\"docker0\","
            "\"valid_life_time\":4294967295,"
            "\"preferred_life_time\":4294967295"
        "},{"
            "\"family\":\"inet6\","
            "\"local\":\"::1\","
            "\"prefixlen\":128,"
            "\"scope\":\"host\","
            "\"valid_life_time\":4294967295,"
            "\"preferred_life_time\":4294967295"
        "}]"
    "},{"
        "\"ifindex\":2,"
        "\"ifname\":\"eth0\","
        "\"flags\":[\"BROADCAST\",\"MULTICAST\",\"DOWN\"],"
        "\"mtu\":1500,"
        "\"qdisc\":\"mq\","
        "\"operstate\":\"DOWN\","
        "\"group\":\"default\","
        "\"txqlen\":1000,"
        "\"link_type\":\"ether\","
        "\"address\":\"00:15:5d:26:cf:89\","
        "\"broadcast\":\"ff:ff:ff:ff:ff:ff\","
        "\"addr_info\":[{"
            "\"family\":\"inet\","
            "\"local\":\"172.27.181.213\","
            "\"prefixlen\":20,"
            "\"broadcast\":\"192.168.239.255\","
            "\"scope\":\"global\","
            "\"noprefixroute\":true,"
            "\"label\":\"eth0\","
            "\"valid_life_time\":85902,"
            "\"preferred_life_time\":85902"
        "},{"
            "\"family\":\"inet\","
            "\"local\":\"10.1.1.2\","
            "\"prefixlen\":16,"
            "\"broadcast\":\"192.168.239.255\","
            "\"scope\":\"global\","
            "\"noprefixroute\":true,"
            "\"label\":\"eth0\","
            "\"valid_life_time\":85902,"
            "\"preferred_life_time\":85902"
        "},{"
            "\"family\":\"inet6\","
            "\"local\":\"fe80::5e42:4bf7:dddd:9b0f\","
            "\"prefixlen\":64,"
            "\"scope\":\"link\","
            "\"valid_life_time\":4294967295,"
            "\"preferred_life_time\":4294967295"
        "}]"
    "}]";

    std::string g_testCommandOutputDefaultGateways =
    "default via 172.17.128.1 dev docker0 proto\n"
    " 172.29.64.0/20 dev eth0 proto kernel scope link src 172.29.78.164\n"
    " default via 172.13.145.1 dev eth0 proto";

    std::string g_testCommandOutputDnsServers =
    "Link 2 (docker0)\n"
        "Current Scopes: DNS\n"
    "DefaultRoute setting: yes\n"
        "LLMNR setting: yes\n"
    "MulticastDNS setting: no\n"
    "DNSOverTLS setting: no\n"
        "DNSSEC setting: no\n"
        "DNSSEC supported: no\n"
    "Current DNS Server: 8.8.8.8\n"
            "DNS Servers: 8.8.8.8\n"
                        "172.29.64.1\n"
            "DNS Domain: mshome.net\n"
    "Link 3 (eth0)\n"
        "Current Scopes: DNS\n"
    "DefaultRoute setting: yes\n"
        "LLMNR setting: yes\n"
    "MulticastDNS setting: no\n"
    "DNSOverTLS setting: no\n"
        "DNSSEC setting: no\n"
        "DNSSEC supported: no\n"
    "Current DNS Server: 172.29.64.1\n"
            "DNS Servers: 172.29.64.1\n"
            "DNS Domain: mshome.net\n";

    std::vector<std::string> g_returnValues = {g_testCommandOutputNames, g_testCommandOutputInterfaceTypesNmcli, g_testIpData, g_testCommandOutputDefaultGateways, g_testCommandOutputDnsServers};

    TEST(NetworkingTests, Get_Success)
    {
        const char* payloadExpected =
        "{\"InterfaceTypes\":\"docker0=bridge;eth0=ethernet\","
        "\"MacAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
        "\"IpAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
        "\"SubnetMasks\":\"docker0=8,128;eth0=20,16,64\","
        "\"DefaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
        "\"DnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
        "\"DhcpEnabled\":\"docker0=true;eth0=unknown\","
        "\"Enabled\":\"docker0=true;eth0=false\","
        "\"Connected\":\"docker0=true;eth0=false\"}";

        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;
        int result = testModule.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        delete payload;
    }

    TEST(NetworkingTests, Get_InterfaceTypes)
    {
        std::string testCommandOutputNmcliInterfaceTypesDataMissing =
        "GENERAL.DEVICE:                         docker0\n"
        "GENERAL.TYPE:                           --\n"
        "GENERAL.HWADDR:                         02:42:65:B3:AC:5A\n"
        "GENERAL.MTU:                            1500\n"
        "GENERAL.STATE:                          100 (connected)\n"
        "GENERAL.CONNECTION:                     docker0\n"

        "GENERAL.DEVICE:                         eth0\n"
        "GENERAL.TYPE:                           --\n"
        "GENERAL.HWADDR:                         00:15:5D:26:CF:AB\n"
        "GENERAL.MTU:                            1500\n"
        "GENERAL.STATE:                          100 (connected)\n"
        "GENERAL.CONNECTION:                     Wired connection 1\n";

        const char* payloadNmcliInterfaceTypesDataMissing =
        "{\"InterfaceTypes\":\"\","
        "\"MacAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
        "\"IpAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
        "\"SubnetMasks\":\"docker0=8,128;eth0=20,16,64\","
        "\"DefaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
        "\"DnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
        "\"DhcpEnabled\":\"docker0=true;eth0=unknown\","
        "\"Enabled\":\"docker0=true;eth0=false\","
        "\"Connected\":\"docker0=true;eth0=false\"}";

        const char* payloadNmcliNotInstalled =
        "{\"InterfaceTypes\":\"docker0=bridge;eth0=ether\","
        "\"MacAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
        "\"IpAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
        "\"SubnetMasks\":\"docker0=8,128;eth0=20,16,64\","
        "\"DefaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
        "\"DnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
        "\"DhcpEnabled\":\"docker0=true;eth0=unknown\","
        "\"Enabled\":\"docker0=true;eth0=false\","
        "\"Connected\":\"docker0=true;eth0=false\"}";

        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;
        testModule.returnValues[1] = testCommandOutputNmcliInterfaceTypesDataMissing;
        int result = testModule.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadNmcliInterfaceTypesDataMissing);

        EXPECT_NE(payload, nullptr);
        delete payload;

        testModule.runCommandCount = 0;
        testModule.returnValues = { g_testCommandOutputNames, "", g_testCommandOutputInterfaceTypesNetworkctl, g_testIpData, g_testCommandOutputDefaultGateways, g_testCommandOutputDnsServers };
        result = testModule.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadNmcliNotInstalled);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, Get_DnsServers)
    {
        std::string testCommandOutputNamesDnsServers = "br-1234\ndocker0\nveth\neth0";

        std::string testCommandOutputDnsServers =
        "Link 1 (br-1234)\n"
            "Current Scopes: none\n"
            "LLMNR setting: yes\n"
        "MulticastDNS setting: no\n"
        "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
        "\n"
        "Link 2 (docker0)\n"
            "Current Scopes: DNS\n"
        "DefaultRoute setting: yes\n"
            "LLMNR setting: yes\n"
        "MulticastDNS setting: no\n"
        "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
        "Current DNS Server: 8.8.8.8\n"
                "DNS Servers: 8.8.8.8\n"
                            "fe80::215:5dff:fe26:cf91\n"
                "DNS Domain: mshome.net\n"
        "\n"
        "Link 3 (veth)\n"
            "Current Scopes: none\n"
            "LLMNR setting: yes\n"
        "MulticastDNS setting: no\n"
        "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
        "\n"
        "Link 4 (eth0)\n"
            "Current Scopes: DNS\n"
        "DefaultRoute setting: yes\n"
            "LLMNR setting: yes\n"
        "MulticastDNS setting: no\n"
        "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
        "Current DNS Server: 172.29.64.1\n"
                "DNS Servers: 172.29.64.1\n"
                "DNS Domain: mshome.net\n";

        const char* payloadExpected =
        "{\"InterfaceTypes\":\"docker0=bridge;eth0=ethernet\","
        "\"MacAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
        "\"IpAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
        "\"SubnetMasks\":\"docker0=8,128;eth0=20,16,64\","
        "\"DefaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
        "\"DnsServers\":\"docker0=8.8.8.8,fe80::215:5dff:fe26:cf91;eth0=172.29.64.1\","
        "\"DhcpEnabled\":\"br-1234=unknown;docker0=true;eth0=unknown;veth=unknown\","
        "\"Enabled\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\","
        "\"Connected\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\"}";

        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;
        testModule.returnValues[0] = testCommandOutputNamesDnsServers;
        testModule.returnValues[4] = testCommandOutputDnsServers;

        int result = testModule.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, Get_Success_Multiple_Calls)
    {
        std::string testIpDataDocker0AddedAddress =
        "[{\"ifindex\":1,"
            "\"ifname\":\"docker0\","
            "\"flags\":[\"BROADCAST\",\"UP\",\"LOWER_UP\"],"
            "\"mtu\":65536,"
            "\"qdisc\":\"noqueue\","
            "\"operstate\":\"UP\","
            "\"group\":\"default\","
            "\"txqlen\":1000,"
            "\"link_type\":\"bridge\","
            "\"address\":\"0a:25:3g:6v:2f:89\","
            "\"broadcast\":\"00:00:00:00:00:00\","
            "\"addr_info\":[{"
                "\"family\":\"inet\","
                "\"local\":\"172.32.233.234\","
                "\"prefixlen\":8,"
                "\"scope\":\"host\","
                "\"dynamic\":true,"
                "\"label\":\"docker0\","
                "\"valid_life_time\":4294967295,"
                "\"preferred_life_time\":4294967295"
            "},{"
                "\"family\":\"inet\","
                "\"local\":\"10.1.1.1\","
                "\"prefixlen\":16,"
                "\"scope\":\"host\","
                "\"dynamic\":true,"
                "\"label\":\"docker0\","
                "\"valid_life_time\":4294967295,"
                "\"preferred_life_time\":4294967295"
            "},{"
                "\"family\":\"inet6\","
                "\"local\":\"::1\","
                "\"prefixlen\":128,"
                "\"scope\":\"host\","
                "\"valid_life_time\":4294967295,"
                "\"preferred_life_time\":4294967295"
            "}]"
        "},{"
            "\"ifindex\":2,"
            "\"ifname\":\"eth0\","
            "\"flags\":[\"BROADCAST\",\"MULTICAST\",\"DOWN\"],"
            "\"mtu\":1500,"
            "\"qdisc\":\"mq\","
            "\"operstate\":\"DOWN\","
            "\"group\":\"default\","
            "\"txqlen\":1000,"
            "\"link_type\":\"ether\","
            "\"address\":\"00:15:5d:26:cf:89\","
            "\"broadcast\":\"ff:ff:ff:ff:ff:ff\","
            "\"addr_info\":[{"
                "\"family\":\"inet\","
                "\"local\":\"172.27.181.213\","
                "\"prefixlen\":20,"
                "\"broadcast\":\"192.168.239.255\","
                "\"scope\":\"global\","
                "\"noprefixroute\":true,"
                "\"label\":\"eth0\","
                "\"valid_life_time\":85902,"
                "\"preferred_life_time\":85902"
            "},{"
                "\"family\":\"inet\","
                "\"local\":\"10.1.1.2\","
                "\"prefixlen\":16,"
                "\"broadcast\":\"192.168.239.255\","
                "\"scope\":\"global\","
                "\"noprefixroute\":true,"
                "\"label\":\"eth0\","
                "\"valid_life_time\":85902,"
                "\"preferred_life_time\":85902"
            "},{"
                "\"family\":\"inet6\","
                "\"local\":\"fe80::5e42:4bf7:dddd:9b0f\","
                "\"prefixlen\":64,"
                "\"scope\":\"link\","
                "\"valid_life_time\":4294967295,"
                "\"preferred_life_time\":4294967295"
            "}]"
        "}]";

        const char* payloadExpected =
        "{\"InterfaceTypes\":\"docker0=bridge;eth0=ethernet\","
        "\"MacAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
        "\"IpAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
        "\"SubnetMasks\":\"docker0=8,128;eth0=20,16,64\","
        "\"DefaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
        "\"DnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
        "\"DhcpEnabled\":\"docker0=true;eth0=unknown\","
        "\"Enabled\":\"docker0=true;eth0=false\","
        "\"Connected\":\"docker0=true;eth0=false\"}";

        const char* payloadExpectedAddedAddress =
        "{\"InterfaceTypes\":\"docker0=bridge;eth0=ethernet\","
        "\"MacAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
        "\"IpAddresses\":\"docker0=172.32.233.234,10.1.1.1,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
        "\"SubnetMasks\":\"docker0=8,16,128;eth0=20,16,64\","
        "\"DefaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
        "\"DnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
        "\"DhcpEnabled\":\"docker0=true;eth0=unknown\","
        "\"Enabled\":\"docker0=true;eth0=false\","
        "\"Connected\":\"docker0=true;eth0=false\"}";

        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;
        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        int result = testModule.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        EXPECT_NE(payload, nullptr);
        delete payload;

        testModule.runCommandCount = 0;
        testModule.returnValues[2] = testIpDataDocker0AddedAddress;

        result = testModule.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpectedAddedAddress);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, Get_Success_EmptyData_InterfaceNames)
    {
        std::string testCommandOutputNamesEmpty = "";
        const char* payloadExpected =
        "{\"InterfaceTypes\":\"\","
        "\"MacAddresses\":\"\","
        "\"IpAddresses\":\"\","
        "\"SubnetMasks\":\"\","
        "\"DefaultGateways\":\"\","
        "\"DnsServers\":\"\","
        "\"DhcpEnabled\":\"\","
        "\"Enabled\":\"\","
        "\"Connected\":\"\"}";

        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = {testCommandOutputNamesEmpty};
        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        int result = testModule.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, Get_Success_EmptyData_Eth0)
    {
        std::string testInterfaceTypesDataEth0Empty =
        "GENERAL.DEVICE:                         docker0\n"
        "GENERAL.TYPE:                           bridge\n"
        "GENERAL.HWADDR:                         02:42:65:B3:AC:5A\n"
        "GENERAL.MTU:                            1500\n"
        "GENERAL.STATE:                          100 (connected)\n"
        "GENERAL.CONNECTION:                     docker0\n"
        "GENERAL.DEVICE:                         eth0\n";

        std::string testIpDataEth0Empty =
        "[{\"ifindex\":1,"
            "\"ifname\":\"docker0\","
            "\"flags\":[\"BROADCAST\",\"UP\",\"LOWER_UP\"],"
            "\"mtu\":65536,"
            "\"qdisc\":\"noqueue\","
            "\"operstate\":\"UP\","
            "\"group\":\"default\","
            "\"txqlen\":1000,"
            "\"link_type\":\"bridge\","
            "\"address\":\"0a:25:3g:6v:2f:89\","
            "\"broadcast\":\"00:00:00:00:00:00\","
            "\"addr_info\":[{"
                "\"family\":\"inet\","
                "\"local\":\"172.32.233.234\","
                "\"prefixlen\":8,"
                "\"scope\":\"host\","
                "\"dynamic\":true,"
                "\"label\":\"docker0\","
                "\"valid_life_time\":4294967295,"
                "\"preferred_life_time\":4294967295"
            "},{"
                "\"family\":\"inet6\","
                "\"local\":\"::1\","
                "\"prefixlen\":128,"
                "\"scope\":\"host\","
                "\"valid_life_time\":4294967295,"
                "\"preferred_life_time\":4294967295"
            "}]"
        "}]";

        std::string testCommandOutputDefaultGatewaysEth0Empty =
        "default via 172.17.128.1 dev docker0 proto\n"
        " 172.29.64.0/20 dev eth0 proto kernel scope link src 172.29.78.164";

        std::string testCommandOutputDnsServersEth0Empty =
        "Link 2 (docker0)\n"
            "Current Scopes: DNS\n"
        "DefaultRoute setting: yes\n"
            "LLMNR setting: yes\n"
        "MulticastDNS setting: no\n"
        "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
        "Current DNS Server: 8.8.8.8\n"
                "DNS Servers: 8.8.8.8\n"
                            "172.29.64.1\n"
                "DNS Domain: mshome.net\n"
        "Link 3 (eth0)\n"
            "Current Scopes: DNS\n"
        "DefaultRoute setting: yes\n"
            "LLMNR setting: yes\n"
        "MulticastDNS setting: no\n"
        "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n";

        const char* payloadExpected =
        "{\"InterfaceTypes\":\"docker0=bridge\","
        "\"MacAddresses\":\"docker0=0a:25:3g:6v:2f:89\","
        "\"IpAddresses\":\"docker0=172.32.233.234,::1\","
        "\"SubnetMasks\":\"docker0=8,128\","
        "\"DefaultGateways\":\"docker0=172.17.128.1\","
        "\"DnsServers\":\"docker0=8.8.8.8,172.29.64.1\","
        "\"DhcpEnabled\":\"docker0=true;eth0=unknown\","
        "\"Enabled\":\"docker0=true;eth0=unknown\","
        "\"Connected\":\"docker0=true;eth0=unknown\"}";

        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;
        testModule.returnValues[1] = testInterfaceTypesDataEth0Empty;
        testModule.returnValues[2] = testIpDataEth0Empty;
        testModule.returnValues[3] = testCommandOutputDefaultGatewaysEth0Empty;
        testModule.returnValues[4] = testCommandOutputDnsServersEth0Empty;

        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        int result = testModule.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, GetPayloadSizeLimit)
    {
        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        unsigned int maxPayloadSizeBytes = 260;
        NetworkingObjectTest testModule(maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;
        int result = testModule.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString = std::string(payload, payloadSizeBytes);
        std::string expectedString =
        "{\"InterfaceTypes\":\"..\","
        "\"MacAddresses\":\"docker0=0a:25:3g:..\","
        "\"IpAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10...\","
        "\"SubnetMasks\":\"..\","
        "\"DefaultGateways\":\"docker0..\","
        "\"DnsServers\":\"docker0=8.8.8..\","
        "\"DhcpEnabled\":\"..\","
        "\"Enabled\":\"..\","
        "\"Connected\":\"..\"}";

        EXPECT_STREQ(resultString.c_str(), expectedString.c_str());

        EXPECT_NE(payload, nullptr);
        delete payload;

        maxPayloadSizeBytes = 100;
        NetworkingObjectTest testModule2(maxPayloadSizeBytes);
        testModule2.returnValues = g_returnValues;
        result = testModule2.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        expectedString =
        "{\"InterfaceTypes\":\"..\","
        "\"MacAddresses\":\"..\","
        "\"IpAddresses\":\"..\","
        "\"SubnetMasks\":\"..\","
        "\"DefaultGateways\":\"..\","
        "\"DnsServers\":\"..\","
        "\"DhcpEnabled\":\"..\","
        "\"Enabled\":\"..\","
        "\"Connected\":\"..\"}";

        EXPECT_STREQ(resultString.c_str(), expectedString.c_str());

        EXPECT_NE(payload, nullptr);
        delete payload;

        maxPayloadSizeBytes = 0;
        NetworkingObjectTest testModule3(maxPayloadSizeBytes);
        testModule3.returnValues = g_returnValues;
        result = testModule3.Get(nullptr, nullptr, nullptr, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        expectedString =
        "{\"InterfaceTypes\":\"docker0=bridge;eth0=ethernet\","
        "\"MacAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
        "\"IpAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
        "\"SubnetMasks\":\"docker0=8,128;eth0=20,16,64\","
        "\"DefaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
        "\"DnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
        "\"DhcpEnabled\":\"docker0=true;eth0=unknown\","
        "\"Enabled\":\"docker0=true;eth0=false\","
        "\"Connected\":\"docker0=true;eth0=false\"}";

        EXPECT_STREQ(resultString.c_str(), expectedString.c_str());

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, MmiGetInfo)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        int result = MmiGetInfo(nullptr, &payload, &payloadSizeBytes);
        EXPECT_EQ(result, EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        result = MmiGetInfo(g_clientName, nullptr, &payloadSizeBytes);
        EXPECT_EQ(result, EINVAL);
        EXPECT_EQ(payloadSizeBytes, 0);

        result = MmiGetInfo(g_clientName, &payload, nullptr);
        EXPECT_EQ(result, EINVAL);
        EXPECT_EQ(payload, nullptr);

        result = MmiGetInfo(g_clientName, &payload, &payloadSizeBytes);
        EXPECT_EQ(result, MMI_OK);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, MmiOpen)
    {
        MMI_HANDLE handle = MmiOpen(nullptr, g_maxPayloadSizeBytes);
        EXPECT_EQ(handle, nullptr);

        handle = MmiOpen(g_clientName, g_maxPayloadSizeBytes);
        EXPECT_NE(handle, nullptr);

        NetworkingObject* networking = reinterpret_cast<NetworkingObject*>(handle);

        EXPECT_NE(networking, nullptr);
        delete networking;
    }

    TEST(NetworkingTests, MmiGet)
    {
        const char* payloadExpected =
        "{\"InterfaceTypes\":\"docker0=bridge;eth0=ethernet\","
        "\"MacAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
        "\"IpAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
        "\"SubnetMasks\":\"docker0=8,128;eth0=20,16,64\","
        "\"DefaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
        "\"DnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
        "\"DhcpEnabled\":\"docker0=true;eth0=unknown\","
        "\"Enabled\":\"docker0=true;eth0=false\","
        "\"Connected\":\"docker0=true;eth0=false\"}";

        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;

        MMI_HANDLE clientSession = reinterpret_cast<void*>(&testModule);
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        int result = MmiGet(nullptr, NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);
        EXPECT_EQ(result, EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        const char* componentNameUnknown = "ComponentNameUnknown";
        result = MmiGet(clientSession, componentNameUnknown, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);
        EXPECT_EQ(result, EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        const char* objectNameUnknown = "ObjectNameUnknown";
        result = MmiGet(clientSession, NETWORKING, objectNameUnknown, &payload, &payloadSizeBytes);
        EXPECT_EQ(result, EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        result = MmiGet(clientSession, NETWORKING, NETWORK_CONFIGURATION, nullptr, &payloadSizeBytes);
        EXPECT_EQ(result, EINVAL);
        EXPECT_EQ(payloadSizeBytes, 0);

        result = MmiGet(clientSession, NETWORKING, NETWORK_CONFIGURATION, &payload, nullptr);
        EXPECT_EQ(result, EINVAL);
        EXPECT_EQ(payload, nullptr);

        result = MmiGet(clientSession, NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);
        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }
} // namespace OSConfig::Platform::Tests