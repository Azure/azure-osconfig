// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <cstdio>
#include <gtest/gtest.h>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <cstring>
#include <CommonUtils.h>
#include <Mmi.h>
#include <HostNameBase.h>
#include <Logging.h>

class HostNameBaseTests : public HostNameBase
{
public:
    HostNameBaseTests(const std::map<std::string, std::string> &textResults, size_t maxPayloadSizeBytes);
    ~HostNameBaseTests();

    int RunCommand(const char* command, bool replaceEol, std::string* textResult) override;

private:
    const std::map<std::string, std::string> &m_textResults;
};

HostNameBaseTests::HostNameBaseTests(const std::map<std::string, std::string> &textResults, size_t maxPayloadSizeBytes)
    : HostNameBase(maxPayloadSizeBytes), m_textResults(textResults)
{
}

HostNameBaseTests::~HostNameBaseTests()
{
}

int HostNameBaseTests::RunCommand(const char* command, bool replaceEol, std::string* textResult)
{
    UNUSED(replaceEol);

    std::map<std::string, std::string>::const_iterator it = m_textResults.find(command);
    if (it != m_textResults.end())
    {
        if (textResult)
        {
            *textResult = it->second;
        }
        return MMI_OK;
    }
    return ENOSYS;
}

namespace OSConfig::HostName::Tests
{
    constexpr const size_t g_maxPayloadSizeBytes = 4000;

    TEST(HostNameBaseTests, GetName)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hostname", "device"},
            };

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyName, &payload, &payloadSizeBytes);

        std::string result(payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);
        EXPECT_STREQ(result.c_str(), "\"device\"");

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, GetNameWithNewLine)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hostname", "device\n\r"},
            };

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyName, &payload, &payloadSizeBytes);

        std::string result(payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);
        EXPECT_STREQ(result.c_str(), "\"device\"");

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, GetNameWithNullTerminator)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hostname", "device\0"},
            };

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyName, &payload, &payloadSizeBytes);

        std::string result(payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);
        EXPECT_STREQ(result.c_str(), "\"device\"");

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, GetNameWithZeroPayloadByteSize)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hostname", "device"},
            };

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        HostNameBaseTests testModule(textResults, 0);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyName, &payload, &payloadSizeBytes);

        std::string result(payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);
        EXPECT_STREQ(result.c_str(), "\"device\"");

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, GetHosts)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hosts",
                 "127.0.0.1 localhost\n"
                 "::1 ip6-localhost ip6-loopback\n"
                 "fe00::0 ip6-localnet\n"
                 "ff00::0 ip6-mcastprefix\n"
                 "ff02::1 ip6-allnodes\n"
                 "ff02::2 ip6-allrouters\n"
                 "ff02::3 ip6-allhosts"},
            };

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyHosts, &payload, &payloadSizeBytes);

        std::string result(payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);
        EXPECT_STREQ(result.c_str(), "\"127.0.0.1 localhost;::1 ip6-localhost ip6-loopback;fe00::0 ip6-localnet;ff00::0 ip6-mcastprefix;ff02::1 ip6-allnodes;ff02::2 ip6-allrouters;ff02::3 ip6-allhosts\"");

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, GetHostsWithNewLine)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hosts",
                 "127.0.0.1 localhost\n"
                 "::1 ip6-localhost ip6-loopback\n"
                 "fe00::0 ip6-localnet\n"
                 "ff00::0 ip6-mcastprefix\n"
                 "ff02::1 ip6-allnodes\n"
                 "ff02::2 ip6-allrouters\n"
                 "ff02::3 ip6-allhosts\n\r"},
            };

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyHosts, &payload, &payloadSizeBytes);

        std::string result(payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);
        EXPECT_STREQ(result.c_str(), "\"127.0.0.1 localhost;::1 ip6-localhost ip6-loopback;fe00::0 ip6-localnet;ff00::0 ip6-mcastprefix;ff02::1 ip6-allnodes;ff02::2 ip6-allrouters;ff02::3 ip6-allhosts\"");

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, GetHostsWithNullTerminator)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hosts",
                 "127.0.0.1 localhost\n"
                 "::1 ip6-localhost ip6-loopback\n"
                 "fe00::0 ip6-localnet\n"
                 "ff00::0 ip6-mcastprefix\n"
                 "ff02::1 ip6-allnodes\n"
                 "ff02::2 ip6-allrouters\n"
                 "ff02::3 ip6-allhosts\n\0"},
            };

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyHosts, &payload, &payloadSizeBytes);

        std::string result(payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);
        EXPECT_STREQ(result.c_str(), "\"127.0.0.1 localhost;::1 ip6-localhost ip6-loopback;fe00::0 ip6-localnet;ff00::0 ip6-mcastprefix;ff02::1 ip6-allnodes;ff02::2 ip6-allrouters;ff02::3 ip6-allhosts\"");

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, GetHostsWithComments)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hosts",
                 "127.0.0.1 localhost\n"
                 "# The following lines are desirable for IPv6 capable hosts\n"
                 "::1 ip6-localhost ip6-loopback\n"
                 "fe00::0 ip6-localnet\n"
                 "ff00::0 ip6-mcastprefix\n"
                 "ff02::1 ip6-allnodes\n"
                 "ff02::2 ip6-allrouters\n"
                 "ff02::3 ip6-allhosts\n"},
            };

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyHosts, &payload, &payloadSizeBytes);

        std::string result(payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);
        EXPECT_STREQ(result.c_str(), "\"127.0.0.1 localhost;::1 ip6-localhost ip6-loopback;fe00::0 ip6-localnet;ff00::0 ip6-mcastprefix;ff02::1 ip6-allnodes;ff02::2 ip6-allrouters;ff02::3 ip6-allhosts\"");

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, GetHostsWithWhitespace)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hosts",
                 "  127.0.0.1 localhost\n"
                 "::1 ip6-localhost   ip6-loopback   \n"},
            };

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyHosts, &payload, &payloadSizeBytes);

        std::string result(payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);
        EXPECT_STREQ(result.c_str(), "\"127.0.0.1 localhost;::1 ip6-localhost ip6-loopback\"");

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, GetInvalidObject)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hostname", ""},
                {"cat /etc/hosts", ""},
            };

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, nullptr, nullptr, 0);

        EXPECT_EQ(status, EINVAL);
    }

    TEST(HostNameBaseTests, GetInvalidPayload)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hostname", "device1"},
            };

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyName, nullptr, 0);

        EXPECT_EQ(status, EINVAL);
    }

    TEST(HostNameBaseTests, GetPayloadTooLarge)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"cat /etc/hosts",
                 "127.0.0.1 localhost\n"
                 "::1 ip6-localhost ip6-loopback\n"
                 "fe00::0 ip6-localnet\n"
                 "ff00::0 ip6-mcastprefix\n"
                 "ff02::1 ip6-allnodes\n"
                 "ff02::2 ip6-allrouters\n"
                 "ff02::3 ip6-allhosts\n\0"},
            };

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        HostNameBaseTests testModule(textResults, 1);
        int status = testModule.Get(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyHosts, &payload, &payloadSizeBytes);

        std::string result(payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);
        EXPECT_STREQ(result.c_str(), "\"\"");

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, SetName)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"hostnamectl set-hostname --static 'device1'", ""},
            };
        const std::string name = "\"device1\"";
        const int payloadSizeBytes = name.length();

        MMI_JSON_STRING payload = new char[payloadSizeBytes];
        std::fill(payload, payload + payloadSizeBytes, 0);
        std::memcpy(payload, name.c_str(), payloadSizeBytes);

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Set(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyDesiredName, payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, SetHosts)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"echo '127.0.0.1 localhost\n::1 ip6-localhost ip6-loopback' > /etc/hosts", ""},
            };
        const std::string hosts = "\"127.0.0.1 localhost;::1 ip6-localhost ip6-loopback\"";
        const int payloadSizeBytes = hosts.length();

        MMI_JSON_STRING payload = new char[payloadSizeBytes];
        std::fill(payload, payload + payloadSizeBytes, 0);
        std::memcpy(payload, hosts.c_str(), payloadSizeBytes);

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Set(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyDesiredHosts, payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, SetHostsWithWhitespace)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"echo '127.0.0.1 localhost\n::1 ip6-localhost ip6-loopback' > /etc/hosts", ""},
            };
        const std::string hosts = "\"   127.0.0.1 localhost   ;   ::1    ip6-localhost   ip6-loopback   \"";
        const int payloadSizeBytes = hosts.length();

        MMI_JSON_STRING payload = new char[payloadSizeBytes];
        std::fill(payload, payload + payloadSizeBytes, 0);
        std::memcpy(payload, hosts.c_str(), payloadSizeBytes);

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Set(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyDesiredHosts, payload, payloadSizeBytes);

        EXPECT_EQ(status, MMI_OK);

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, SetInvalidObject)
    {
        const std::map<std::string, std::string> textResults;
        const std::string name = "_device";
        const int payloadSizeBytes = name.length();

        MMI_JSON_STRING payload = new char[payloadSizeBytes];
        std::fill(payload, payload + payloadSizeBytes, 0);
        std::memcpy(payload, name.c_str(), payloadSizeBytes);

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Set(&testModule, HostNameBase::m_componentName, nullptr, payload, payloadSizeBytes);

        EXPECT_EQ(status, EINVAL);

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, SetInvalidPayload)
    {
        const std::map<std::string, std::string> textResults =
            {
                {"hostnamectl set-hostname --static 'device1'", ""},
            };

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Set(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyDesiredName, nullptr, 0);

        EXPECT_EQ(status, EINVAL);
    }

    TEST(HostNameBaseTests, SetInvalidName)
    {
        const std::map<std::string, std::string> textResults;
        const std::string name = "_device";
        const int payloadSizeBytes = name.length();

        MMI_JSON_STRING payload = new char[payloadSizeBytes];
        std::fill(payload, payload + payloadSizeBytes, 0);
        std::memcpy(payload, name.c_str(), payloadSizeBytes);

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Set(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyDesiredName, payload, payloadSizeBytes);

        EXPECT_EQ(status, EINVAL);

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, SetInvalidHosts)
    {
        const std::map<std::string, std::string> textResults;
        const std::string hosts =
            "127.0.0.1 localhost"
            "fe00::0 #ip6-localnet";
        const int payloadSizeBytes = hosts.length();

        MMI_JSON_STRING payload = new char[payloadSizeBytes];
        std::fill(payload, payload + payloadSizeBytes, 0);
        std::memcpy(payload, hosts.c_str(), payloadSizeBytes);

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Set(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyDesiredHosts, payload, payloadSizeBytes);

        EXPECT_EQ(status, EINVAL);

        ::HostNameFree(payload);
    }

    TEST(HostNameBaseTests, SetPayloadTooLarge)
    {
        const std::map<std::string, std::string> textResults;
        const int payloadSizeBytes = g_maxPayloadSizeBytes + 1;

        MMI_JSON_STRING payload = new char[payloadSizeBytes];
        std::fill(payload, payload + payloadSizeBytes, 0);

        HostNameBaseTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Set(&testModule, HostNameBase::m_componentName, HostNameBase::m_propertyDesiredHosts, payload, payloadSizeBytes);

        EXPECT_EQ(status, E2BIG);

        ::HostNameFree(payload);
    }

} // namespace OSConfig::HostName::Tests
