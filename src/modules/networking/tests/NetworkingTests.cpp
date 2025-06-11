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
    this->m_networkManagementService = NetworkManagementService::Unknown;
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
        "1: docker0: <BROADCAST,UP,LOWER_UP> mtu 65536 qdisc noqueue state UP group default qlen 1000\n"
        "link/bridge 0a:25:3g:6v:2f:89 brd 00:00:00:00:00:00\n"
        "inet 172.32.233.234/8 scope global dynamic noprefixroute docker0 valid_lft forever preferred_lft forever\n"
        "inet6 ::1/128 scope host valid_lft forever preferred_lft forever\n"
        "2: eth0: <BROADCAST,UP> mtu 65536 qdisc noqueue state DOWN group default qlen 1000\n"
        "link/ether 00:15:5d:26:cf:89 brd 00:00:00:00:00:00\n"
        "inet 172.27.181.213/20 scope noprefixroute valid_lft forever preferred_lft forever\n"
        "inet 10.1.1.2/16 scope global eth0\n"
        "inet6 fe80::5e42:4bf7:dddd:9b0f/64 scope link noprefixroute";

    std::string g_testCommandOutputDefaultGateways =
        "default via 172.17.128.1 dev docker0 proto\n"
        "172.29.64.0/20 dev eth0 proto kernel scope link src 172.29.78.164\n"
        "default via 172.13.145.1 dev eth0 proto";

    std::string g_testCommandOutputDnsServers =
        "Link 1 (docker0)\n"
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
        "Link 2 (eth0)\n"
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
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
            "\"dhcpEnabled\":\"docker0=true;eth0=false\","
            "\"enabled\":\"docker0=true;eth0=false\","
            "\"connected\":\"docker0=true;eth0=false\"}";

        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;
        int result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, GetSuccessMangledNames)
    {
        const char* payloadExpected =
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
            "\"dhcpEnabled\":\"docker0=true;eth0=false\","
            "\"enabled\":\"docker0=true;eth0=false\","
            "\"connected\":\"docker0=true;eth0=false\"}";

        std::string testIpDataMangledNames =
            "1: docker0@if123: <BROADCAST,UP,LOWER_UP> mtu 65536 qdisc noqueue state UP group default qlen 1000\n"
            "link/bridge 0a:25:3g:6v:2f:89 brd 00:00:00:00:00:00\n"
            "inet 172.32.233.234/8 scope global dynamic noprefixroute docker0 valid_lft forever preferred_lft forever\n"
            "inet6 ::1/128 scope host valid_lft forever preferred_lft forever\n"
            "2: eth0@if456: <BROADCAST,UP> mtu 65536 qdisc noqueue state DOWN group default qlen 1000\n"
            "link/ether 00:15:5d:26:cf:89 brd 00:00:00:00:00:00\n"
            "inet 172.27.181.213/20 scope noprefixroute valid_lft forever preferred_lft forever\n"
            "inet 10.1.1.2/16 scope global eth0\n"
            "inet6 fe80::5e42:4bf7:dddd:9b0f/64 scope link noprefixroute";

        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);

        testModule.returnValues = g_returnValues;
        testModule.returnValues[2] = testIpDataMangledNames;
        int result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, GetInterfaceTypes)
    {
        const char* payloadInterfaceTypesDataMissing =
            "{\"interfaceTypes\":\"\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
            "\"dhcpEnabled\":\"docker0=true;eth0=false\","
            "\"enabled\":\"docker0=true;eth0=false\","
            "\"connected\":\"docker0=true;eth0=false\"}";

        const char* payloadNetworkManagerEnabled =
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
            "\"dhcpEnabled\":\"docker0=true;eth0=false\","
            "\"enabled\":\"docker0=true;eth0=false\","
            "\"connected\":\"docker0=true;eth0=false\"}";

        const char* payloadSystemdNetworkdEnabled =
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ether\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
            "\"dhcpEnabled\":\"docker0=true;eth0=false\","
            "\"enabled\":\"docker0=true;eth0=false\","
            "\"connected\":\"docker0=true;eth0=false\"}";

        std::string testCommandOutputInterfaceTypesNmcliDataMissing =
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

        std::vector<std::string> returnValuesDataMissing = {g_testCommandOutputNames, testCommandOutputInterfaceTypesNmcliDataMissing, "", g_testIpData, g_testCommandOutputDefaultGateways, g_testCommandOutputDnsServers};

        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = returnValuesDataMissing;
        int result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadInterfaceTypesDataMissing);

        EXPECT_NE(payload, nullptr);
        delete payload;

        std::vector<std::string> returnValuesNetworkManagerEnabled = {g_testCommandOutputNames, g_testCommandOutputInterfaceTypesNmcli, g_testIpData, g_testCommandOutputDefaultGateways, g_testCommandOutputDnsServers};
        testModule.runCommandCount = 0;
        testModule.returnValues = returnValuesNetworkManagerEnabled;
        result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadNetworkManagerEnabled);

        EXPECT_NE(payload, nullptr);
        delete payload;

        std::vector<std::string> returnValuesSystemdNetworkdEnabled = {g_testCommandOutputNames, testCommandOutputInterfaceTypesNmcliDataMissing, g_testCommandOutputInterfaceTypesNetworkctl, g_testIpData, g_testCommandOutputDefaultGateways, g_testCommandOutputDnsServers};
        testModule.runCommandCount = 0;
        testModule.returnValues = returnValuesSystemdNetworkdEnabled;
        result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadSystemdNetworkdEnabled);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, GetDnsServers)
    {
        const char* payloadExpected =
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,fe80::215:5dff:fe26:cf91;eth0=172.29.64.1\","
            "\"dhcpEnabled\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\","
            "\"enabled\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\","
            "\"connected\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\"}";

        const char* payloadExpectedGlobalDnsServers =
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"br-1234=1.1.1.1,8.8.8.8;docker0=1.1.1.1,8.8.8.8,fe80::215:5dff:fe26:cf91;eth0=1.1.1.1,172.29.64.1,8.8.8.8;veth=1.1.1.1,8.8.8.8\","
            "\"dhcpEnabled\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\","
            "\"enabled\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\","
            "\"connected\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\"}";

        const char* payloadExpectedOnlyGlobalDnsServers =
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"br-1234=1.1.1.1,8.8.8.8;docker0=1.1.1.1,8.8.8.8;eth0=1.1.1.1,8.8.8.8;veth=1.1.1.1,8.8.8.8\","
            "\"dhcpEnabled\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\","
            "\"enabled\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\","
            "\"connected\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\"}";

        const char* payloadExpectedMultipleDnsServersPerLine =
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=1.1.1.1,10.20.30.40,100.10.20.30,2.2.2.2;eth0=1.1.1.1,10.20.30.40,100.40.50.60,2.2.2.2,3.3.3.3\","
            "\"dhcpEnabled\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\","
            "\"enabled\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\","
            "\"connected\":\"br-1234=unknown;docker0=true;eth0=false;veth=unknown\"}";

        std::string testCommandOutputInterfaceNames = "br-1234\ndocker0\nveth\neth0";

        std::string testCommandOutputDnsServers =
            "Global\n"
            "LLMNR setting: no\n"
            "MulticastDNS setting: no\n"
            "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
            "DNSSEC NTA: 10.in-addr.arpa\n"
            "\n"
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

        std::string testCommandOutputGlobalDnsServers =
            "Global\n"
            "LLMNR setting: no\n"
            "MulticastDNS setting: no\n"
            "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
            "Current DNS Server: 1.1.1.1\n"
            "DNS Servers: 1.1.1.1\n"
            "8.8.8.8\n"
            "DNSSEC NTA: 10.in-addr.arpa\n"
            "\n"
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

        std::string testCommandOutputOnlyGlobalDnsServers =
            "Global\n"
            "LLMNR setting: no\n"
            "MulticastDNS setting: no\n"
            "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
            "Current DNS Server: 1.1.1.1\n"
            "DNS Servers: 1.1.1.1\n"
            "8.8.8.8\n"
            "DNSSEC NTA: 10.in-addr.arpa\n"
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
            "DNS Domain: mshome.net\n";

        std::string testCommandOutputMultipleDnsServersPerLine =
            "Global\n"
            "LLMNR setting: no\n"
            "MulticastDNS setting: no\n"
            "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
            "Current DNS Server: 1.1.1.1\n"
            "DNS Servers: 1.1.1.1 2.2.2.2\n"
            "DNSSEC NTA: 10.in-addr.arpa\n"
            "Link 1 (docker0)\n"
            "Current Scopes: DNS\n"
            "DefaultRoute setting: yes\n"
            "LLMNR setting: yes\n"
            "MulticastDNS setting: no\n"
            "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
            "Current DNS Server: 10.20.30.40\n"
            "DNS Servers: 10.20.30.40 100.10.20.30\n"
            "DNS Domain: mshome.net\n"
            "Link 2 (eth0)\n"
            "Current Scopes: DNS\n"
            "DefaultRoute setting: yes\n"
            "LLMNR setting: yes\n"
            "MulticastDNS setting: no\n"
            "DNSOverTLS setting: no\n"
            "DNSSEC setting: no\n"
            "DNSSEC supported: no\n"
            "Current DNS Server: 10.20.30.40\n"
            "DNS Servers: 10.20.30.40 100.40.50.60\n"
            "2.2.2.2 3.3.3.3\n"
            "DNS Domain: mshome.net\n";



        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;
        testModule.returnValues[0] = testCommandOutputInterfaceNames;
        testModule.returnValues[4] = testCommandOutputDnsServers;

        int result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        EXPECT_NE(payload, nullptr);
        delete payload;

        testModule.runCommandCount = 0;
        testModule.returnValues[4] = testCommandOutputGlobalDnsServers;

        result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpectedGlobalDnsServers);

        EXPECT_NE(payload, nullptr);
        delete payload;

        testModule.runCommandCount = 0;
        testModule.returnValues[4] = testCommandOutputOnlyGlobalDnsServers;

        result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpectedOnlyGlobalDnsServers);

        EXPECT_NE(payload, nullptr);
        delete payload;

        testModule.runCommandCount = 0;
        testModule.returnValues[4] = testCommandOutputMultipleDnsServersPerLine;

        result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpectedMultipleDnsServersPerLine);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, GetSuccessMultipleCalls)
    {
        const char* payloadExpected =
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
            "\"dhcpEnabled\":\"docker0=true;eth0=false\","
            "\"enabled\":\"docker0=true;eth0=false\","
            "\"connected\":\"docker0=true;eth0=false\"}";

        const char* payloadExpectedAddedAddress =
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,10.1.1.1,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/16,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
            "\"dhcpEnabled\":\"docker0=true;eth0=false\","
            "\"enabled\":\"docker0=true;eth0=false\","
            "\"connected\":\"docker0=true;eth0=false\"}";

        std::string testIpDataDocker0AddedAddress =
            "1: docker0: <BROADCAST,UP,LOWER_UP> mtu 65536 qdisc noqueue state UP group default qlen 1000\n"
            "link/bridge 0a:25:3g:6v:2f:89 brd 00:00:00:00:00:00\n"
            "inet 172.32.233.234/8 scope global dynamic noprefixroute docker0 valid_lft forever preferred_lft forever\n"
            "inet 10.1.1.1/16 scope global dynamic noprefixroute docker0 valid_lft forever preferred_lft forever\n"
            "inet6 ::1/128 scope host valid_lft forever preferred_lft forever\n"
            "2: eth0: <BROADCAST,UP> mtu 65536 qdisc noqueue state DOWN group default qlen 1000\n"
            "link/ether 00:15:5d:26:cf:89 brd 00:00:00:00:00:00\n"
            "inet 172.27.181.213/20 scope noprefixroute valid_lft forever preferred_lft forever\n"
            "inet 10.1.1.2/16 scope global eth0\n"
            "inet6 fe80::5e42:4bf7:dddd:9b0f/64 scope link noprefixroute";

        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;
        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        int result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        EXPECT_NE(payload, nullptr);
        delete payload;

        testModule.runCommandCount = 0;
        testModule.returnValues[2] = testIpDataDocker0AddedAddress;

        result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpectedAddedAddress);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, GetSuccessEmptyDataInterfaceNames)
    {
        const char* payloadExpected =
            "{\"interfaceTypes\":\"\","
            "\"macAddresses\":\"\","
            "\"ipAddresses\":\"\","
            "\"subnetMasks\":\"\","
            "\"defaultGateways\":\"\","
            "\"dnsServers\":\"\","
            "\"dhcpEnabled\":\"\","
            "\"enabled\":\"\","
            "\"connected\":\"\"}";

        std::string testCommandOutputNamesEmpty = "";
        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = {testCommandOutputNamesEmpty};
        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        int result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString(payload, payloadSizeBytes);
        EXPECT_STREQ(resultString.c_str(), payloadExpected);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(NetworkingTests, GetSuccessEmptyDataEth0)
    {
        const char* payloadExpected =
            "{\"interfaceTypes\":\"docker0=bridge\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1\","
            "\"subnetMasks\":\"docker0=/8,/128\","
            "\"defaultGateways\":\"docker0=172.17.128.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,172.29.64.1\","
            "\"dhcpEnabled\":\"docker0=true;eth0=false\","
            "\"enabled\":\"docker0=true;eth0=unknown\","
            "\"connected\":\"docker0=true;eth0=false\"}";

        std::string testInterfaceTypesDataEth0Empty =
            "GENERAL.DEVICE:                         docker0\n"
            "GENERAL.TYPE:                           bridge\n"
            "GENERAL.HWADDR:                         02:42:65:B3:AC:5A\n"
            "GENERAL.MTU:                            1500\n"
            "GENERAL.STATE:                          100 (connected)\n"
            "GENERAL.CONNECTION:                     docker0\n"
            "GENERAL.DEVICE:                         eth0\n";

        std::string testIpDataEth0Empty =
            "1: docker0: <BROADCAST,UP,LOWER_UP> mtu 65536 qdisc noqueue state UP group default qlen 1000\n"
            "link/bridge 0a:25:3g:6v:2f:89 brd 00:00:00:00:00:00\n"
            "inet 172.32.233.234/8 scope global dynamic noprefixroute docker0 valid_lft forever preferred_lft forever\n"
            "inet6 ::1/128 scope host valid_lft forever preferred_lft forever\n"
            "2: eth0: ";

        std::string testCommandOutputDefaultGatewaysEth0Empty =
            "default via 172.17.128.1 dev docker0 proto\n"
            "172.29.64.0/20 dev eth0 proto kernel scope link src 172.29.78.164";

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

        NetworkingObjectTest testModule(g_maxPayloadSizeBytes);
        testModule.returnValues = g_returnValues;
        testModule.returnValues[1] = testInterfaceTypesDataEth0Empty;
        testModule.returnValues[2] = testIpDataEth0Empty;
        testModule.returnValues[3] = testCommandOutputDefaultGatewaysEth0Empty;
        testModule.returnValues[4] = testCommandOutputDnsServersEth0Empty;

        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        int result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

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
        int result = testModule.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        std::string resultString = std::string(payload, payloadSizeBytes);
        std::string expectedString =
            "{\"interfaceTypes\":\"..\","
            "\"macAddresses\":\"docker0=0a:25:3g:..\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10...\","
            "\"subnetMasks\":\"..\","
            "\"defaultGateways\":\"docker0..\","
            "\"dnsServers\":\"docker0=8.8.8..\","
            "\"dhcpEnabled\":\"..\","
            "\"enabled\":\"..\","
            "\"connected\":\"..\"}";

        EXPECT_STREQ(resultString.c_str(), expectedString.c_str());

        EXPECT_NE(payload, nullptr);
        delete payload;

        maxPayloadSizeBytes = 100;
        NetworkingObjectTest testModule2(maxPayloadSizeBytes);
        testModule2.returnValues = g_returnValues;
        result = testModule2.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        expectedString =
            "{\"interfaceTypes\":\"..\","
            "\"macAddresses\":\"..\","
            "\"ipAddresses\":\"..\","
            "\"subnetMasks\":\"..\","
            "\"defaultGateways\":\"..\","
            "\"dnsServers\":\"..\","
            "\"dhcpEnabled\":\"..\","
            "\"enabled\":\"..\","
            "\"connected\":\"..\"}";

        EXPECT_STREQ(resultString.c_str(), expectedString.c_str());

        EXPECT_NE(payload, nullptr);
        delete payload;

        maxPayloadSizeBytes = 0;
        NetworkingObjectTest testModule3(maxPayloadSizeBytes);
        testModule3.returnValues = g_returnValues;
        result = testModule3.Get(NETWORKING, NETWORK_CONFIGURATION, &payload, &payloadSizeBytes);

        EXPECT_EQ(result, MMI_OK);

        resultString = std::string(payload, payloadSizeBytes);
        expectedString =
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
            "\"dhcpEnabled\":\"docker0=true;eth0=false\","
            "\"enabled\":\"docker0=true;eth0=false\","
            "\"connected\":\"docker0=true;eth0=false\"}";

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
            "{\"interfaceTypes\":\"docker0=bridge;eth0=ethernet\","
            "\"macAddresses\":\"docker0=0a:25:3g:6v:2f:89;eth0=00:15:5d:26:cf:89\","
            "\"ipAddresses\":\"docker0=172.32.233.234,::1;eth0=172.27.181.213,10.1.1.2,fe80::5e42:4bf7:dddd:9b0f\","
            "\"subnetMasks\":\"docker0=/8,/128;eth0=/20,/16,/64\","
            "\"defaultGateways\":\"docker0=172.17.128.1;eth0=172.13.145.1\","
            "\"dnsServers\":\"docker0=8.8.8.8,172.29.64.1;eth0=172.29.64.1\","
            "\"dhcpEnabled\":\"docker0=true;eth0=false\","
            "\"enabled\":\"docker0=true;eth0=false\","
            "\"connected\":\"docker0=true;eth0=false\"}";

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
