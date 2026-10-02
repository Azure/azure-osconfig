// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <TelemetryHttp.h>
#include <TelemetryProxy.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

class TelemetryProxyTest : public ::testing::Test
{
protected:
    struct SavedVariable
    {
        const char* name;
        bool present;
        std::string value;
    };
    std::vector<SavedVariable> saved;
    TelemetryProxySelection selection = {};

    void SetUp() override
    {
        for (const char* name : {"https_proxy", "HTTPS_PROXY", "all_proxy", "ALL_PROXY",
            "no_proxy", "NO_PROXY", "http_proxy", "HTTP_PROXY"})
        {
            const char* value = getenv(name);
            saved.push_back({name, value != NULL, value ? value : ""});
            ASSERT_EQ(0, unsetenv(name));
        }
    }

    void TearDown() override
    {
        for (const auto& variable : saved)
        {
            int status = variable.present ? setenv(variable.name, variable.value.c_str(), 1) :
                unsetenv(variable.name);
            EXPECT_EQ(0, status);
        }
    }

    void Set(const char* name, const std::string& value)
    {
        ASSERT_EQ(0, setenv(name, value.c_str(), 1));
    }

    void ExpectProxy(const char* url)
    {
        ASSERT_EQ(0, TelemetryProxyDiscover(&selection, NULL));
        EXPECT_EQ(TelemetryProxyConfigured, selection.kind);
        EXPECT_STREQ(url, selection.url);
    }

    void ExpectDirect()
    {
        ASSERT_EQ(0, TelemetryProxyDiscover(&selection, NULL));
        EXPECT_EQ(TelemetryProxyDirect, selection.kind);
        EXPECT_STREQ("", selection.url);
    }
};

TEST_F(TelemetryProxyTest, DefaultsToDirectWithoutProxyConfiguration)
{
    EXPECT_EQ(TelemetryProxyUnresolved, selection.kind);
    ExpectDirect();
}

TEST_F(TelemetryProxyTest, UsesHttpsThenAllProxyWithLowercasePrecedence)
{
    const char* names[] = {"https_proxy", "HTTPS_PROXY", "all_proxy", "ALL_PROXY"};
    const char* urls[] = {"http://lower.invalid:81", "http://upper.invalid:82",
        "socks5h://lower.invalid:83", "https://upper.invalid:84"};
    for (size_t i = 0; i < 4; ++i) Set(names[i], urls[i]);
    for (size_t i = 0; i < 4; ++i)
    {
        ExpectProxy(urls[i]);
        ASSERT_EQ(0, unsetenv(names[i]));
    }
    ExpectDirect();
}

TEST_F(TelemetryProxyTest, EmptyValuesFallThroughLikeCurlGetenv)
{
    Set("ALL_PROXY", "http://fallback.invalid");
    for (const char* name : {"https_proxy", "HTTPS_PROXY", "all_proxy"}) Set(name, "");
    ExpectProxy("http://fallback.invalid");
    Set("ALL_PROXY", "");
    ExpectDirect();
}

TEST_F(TelemetryProxyTest, NeverUsesHttpProxyVariablesForHttpsCollector)
{
    Set("http_proxy", "http://lower.invalid");
    Set("HTTP_PROXY", "http://upper.invalid");
    ExpectDirect();
    Set("ALL_PROXY", "http://fallback.invalid");
    ExpectProxy("http://fallback.invalid");
}

TEST_F(TelemetryProxyTest, LowercaseNoProxyOverridesUppercaseUnlessEmpty)
{
    Set("https_proxy", "http://proxy.invalid");
    Set("NO_PROXY", TELEMETRY_ARIA_HOST);
    Set("no_proxy", "unrelated.invalid");
    ExpectProxy("http://proxy.invalid");
    Set("no_proxy", "");
    ExpectDirect();
    ASSERT_EQ(0, unsetenv("no_proxy"));
    ExpectDirect();
}

TEST_F(TelemetryProxyTest, MatchesExactHostAndDomainSuffixAtLabelBoundaries)
{
    Set("https_proxy", "http://proxy.invalid");
    for (const char* bypass : {TELEMETRY_ARIA_HOST, "events.data.microsoft.com",
        "data.microsoft.com", "microsoft.com", "com"})
    {
        SCOPED_TRACE(bypass);
        Set("no_proxy", bypass);
        ExpectDirect();
    }
    for (const char* bypass : {"icrosoft.com", "soft.com", "mobile.events", "microsoft",
        "sub.mobile.events.data.microsoft.com", "microsoft.com.invalid"})
    {
        SCOPED_TRACE(bypass);
        Set("no_proxy", bypass);
        ExpectProxy("http://proxy.invalid");
    }
}

TEST_F(TelemetryProxyTest, IgnoresCaseAndOneLeadingAndTrailingDot)
{
    Set("https_proxy", "http://proxy.invalid");
    for (const char* bypass : {"MOBILE.EVENTS.DATA.MICROSOFT.COM", ".microsoft.com",
        "microsoft.com.", ".MICROSOFT.COM."})
    {
        SCOPED_TRACE(bypass);
        Set("no_proxy", bypass);
        ExpectDirect();
    }
    for (const char* bypass : {"..microsoft.com", "microsoft.com..", ".", ".."})
    {
        SCOPED_TRACE(bypass);
        Set("no_proxy", bypass);
        ExpectProxy("http://proxy.invalid");
    }
}

TEST_F(TelemetryProxyTest, SupportsCommaListsWithSpacesTabsAndEmptyEntries)
{
    Set("https_proxy", "http://proxy.invalid");
    for (const char* bypass : {"other.invalid, microsoft.com", ",,,\t.microsoft.com. \t,,,",
        "other.invalid \t, \t microsoft.com \t, last.invalid", " \tmicrosoft.com\t "})
    {
        SCOPED_TRACE(bypass);
        Set("no_proxy", bypass);
        ExpectDirect();
    }
    Set("no_proxy", " \t,,, ,,, ");
    ExpectProxy("http://proxy.invalid");
}

TEST_F(TelemetryProxyTest, WhitespaceDoesNotReplaceCommaSeparators)
{
    Set("https_proxy", "http://proxy.invalid");
    for (const char* bypass : {"other.invalid microsoft.com", "other.invalid\tmicrosoft.com",
        "other.invalid ignored.invalid,microsoft.com", "microsoft.com\n", "microsoft.com\r"})
    {
        SCOPED_TRACE(bypass);
        Set("no_proxy", bypass);
        ExpectProxy("http://proxy.invalid");
    }
    // A newline is part of a nonmatching token; the following comma still separates entries.
    Set("no_proxy", "other.invalid\n,microsoft.com");
    ExpectDirect();
    Set("no_proxy", "microsoft.com ignored.invalid");
    ExpectDirect();
}

TEST_F(TelemetryProxyTest, WildcardMustBeTheEntireUntrimmedValue)
{
    Set("https_proxy", "http://proxy.invalid");
    Set("no_proxy", "*");
    ExpectDirect();
    for (const char* bypass : {" *", "* ", "*,other.invalid", "other.invalid,*",
        "*.microsoft.com", "mobile.*", "*.*", "?"})
    {
        SCOPED_TRACE(bypass);
        Set("no_proxy", bypass);
        ExpectProxy("http://proxy.invalid");
    }
}

TEST_F(TelemetryProxyTest, DoesNotResolveDestinationForIpOrCidrBypass)
{
    Set("https_proxy", "http://proxy.invalid");
    for (const char* bypass : {"127.0.0.1", "0.0.0.0/0", "10.0.0.0/8", "::1",
        "[::1]", "::/0", "microsoft.com/24"})
    {
        SCOPED_TRACE(bypass);
        Set("no_proxy", bypass);
        ExpectProxy("http://proxy.invalid");
    }
}

TEST_F(TelemetryProxyTest, DoesNotTreatPortsOrUrlsAsBypassDomains)
{
    Set("https_proxy", "http://proxy.invalid");
    for (const char* bypass : {"microsoft.com:443", TELEMETRY_ARIA_HOST ":443",
        "https://microsoft.com", "https://" TELEMETRY_ARIA_HOST "/", "<local>"})
    {
        SCOPED_TRACE(bypass);
        Set("no_proxy", bypass);
        ExpectProxy("http://proxy.invalid");
    }
}

TEST_F(TelemetryProxyTest, PreservesConfiguredUrlWithoutDecodingOrTrimming)
{
    for (const char* url : {"proxy.invalid:8080", "http://user:p%40ss@proxy.invalid:8080/",
        "https://proxy.invalid", "socks4://proxy.invalid", "socks4a://proxy.invalid",
        "socks5://proxy.invalid", "socks5h://proxy.invalid", "unknown://proxy.invalid",
        "   ", "http://proxy.invalid\r\n", "http://[::1]:8080"})
    {
        Set("https_proxy", url);
        ExpectProxy(url);
    }
}

TEST_F(TelemetryProxyTest, CopiesSelectionAndDoesNotModifyEnvironment)
{
    Set("https_proxy", "http://proxy.invalid");
    Set("no_proxy", "other.invalid");
    ExpectProxy("http://proxy.invalid");
    EXPECT_STREQ("http://proxy.invalid", getenv("https_proxy"));
    EXPECT_STREQ("other.invalid", getenv("no_proxy"));
    Set("https_proxy", "http://replacement.invalid");
    EXPECT_STREQ("http://proxy.invalid", selection.url);
    ExpectProxy("http://replacement.invalid");
    Set("no_proxy", "*");
    ExpectDirect();
    for (char value : selection.url) EXPECT_EQ('\0', value);
}

TEST_F(TelemetryProxyTest, EnforcesExactProxyLimitWithoutTryingAnotherRoute)
{
    std::string url = "http://proxy.invalid/";
    url.append(TELEMETRY_PROXY_URL_LIMIT - url.size(), 'a');
    Set("https_proxy", url);
    ExpectProxy(url.c_str());
    Set("HTTPS_PROXY", "http://fallback.invalid");
    url += 'a';
    Set("https_proxy", url);
    EXPECT_EQ(E2BIG, TelemetryProxyDiscover(&selection, NULL));
    EXPECT_EQ(TelemetryProxyUnresolved, selection.kind);
    for (char value : selection.url) EXPECT_EQ('\0', value);
}

TEST_F(TelemetryProxyTest, EnforcesExactBypassLimitBeforeMakingRoutingDecision)
{
    Set("https_proxy", "http://proxy.invalid");
    std::string bypass(TELEMETRY_PROXY_BYPASS_LIMIT - strlen(",microsoft.com"), 'x');
    bypass += ",microsoft.com";
    Set("no_proxy", bypass);
    ExpectDirect();
    Set("no_proxy", bypass + ",");
    EXPECT_EQ(E2BIG, TelemetryProxyDiscover(&selection, NULL));
    EXPECT_EQ(TelemetryProxyUnresolved, selection.kind);
    EXPECT_STREQ("", selection.url);
    ASSERT_EQ(0, unsetenv("https_proxy"));
    EXPECT_EQ(E2BIG, TelemetryProxyDiscover(&selection, NULL));
}

TEST_F(TelemetryProxyTest, DoesNotInspectOverriddenOrBypassedProxyValues)
{
    std::string oversized(TELEMETRY_PROXY_URL_LIMIT + 1, 'x');
    Set("https_proxy", "http://proxy.invalid");
    for (const char* name : {"HTTPS_PROXY", "all_proxy", "ALL_PROXY"}) Set(name, oversized);
    ExpectProxy("http://proxy.invalid");
    Set("https_proxy", oversized);
    Set("no_proxy", "*");
    ExpectDirect();
    Set("NO_PROXY", std::string(TELEMETRY_PROXY_BYPASS_LIMIT + 1, 'x'));
    ExpectDirect();
}

TEST_F(TelemetryProxyTest, RejectsMissingOutput)
{
    EXPECT_EQ(EINVAL, TelemetryProxyDiscover(NULL, NULL));
}

TEST_F(TelemetryProxyTest, ParsesHttpProxyHostsPortsAndCurlDefault)
{
    TelemetryHttpProxy proxy = {};
    for (const char* url : {"proxy.invalid", "http://proxy.invalid/", "HTTP://proxy.invalid"})
    {
        ASSERT_EQ(0, TelemetryProxyParseHttp(url, &proxy, NULL));
        EXPECT_STREQ("proxy.invalid", proxy.host);
        EXPECT_EQ(1080, proxy.port);
        EXPECT_STREQ("", proxy.authorization);
    }
    ASSERT_EQ(0, TelemetryProxyParseHttp("http://127.0.0.1:65535", &proxy, NULL));
    EXPECT_EQ(65535, proxy.port);
    ASSERT_EQ(0, TelemetryProxyParseHttp("http://[::1]:8080/", &proxy, NULL));
    EXPECT_STREQ("::1", proxy.host);
    EXPECT_EQ(8080, proxy.port);
}

TEST_F(TelemetryProxyTest, DecodesCredentialsOnlyForProxyAuthorization)
{
    TelemetryHttpProxy proxy = {};
    ASSERT_EQ(0, TelemetryProxyParseHttp("http://us%65r:p%40ss@proxy.invalid", &proxy, NULL));
    EXPECT_STREQ("Basic dXNlcjpwQHNz", proxy.authorization);
    ASSERT_EQ(0, TelemetryProxyParseHttp("user@proxy.invalid", &proxy, NULL));
    EXPECT_STREQ("Basic dXNlcjo=", proxy.authorization);
    ASSERT_EQ(0, TelemetryProxyParseHttp("http://:@proxy.invalid", &proxy, NULL));
    EXPECT_STREQ("Basic Og==", proxy.authorization);
}

TEST_F(TelemetryProxyTest, RejectsUnsupportedRoutesWithoutDirectFallback)
{
    TelemetryHttpProxy proxy = {};
    for (const char* url : {"https://proxy.invalid", "socks5://proxy.invalid",
        "socks5h://proxy.invalid", "socks4a://proxy.invalid", "ftp://proxy.invalid",
        "http://proxy.invalid/path", "http://proxy.invalid?query", "http://proxy.invalid#fragment"})
    {
        SCOPED_TRACE(url);
        EXPECT_EQ(ENOTSUP, TelemetryProxyParseHttp(url, &proxy, NULL));
        EXPECT_EQ(0, proxy.port);
        EXPECT_STREQ("", proxy.host);
        EXPECT_STREQ("", proxy.authorization);
    }
}

TEST_F(TelemetryProxyTest, RejectsMalformedProxyAuthorityAndCredentials)
{
    TelemetryHttpProxy proxy = {};
    for (const char* url : {"", "http://", "http://:8080", "http://proxy:0", "http://proxy:65536",
        "http://proxy:-1", "http://proxy:", "http://proxy:1x", "http://proxy:999999999999999999",
        "http://[::1", "http://[invalid]", "http://[::1]bad", "http://::1",
        "http://[fe80::1%25eth0]", "http://pro%78y", "http://a b", "http://p\r\n",
        "http://user:p%00@proxy", "http://user:p%0a@proxy", "http://user:p%@proxy",
        "http://user:p%gg@proxy", "http://us%3aer:p@proxy", "http://a@b@proxy"})
    {
        SCOPED_TRACE(url);
        EXPECT_EQ(EINVAL, TelemetryProxyParseHttp(url, &proxy, NULL));
        EXPECT_EQ(0, proxy.port);
        EXPECT_STREQ("", proxy.host);
        EXPECT_STREQ("", proxy.authorization);
    }
}

TEST_F(TelemetryProxyTest, BoundsDecodedCredentialsAndUrl)
{
    TelemetryHttpProxy proxy = {};
    const std::string user(256, 'u'), password(256, 'p');
    EXPECT_EQ(0, TelemetryProxyParseHttp(("http://" + user + ":" + password + "@proxy").c_str(), &proxy, NULL));
    EXPECT_EQ(690U, strlen(proxy.authorization));
    EXPECT_EQ(E2BIG, TelemetryProxyParseHttp(("http://" + user + "u:@proxy").c_str(), &proxy, NULL));
    EXPECT_EQ(E2BIG, TelemetryProxyParseHttp(("http://u:" + password + "p@proxy").c_str(), &proxy, NULL));
    EXPECT_EQ(E2BIG, TelemetryProxyParseHttp(std::string(TELEMETRY_PROXY_URL_LIMIT + 1, 'a').c_str(), &proxy, NULL));
}

TEST_F(TelemetryProxyTest, BuildsExactConnectWithoutCollectorTokenOrBody)
{
    TelemetryHttpProxy proxy = {};
    ASSERT_EQ(0, TelemetryProxyParseHttp("http://user:p%40ss@proxy.invalid:8080", &proxy, NULL));
    char bytes[1024] = {};
    size_t size = 0;
    ASSERT_EQ(0, TelemetryProxyBuildConnect(&proxy, bytes, sizeof(bytes), &size, NULL));
    const std::string expected = "CONNECT " TELEMETRY_ARIA_HOST ":443 HTTP/1.1\r\n"
        "Host: " TELEMETRY_ARIA_HOST ":443\r\nProxy-Authorization: Basic dXNlcjpwQHNz\r\n\r\n";
    EXPECT_EQ(expected, std::string(bytes, size));
    EXPECT_EQ(EMSGSIZE, TelemetryProxyBuildConnect(&proxy, bytes, expected.size(), &size, NULL));
    EXPECT_EQ(0U, size);
    EXPECT_EQ(expected, bytes);
    EXPECT_EQ(0, TelemetryProxyBuildConnect(&proxy, bytes, expected.size() + 1, &size, NULL));
    ASSERT_EQ(0, TelemetryProxyParseHttp("http://proxy.invalid", &proxy, NULL));
    ASSERT_EQ(0, TelemetryProxyBuildConnect(&proxy, bytes, sizeof(bytes), &size, NULL));
    EXPECT_STREQ("CONNECT " TELEMETRY_ARIA_HOST ":443 HTTP/1.1\r\n"
        "Host: " TELEMETRY_ARIA_HOST ":443\r\n\r\n", bytes);
}

TEST_F(TelemetryProxyTest, RejectsInvalidHttpProxyApiArguments)
{
    TelemetryHttpProxy proxy = {};
    char bytes[1024] = {};
    size_t size = 999;
    EXPECT_EQ(EINVAL, TelemetryProxyParseHttp(NULL, &proxy, NULL));
    EXPECT_EQ(EINVAL, TelemetryProxyParseHttp("proxy", NULL, NULL));
    EXPECT_EQ(EINVAL, TelemetryProxyBuildConnect(&proxy, bytes, sizeof(bytes), &size, NULL));
    EXPECT_EQ(0U, size);
    ASSERT_EQ(0, TelemetryProxyParseHttp("proxy", &proxy, NULL));
    EXPECT_EQ(EINVAL, TelemetryProxyBuildConnect(NULL, bytes, sizeof(bytes), &size, NULL));
    EXPECT_EQ(EINVAL, TelemetryProxyBuildConnect(&proxy, NULL, sizeof(bytes), &size, NULL));
    EXPECT_EQ(EINVAL, TelemetryProxyBuildConnect(&proxy, bytes, sizeof(bytes), NULL, NULL));
}
