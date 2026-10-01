// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <TelemetryEncoder.h>
#include <TelemetryHttp.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
const std::string accepted = "{\"acc\":1,\"rej\":0}";

std::string Response(const std::string& body = accepted, const std::string& headers = "",
    const std::string& status = "HTTP/1.1 200 OK")
{
    return status + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n" + headers + "\r\n" + body;
}

std::string Chunk(const std::string& body, const std::string& extensions = "")
{
    char size[32] = {};
    snprintf(size, sizeof(size), "%zx", body.size());
    return std::string(size) + extensions + "\r\n" + body + "\r\n";
}

std::string Chunked(const std::string& chunks)
{
    return "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" + chunks;
}
}

class TelemetryHttpTest : public ::testing::Test
{
protected:
    TelemetryHttpResponse response = {};

    void SetUp() override
    {
        ASSERT_EQ(0, TelemetryHttpResponseInitialize(&response, NULL));
    }

    int Parse(const std::string& wire, bool eof = false)
    {
        int status = TelemetryHttpResponseInitialize(&response, NULL);
        return status ? status : TelemetryHttpResponseFeed(&response, wire.data(), wire.size(), eof, NULL);
    }

    void ExpectFailure(const std::string& wire, int expected, bool eof = false)
    {
        EXPECT_EQ(expected, Parse(wire, eof));
        EXPECT_FALSE(response.complete);
        EXPECT_FALSE(response.reusable);
        EXPECT_EQ(TelemetryUnconfirmed, response.acceptance);
    }
};

TEST_F(TelemetryHttpTest, BuildsExactUncompressedSingleEventHeaders)
{
    char headers[TELEMETRY_HTTP_HEADER_LIMIT + 1] = {};
    size_t size = 0;
    ASSERT_EQ(0, TelemetryHttpBuildRequest("test-token", "OSConfig-C/1.0", 123456789, 17,
        headers, sizeof(headers), &size, NULL));
    const std::string expected =
        "POST /OneCollector/1.0/ HTTP/1.1\r\n"
        "Host: mobile.events.data.microsoft.com\r\n"
        "Content-Type: application/bond-compact-binary\r\n"
        "Content-Length: 17\r\n"
        "APIKey: test-token\r\n"
        "Client-Id: NO_AUTH\r\n"
        "SDK-Version: OSConfig-C/1.0\r\n"
        "Upload-Time: 123456789\r\n"
        "Accept-Encoding: identity\r\n\r\n";
    EXPECT_EQ(expected, std::string(headers, size));
    EXPECT_EQ('\0', headers[size]);
}

TEST_F(TelemetryHttpTest, RejectsHeaderInjectionAndMultipleTokensWithoutChangingOutput)
{
    std::array<char, TELEMETRY_HTTP_HEADER_LIMIT + 1> headers;
    headers.fill('x');
    const auto original = headers;
    for (const char* token : {"", "a\r\nInjected: yes", "a\nb", "a b", "a\tb", "a,b", "\x7f", "\x80"})
    {
        size_t size = 999;
        EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest(token, "OSConfig-C/1.0", 1, 17,
            headers.data(), headers.size(), &size, NULL));
        EXPECT_EQ(0U, size);
        EXPECT_EQ(original, headers);
    }
    size_t size = 999;
    EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest("test", "a\r\nb", 1, 17,
        headers.data(), headers.size(), &size, NULL));
    EXPECT_EQ(0U, size);
    EXPECT_EQ(original, headers);
}

TEST_F(TelemetryHttpTest, EnforcesRequestLimitsAndExactOutputCapacity)
{
    char headers[TELEMETRY_HTTP_HEADER_LIMIT + 1] = {};
    size_t size = 0;
    const std::string token(TELEMETRY_HTTP_TOKEN_LIMIT, 't');
    const std::string version(TELEMETRY_HTTP_VERSION_LIMIT, 'v');
    ASSERT_EQ(0, TelemetryHttpBuildRequest(token.c_str(), version.c_str(), INT64_MAX,
        TELEMETRY_MAX_EVENT_SIZE, headers, sizeof(headers), &size, NULL));
    const size_t required = size;
    ASSERT_EQ(0, TelemetryHttpBuildRequest(token.c_str(), version.c_str(), INT64_MAX,
        TELEMETRY_MAX_EVENT_SIZE, headers, required + 1, &size, NULL));
    EXPECT_EQ(EMSGSIZE, TelemetryHttpBuildRequest(token.c_str(), version.c_str(), INT64_MAX,
        TELEMETRY_MAX_EVENT_SIZE, headers, required, &size, NULL));
    EXPECT_EQ(0U, size);
    EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest((token + 't').c_str(), version.c_str(), 0,
        1, headers, sizeof(headers), &size, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest(token.c_str(), (version + 'v').c_str(), 0,
        1, headers, sizeof(headers), &size, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest("t", "v", -1, 1, headers, sizeof(headers), &size, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest("t", "v", 0, 0, headers, sizeof(headers), &size, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest("t", "v", 0, TELEMETRY_MAX_EVENT_SIZE + 1,
        headers, sizeof(headers), &size, NULL));
}

TEST_F(TelemetryHttpTest, RejectsInvalidApiArguments)
{
    char headers[512] = {};
    size_t size = 0;
    EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest(NULL, "v", 0, 1, headers, sizeof(headers), &size, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest("t", NULL, 0, 1, headers, sizeof(headers), &size, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest("t", "v", 0, 1, NULL, 512, &size, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpBuildRequest("t", "v", 0, 1, headers, sizeof(headers), NULL, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpResponseInitialize(NULL, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpResponseFeed(NULL, NULL, 0, false, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpResponseFeed(&response, NULL, 1, false, NULL));
    EXPECT_EQ(EINVAL, TelemetryHttpResponseFeed(&response, NULL, 0, false, NULL));
    ASSERT_EQ(0, TelemetryHttpResponseInitialize(&response, NULL));
    EXPECT_EQ(0, TelemetryHttpResponseFeed(&response, NULL, 0, false, NULL));
    EXPECT_FALSE(response.complete);
}

TEST_F(TelemetryHttpTest, AcceptsCompleteLengthDelimitedResponseWithoutWaitingForEof)
{
    ASSERT_EQ(0, Parse(Response()));
    EXPECT_TRUE(response.complete);
    EXPECT_TRUE(response.reusable);
    EXPECT_EQ(200U, response.status);
    EXPECT_EQ(TelemetryAccepted, response.acceptance);
    EXPECT_EQ(accepted, std::string(response.body, response.bodySize));
}

TEST_F(TelemetryHttpTest, HandlesEverySplitPointAndSingleByteFragments)
{
    const std::string wire = "HTTP/1.1 100 Continue\r\n\r\n" +
        Response(accepted, "X-Extension: value\r\n");
    for (size_t split = 0; split <= wire.size(); ++split)
    {
        SCOPED_TRACE(split);
        ASSERT_EQ(0, TelemetryHttpResponseInitialize(&response, NULL));
        ASSERT_EQ(0, TelemetryHttpResponseFeed(&response, wire.data(), split, false, NULL));
        ASSERT_EQ(0, TelemetryHttpResponseFeed(&response, wire.data() + split, wire.size() - split, false, NULL));
        EXPECT_TRUE(response.complete);
        EXPECT_TRUE(response.reusable);
        EXPECT_EQ(TelemetryAccepted, response.acceptance);
    }
    ASSERT_EQ(0, TelemetryHttpResponseInitialize(&response, NULL));
    for (size_t i = 0; i < wire.size(); ++i)
    {
        ASSERT_EQ(0, TelemetryHttpResponseFeed(&response, &wire[i], 1, false, NULL));
        EXPECT_EQ(i + 1 == wire.size(), response.complete);
    }
}

TEST_F(TelemetryHttpTest, HandlesChunkedExtensionsAndTrailersAtEverySplitPoint)
{
    const std::string wire = Chunked(Chunk(accepted.substr(0, 5), ";a=\"x\\\"y\";b=token") +
        Chunk(accepted.substr(5)) + "0\r\nX-Trace: done\r\n\r\n");
    for (size_t split = 0; split <= wire.size(); ++split)
    {
        ASSERT_EQ(0, TelemetryHttpResponseInitialize(&response, NULL));
        ASSERT_EQ(0, TelemetryHttpResponseFeed(&response, wire.data(), split, false, NULL));
        ASSERT_EQ(0, TelemetryHttpResponseFeed(&response, wire.data() + split, wire.size() - split, false, NULL));
        EXPECT_TRUE(response.complete);
        EXPECT_TRUE(response.reusable);
        EXPECT_EQ(TelemetryAccepted, response.acceptance);
        EXPECT_EQ(accepted, std::string(response.body, response.bodySize));
    }
}

TEST_F(TelemetryHttpTest, CloseDelimitedResponseRequiresValidEofAndCannotBeReused)
{
    const std::string wire = "HTTP/1.1 200 OK\r\n\r\n" + accepted;
    ASSERT_EQ(0, Parse(wire));
    EXPECT_FALSE(response.complete);
    EXPECT_EQ(TelemetryUnconfirmed, response.acceptance);
    ASSERT_EQ(0, TelemetryHttpResponseFeed(&response, NULL, 0, true, NULL));
    EXPECT_TRUE(response.complete);
    EXPECT_FALSE(response.reusable);
    EXPECT_EQ(TelemetryAccepted, response.acceptance);
}

TEST_F(TelemetryHttpTest, AppliesConnectionTokensAndHttpVersionDefaults)
{
    ASSERT_EQ(0, Parse(Response(accepted, "Connection: xclose\r\n")));
    EXPECT_TRUE(response.reusable);
    ASSERT_EQ(0, Parse(Response(accepted, "Connection: keep-alive, CLOSE\r\n")));
    EXPECT_FALSE(response.reusable);
    ASSERT_EQ(0, Parse(Response(accepted, "", "HTTP/1.0 200 OK")));
    EXPECT_FALSE(response.reusable);
    ASSERT_EQ(0, Parse(Response(accepted, "Connection: Keep-Alive\r\n", "HTTP/1.0 200 OK")));
    EXPECT_TRUE(response.reusable);
    ASSERT_EQ(0, Parse(Response(), true));
    EXPECT_FALSE(response.reusable);
    ExpectFailure(Response(accepted, "Connection: close,\r\n"), EPROTO);
    ExpectFailure(Response(accepted, "Connection: keep alive\r\n"), EPROTO);
}

TEST_F(TelemetryHttpTest, EmptyOrBodylessSuccessIsNotConfirmedAcceptance)
{
    ASSERT_EQ(0, Parse(Response("")));
    EXPECT_TRUE(response.complete);
    EXPECT_EQ(TelemetryUnconfirmed, response.acceptance);
    ASSERT_EQ(0, Parse("HTTP/1.1 204 No Content\r\n\r\n"));
    EXPECT_TRUE(response.complete);
    EXPECT_EQ(TelemetryRejected, response.acceptance);
    ASSERT_EQ(0, Parse("HTTP/1.1 304 Not Modified\r\nContent-Length: 999999\r\n\r\n"));
    EXPECT_TRUE(response.complete);
    EXPECT_EQ(TelemetryRejected, response.acceptance);
    ExpectFailure("HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n", EPROTO);
}

TEST_F(TelemetryHttpTest, DoesNotTreatRedirectsOrServerErrorsAsAccepted)
{
    for (const char* status : {"201 Created", "302 Found", "400 Bad Request", "401 Unauthorized",
        "403 Forbidden", "408 Timeout", "429 Too Many Requests", "500 Server Error", "503 Unavailable"})
    {
        SCOPED_TRACE(status);
        ASSERT_EQ(0, Parse(Response(accepted, "", std::string("HTTP/1.1 ") + status)));
        EXPECT_TRUE(response.complete);
        EXPECT_EQ(TelemetryRejected, response.acceptance);
    }
    ASSERT_EQ(0, Parse(Response("<html>unavailable</html>", "", "HTTP/1.1 503 Unavailable")));
    EXPECT_EQ(TelemetryRejected, response.acceptance);
}

TEST_F(TelemetryHttpTest, HonorsExplicitRejectionAndIndexedEventFailures)
{
    for (const char* body : {
        "{\"acc\":0,\"rej\":1}",
        "{\"acc\":1,\"rej\":0,\"efi\":{\"failure\":\"all\"}}",
        "{\"acc\":1,\"rej\":0,\"efi\":{\"failure\":[0]}}",
        "{\"acc\":1,\"rej\":0,\"TokenCrackingFailure\":true}"})
    {
        SCOPED_TRACE(body);
        ASSERT_EQ(0, Parse(Response(body)));
        EXPECT_TRUE(response.complete);
        EXPECT_EQ(TelemetryRejected, response.acceptance);
    }
    ASSERT_EQ(0, Parse(Response("{\"acc\":1,\"rej\":0,\"efi\":{\"failure\":[]},\"extra\":{\"v\":true}}")));
    EXPECT_EQ(TelemetryAccepted, response.acceptance);
}

TEST_F(TelemetryHttpTest, RejectsMissingContradictoryAndInvalidAcknowledgmentCounts)
{
    for (const char* body : {"{}", "{\"acc\":1}", "{\"acc\":\"1\",\"rej\":0}", "{\"acc\":true,\"rej\":0}",
        "{\"acc\":1,\"rej\":1}", "{\"acc\":0,\"rej\":0}", "{\"acc\":2,\"rej\":0}",
        "{\"acc\":0.5,\"rej\":0.5}", "{\"acc\":-1,\"rej\":2}", "{\"acc\":1e999,\"rej\":0}",
        "{\"acc\":1,\"acc\":0,\"rej\":0}", "{\"acc\":1,\"rej\":null}"})
    {
        SCOPED_TRACE(body);
        ExpectFailure(Response(body), EPROTO);
    }
}

TEST_F(TelemetryHttpTest, RejectsMalformedFailureIndicesAndFailureTypes)
{
    for (const char* efi : {"null", "[]", "{\"x\":false}", "{\"x\":\"some\"}", "{\"x\":[1]}",
        "{\"x\":[-1]}", "{\"x\":[0.5]}", "{\"x\":[\"0\"]}", "{\"x\":\"all\\u0000hidden\"}"})
    {
        SCOPED_TRACE(efi);
        ExpectFailure(Response(std::string("{\"acc\":1,\"rej\":0,\"efi\":") + efi + "}"), EPROTO);
    }
}

TEST_F(TelemetryHttpTest, RejectsTrailingJsonValuesAndEmbeddedNulls)
{
    ExpectFailure(Response(accepted + "{}"), EPROTO);
    ExpectFailure(Response(accepted + " trailing"), EPROTO);
    ExpectFailure(Response(accepted + std::string("\0hidden", 7)), EPROTO);
    ExpectFailure(Response("{\"acc\\u0000hidden\":1,\"rej\":0}"), EPROTO);
    ExpectFailure(Response("[{\"acc\":1,\"rej\":0}]"), EPROTO);
    ExpectFailure(Response("{\"acc\":1,\"rej\":0"), EPROTO);
    ASSERT_EQ(0, Parse(Response(" \t" + accepted + "\r\n")));
    EXPECT_EQ(TelemetryAccepted, response.acceptance);
    ASSERT_EQ(0, Parse(Response("{\"acc\":1,\"rej\":0,\"extra\":\"}\\\"[{\"}")));
    EXPECT_EQ(TelemetryAccepted, response.acceptance);
}

TEST_F(TelemetryHttpTest, BoundsJsonNestingBeforeCallingParson)
{
    const std::string prefix = "{\"acc\":1,\"rej\":0,\"extra\":";
    ASSERT_EQ(0, Parse(Response(prefix + std::string(15, '[') + "0" + std::string(15, ']') + "}")));
    ExpectFailure(Response(prefix + std::string(16, '[') + "0" + std::string(16, ']') + "}"), EMSGSIZE);
}

TEST_F(TelemetryHttpTest, RejectsInvalidUtf8AndNonJsonWhitespace)
{
    ASSERT_EQ(0, Parse(Response("{\"acc\":1,\"rej\":0,\"extra\":\"caf\xc3\xa9\"}")));
    EXPECT_EQ(TelemetryAccepted, response.acceptance);
    for (const char* text : {"\x80", "\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xc3"})
    {
        ExpectFailure(Response(std::string("{\"acc\":1,\"rej\":0,\"extra\":\"") + text + "\"}"), EPROTO);
    }
    ExpectFailure(Response("{\"acc\":1,\v\"rej\":0}"), EPROTO);
    ExpectFailure(Response("{\"acc\":1,\f\"rej\":0}"), EPROTO);
}

TEST_F(TelemetryHttpTest, PreservesRepeatedMixedCaseCollectorControlFields)
{
    const std::string headers =
        "rEtRy-AfTeR: 10\r\n"
        "Retry-After: Thu, 01 Oct 2026 20:00:00 GMT\r\n"
        "KiLl-ToKeNs: token-a\r\n"
        "kill-tokens: token-b:details\r\n"
        "kill-duration: 1200\r\n"
        "time-delta-millis: -500\r\n";
    ASSERT_EQ(0, Parse(Response(accepted, headers)));
    ASSERT_EQ(6U, response.controlCount);
    EXPECT_EQ(TelemetryRetryAfter, response.controls[0].kind);
    EXPECT_EQ(TelemetryRetryAfter, response.controls[1].kind);
    EXPECT_EQ(TelemetryKillTokens, response.controls[2].kind);
    EXPECT_EQ(TelemetryKillTokens, response.controls[3].kind);
    EXPECT_EQ(TelemetryKillDuration, response.controls[4].kind);
    EXPECT_EQ(TelemetryTimeDeltaMillis, response.controls[5].kind);
    EXPECT_STREQ("token-b:details", response.controlValues + response.controls[3].offset);
    EXPECT_STREQ("-500", response.controlValues + response.controls[5].offset);
}

TEST_F(TelemetryHttpTest, BoundsControlCountWithoutDiscardingDirectives)
{
    std::string headers;
    for (int i = 0; i < TELEMETRY_HTTP_CONTROL_LIMIT; ++i) headers += "kill-tokens: t\r\n";
    ASSERT_EQ(0, Parse(Response(accepted, headers)));
    EXPECT_EQ(static_cast<size_t>(TELEMETRY_HTTP_CONTROL_LIMIT), response.controlCount);
    ExpectFailure(Response(accepted, headers + "Retry-After: 1\r\n"), EMSGSIZE);
}

TEST_F(TelemetryHttpTest, RejectsAmbiguousAndUnsupportedFraming)
{
    ExpectFailure(Response(accepted, "Transfer-Encoding: chunked\r\n"), EPROTO);
    ExpectFailure(Response(accepted, "Content-Length: 3\r\n"), EPROTO);
    ExpectFailure("HTTP/1.1 200 OK\r\nContent-Length: 17,17\r\n\r\n", EPROTO);
    ExpectFailure("HTTP/1.1 200 OK\r\nContent-Length: +17\r\n\r\n", EPROTO);
    ExpectFailure("HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\n", EPROTO);
    ExpectFailure("HTTP/1.1 200 OK\r\nContent-Length: 999999999999999999999999\r\n\r\n", EOVERFLOW);
    ExpectFailure("HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip, chunked\r\n\r\n", ENOTSUP);
    ExpectFailure("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nTransfer-Encoding: chunked\r\n\r\n", EPROTO);
    ASSERT_EQ(0, Parse(Response(accepted, "Content-Length: " + std::to_string(accepted.size()) + "\r\n")));
    EXPECT_EQ(TelemetryAccepted, response.acceptance);
}

TEST_F(TelemetryHttpTest, RejectsUnexpectedResponseCompression)
{
    for (const char* encoding : {"gzip", "deflate", "br", "identity, gzip", ""})
    {
        ExpectFailure(Response(accepted, std::string("Content-Encoding: ") + encoding + "\r\n"), ENOTSUP);
    }
    ASSERT_EQ(0, Parse(Response(accepted, "Content-Encoding: Identity\r\n")));
    EXPECT_EQ(TelemetryAccepted, response.acceptance);
}

TEST_F(TelemetryHttpTest, RejectsMalformedStatusLinesAndHeaderSyntax)
{
    for (const char* wire : {"HTTP/2.0 200 OK\r\n\r\n", "HTTP/1.1 20 OK\r\n\r\n",
        "HTTP/1.1 600 Invalid\r\n\r\n", "HTTP/1.1 200OK\r\n\r\n", "HTTP/1.1 200 OK\n\n",
        "HTTP/1.1 200 OK\rX", "HTTP/1.1 200 OK\r\n Header: folded\r\n",
        "HTTP/1.1 200 OK\r\nHeader : bad\r\n", "HTTP/1.1 200 OK\r\nMissingColon\r\n",
        "HTTP/1.1 200 OK\r\n: empty\r\n", "HTTP/1.1 200 OK\r\nX: \x01\r\n"})
    {
        SCOPED_TRACE(wire);
        ExpectFailure(wire, EPROTO);
    }
    const std::string prefix = "HTTP/1.1 200 OK\r\nX: ";
    ExpectFailure(prefix + std::string("\0", 1) + "\r\n", EPROTO);
}

TEST_F(TelemetryHttpTest, BoundsInformationalResponsesAndRejectsUpgrades)
{
    std::string prefix;
    for (int i = 0; i < 4; ++i) prefix += "HTTP/1.1 103 Early Hints\r\nX-Hint: x\r\n\r\n";
    ASSERT_EQ(0, Parse(prefix + Response()));
    EXPECT_EQ(TelemetryAccepted, response.acceptance);
    ExpectFailure(prefix + "HTTP/1.1 100 Continue\r\n\r\n" + Response(), EMSGSIZE);
    ExpectFailure("HTTP/1.1 101 Switching Protocols\r\n\r\n", ENOTSUP);
    ExpectFailure("HTTP/1.1 100 Continue\r\nContent-Length: 0\r\n\r\n", EPROTO);
    ASSERT_EQ(0, Parse("HTTP/1.1 103 Early Hints\r\nkill-tokens: ignored\r\n\r\n" + Response()));
    EXPECT_EQ(0U, response.controlCount);
}

TEST_F(TelemetryHttpTest, RejectsTruncatedResponsesAtEveryPrefix)
{
    for (const std::string& wire : {Response(), Chunked(Chunk(accepted) + "0\r\n\r\n")})
    {
        for (size_t size = 0; size < wire.size(); ++size)
        {
            SCOPED_TRACE(size);
            ExpectFailure(wire.substr(0, size), EPROTO, true);
        }
    }
}

TEST_F(TelemetryHttpTest, RejectsExtraBytesAndRequiresReinitializationAfterFailure)
{
    ExpectFailure(Response() + "x", EPROTO);
    ExpectFailure(Chunked(Chunk(accepted) + "0\r\n\r\nx"), EPROTO);
    EXPECT_EQ(EINVAL, TelemetryHttpResponseFeed(&response, NULL, 0, false, NULL));
    ASSERT_EQ(0, Parse(Response()));
    EXPECT_EQ(EPROTO, TelemetryHttpResponseFeed(&response, "x", 1, false, NULL));
    EXPECT_EQ(TelemetryUnconfirmed, response.acceptance);
    ASSERT_EQ(0, Parse(Response()));
    EXPECT_TRUE(response.complete);
}

TEST_F(TelemetryHttpTest, RejectsMalformedChunksAndUnsafeTrailers)
{
    for (const char* chunks : {"z\r\n", "-1\r\n", "\r\n", "1x\r\n", "1;=x\r\n",
        "1;a=\r\n", "1;a=\"unterminated\r\n", "1\r\naXX", "0\r\nContent-Length: 0\r\n\r\n",
        "0\r\nTransfer-Encoding: chunked\r\n\r\n", "0\r\nConnection: close\r\n\r\n",
        "0\r\nContent-Encoding: gzip\r\n\r\n", "0\r\nkill-tokens: t\r\n\r\n",
        "0\r\nRetry-After: 10\r\n\r\n"})
    {
        SCOPED_TRACE(chunks);
        ExpectFailure(Chunked(chunks), EPROTO);
    }
    ExpectFailure(Chunked("ffffffffffffffffffffffff\r\n"), EOVERFLOW);
}

TEST_F(TelemetryHttpTest, EnforcesBodyLimitForAllFramingModes)
{
    const std::string maximum = accepted + std::string(TELEMETRY_HTTP_BODY_LIMIT - accepted.size(), ' ');
    ASSERT_EQ(0, Parse(Response(maximum)));
    EXPECT_EQ(static_cast<size_t>(TELEMETRY_HTTP_BODY_LIMIT), response.bodySize);
    ExpectFailure(Response(maximum + " "), EMSGSIZE);
    ASSERT_EQ(0, Parse(Chunked(Chunk(maximum) + "0\r\n\r\n")));
    ExpectFailure(Chunked(Chunk(maximum) + Chunk(" ") + "0\r\n\r\n"), EMSGSIZE);
    ASSERT_EQ(0, Parse("HTTP/1.1 200 OK\r\n\r\n" + maximum, true));
    ExpectFailure("HTTP/1.1 200 OK\r\n\r\n" + maximum + " ", EMSGSIZE, true);
}

TEST_F(TelemetryHttpTest, EnforcesLineHeaderCountAndTotalHeaderLimits)
{
    ASSERT_EQ(0, Parse(Response(accepted, "X: " + std::string(TELEMETRY_HTTP_LINE_LIMIT - 3, 'a') + "\r\n")));
    ExpectFailure(Response(accepted, "X: " + std::string(TELEMETRY_HTTP_LINE_LIMIT - 2, 'a') + "\r\n"), EMSGSIZE);
    std::string headers;
    for (int i = 0; i < 63; ++i) headers += "X: a\r\n";
    ASSERT_EQ(0, Parse(Response(accepted, headers)));
    ExpectFailure(Response(accepted, headers + "X: a\r\n"), EMSGSIZE);

    std::string wire = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(accepted.size()) + "\r\n";
    while (wire.size() < TELEMETRY_HTTP_HEADER_LIMIT - 2)
    {
        size_t size = std::min<size_t>(2000, TELEMETRY_HTTP_HEADER_LIMIT - 2 - wire.size());
        ASSERT_GE(size, 5U);
        wire += "X: " + std::string(size - 5, 'a') + "\r\n";
    }
    ASSERT_EQ(0, Parse(wire + "\r\n" + accepted));
    EXPECT_EQ(static_cast<size_t>(TELEMETRY_HTTP_HEADER_LIMIT), response.headerBytes);
    ExpectFailure(wire + "X: a\r\n\r\n" + accepted, EMSGSIZE);
}

TEST_F(TelemetryHttpTest, BoundsWireOverheadFromManyTinyChunks)
{
    std::string chunks;
    for (size_t i = 0; i < TELEMETRY_HTTP_BODY_LIMIT; ++i) chunks += "1\r\n \r\n";
    ExpectFailure(Chunked(chunks + "0\r\n\r\n"), EMSGSIZE);
    EXPECT_GT(response.wireBytes, static_cast<size_t>(TELEMETRY_HTTP_WIRE_LIMIT));
}
