// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Mof.hpp>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

using ComplianceEngine::Error;
using ComplianceEngine::Optional;
using ComplianceEngine::MOF::ParseAll;
using ComplianceEngine::MOF::Resource;

namespace
{

// Minimal valid MOF entry — body content only; the caller supplies the
// "instance of OsConfigResource as ..." header line where needed.
constexpr const char* kValidBody = R"(    ResourceID = "Some Rule";
    PayloadKey = "/cis/ubuntu/22.04/v2.0.0/1/1/1";
    ProcedureObjectName = "procedureMyRule";
    ProcedureObjectValue = "base64data==";
    InitObjectName = "initMyRule";
    ReportedObjectName = "auditMyRule";
    DesiredObjectValue = "mask=0600";
};
)";

// MOF entry with a JSON-format DesiredObjectValue (the format produced by the
// augmentation engine for parameterised rules, e.g. mount-point rules).
constexpr const char* kValidBodyJsonPayload =
    "    ResourceID = \"Some Rule\";\n"
    "    PayloadKey = \"/cis/rhel/8/v4.0.0/1/2/3\";\n"
    "    ProcedureObjectName = \"procedureMyRule\";\n"
    "    ProcedureObjectValue = \"base64data==\";\n"
    "    InitObjectName = \"initMyRule\";\n"
    "    ReportedObjectName = \"auditMyRule\";\n"
    "    DesiredObjectValue = \"{\\\"mountPoint\\\":\\\"/tmp\\\"}\";"
    "\n};\n";

std::string WrapEntries(std::size_t count, const std::string& body)
{
    std::string out;
    for (std::size_t i = 0; i < count; ++i)
    {
        out += "instance of OsConfigResource as $entry" + std::to_string(i) + "\n{\n";
        out += body;
    }
    return out;
}

} // namespace

TEST(MofParseSingleEntryTest, HappyPath)
{
    std::istringstream s(kValidBody);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    EXPECT_EQ(result.Value().resourceID, "Some Rule");
    EXPECT_EQ(result.Value().ruleName, "MyRule");
    EXPECT_EQ(result.Value().procedure, "base64data==");
    EXPECT_TRUE(result.Value().hasInitAudit);
    ASSERT_TRUE(result.Value().payload.HasValue());
    EXPECT_EQ(result.Value().payload.Value(), "mask=0600");
    // Section "/" -> "." transform
    EXPECT_EQ(result.Value().benchmarkInfo.section, "1.1.1");
}

TEST(MofParseSingleEntryTest, MissingResourceIDIsError)
{
    std::string body = kValidBody;
    auto pos = body.find("    ResourceID");
    auto end = body.find('\n', pos);
    body.erase(pos, end - pos + 1);
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_FALSE(result.HasValue());
    EXPECT_NE(result.Error().message.find("ResourceID"), std::string::npos);
}

TEST(MofParseSingleEntryTest, MissingPayloadKeyIsError)
{
    std::string body = kValidBody;
    auto pos = body.find("    PayloadKey");
    auto end = body.find('\n', pos);
    body.erase(pos, end - pos + 1);
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_FALSE(result.HasValue());
    EXPECT_NE(result.Error().message.find("PayloadKey"), std::string::npos);
}

TEST(MofParseSingleEntryTest, MissingReportedObjectNameIsError)
{
    std::string body = kValidBody;
    auto pos = body.find("    ReportedObjectName");
    auto end = body.find('\n', pos);
    body.erase(pos, end - pos + 1);
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_FALSE(result.HasValue());
    EXPECT_NE(result.Error().message.find("ReportedObjectName"), std::string::npos);
}

TEST(MofParseSingleEntryTest, MissingProcedureObjectValueIsError)
{
    std::string body = kValidBody;
    auto pos = body.find("    ProcedureObjectValue");
    auto end = body.find('\n', pos);
    body.erase(pos, end - pos + 1);
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_FALSE(result.HasValue());
    EXPECT_NE(result.Error().message.find("ProcedureObjectValue"), std::string::npos);
}

TEST(MofParseSingleEntryTest, InvalidInitObjectNamePrefixIsError)
{
    std::string body = kValidBody;
    auto pos = body.find("\"initMyRule\"");
    ASSERT_NE(pos, std::string::npos);
    body.replace(pos, std::string("\"initMyRule\"").size(), "\"wrongMyRule\"");
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_FALSE(result.HasValue());
}

TEST(MofParseSingleEntryTest, InvalidReportedObjectNamePrefixIsError)
{
    std::string body = kValidBody;
    auto pos = body.find("\"auditMyRule\"");
    ASSERT_NE(pos, std::string::npos);
    body.replace(pos, std::string("\"auditMyRule\"").size(), "\"reportMyRule\"");
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_FALSE(result.HasValue());
}

TEST(MofParseSingleEntryTest, DesiredObjectValueIsOptional)
{
    std::string body = kValidBody;
    auto pos = body.find("    DesiredObjectValue");
    auto end = body.find('\n', pos);
    body.erase(pos, end - pos + 1);
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    EXPECT_FALSE(result.Value().payload.HasValue());
}

TEST(MofParseSingleEntryTest, InitObjectNameOptional)
{
    std::string body = kValidBody;
    auto pos = body.find("    InitObjectName");
    auto end = body.find('\n', pos);
    body.erase(pos, end - pos + 1);
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_TRUE(result.HasValue());
    EXPECT_FALSE(result.Value().hasInitAudit);
}

TEST(MofParseSingleEntryTest, TruncatedInputDoesNotHang)
{
    // EOF before the "};" terminator — must not loop.
    std::string body = "    ResourceID = \"truncated\"\n    PayloadKey = \"/cis/ubuntu/22.04/v2.0.0/x\"\n";
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    // Outcome may be success or error depending on which required field is
    // missing; the contract is just "returns, does not hang".
    SUCCEED();
    (void)result;
}

TEST(MofParseSingleEntryTest, JsonEscapedDesiredObjectValue)
{
    // Production MOFs use JSON for DesiredObjectValue with MOF-escaped inner
    // quotes, e.g. "{\"mountPoint\":\"/tmp\"}".  GetValue must unescape
    // them so UpdateUserParameters receives valid JSON.
    std::istringstream s(kValidBodyJsonPayload);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value().payload.HasValue());
    EXPECT_EQ(result.Value().payload.Value(), "{\"mountPoint\":\"/tmp\"}");
}

TEST(MofParseSingleEntryTest, GetValueHandlesNoQuotes)
{
    // A line without quotes for ResourceID parses to an empty string.
    std::string body = "    ResourceID = no quotes here\n";
    body += "    PayloadKey = \"/cis/ubuntu/22.04/v2.0.0/x\"\n";
    body += "    ProcedureObjectValue = \"x\"\n";
    body += "    ReportedObjectName = \"auditR\"\n";
    body += "};\n";
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().resourceID, "");
}

TEST(MofParseAllTest, NullCallbackIsError)
{
    std::istringstream s("");
    auto err = ParseAll(s, nullptr);
    ASSERT_TRUE(err.HasValue());
}

TEST(MofParseAllTest, EmptyStreamProducesNoCallbacks)
{
    int calls = 0;
    std::istringstream s("");
    auto err = ParseAll(s, [&](Resource&&) {
        ++calls;
        return Optional<Error>();
    });
    EXPECT_FALSE(err.HasValue());
    EXPECT_EQ(calls, 0);
}

TEST(MofParseAllTest, StreamWithoutInstanceHeaderProducesNoCallbacks)
{
    int calls = 0;
    std::istringstream s("# just a comment\nother content\n");
    auto err = ParseAll(s, [&](Resource&&) {
        ++calls;
        return Optional<Error>();
    });
    EXPECT_FALSE(err.HasValue());
    EXPECT_EQ(calls, 0);
}

TEST(MofParseAllTest, MultipleEntries)
{
    int calls = 0;
    auto input = WrapEntries(3, kValidBody);
    std::istringstream s(input);
    auto err = ParseAll(s, [&](Resource&&) {
        ++calls;
        return Optional<Error>();
    });
    EXPECT_FALSE(err.HasValue()) << (err.HasValue() ? err.Value().message : "");
    EXPECT_EQ(calls, 3);
}

TEST(MofParseAllTest, CallbackErrorStopsParsing)
{
    int calls = 0;
    auto input = WrapEntries(3, kValidBody);
    std::istringstream s(input);
    auto err = ParseAll(s, [&](Resource&&) -> Optional<Error> {
        ++calls;
        return Error("stop");
    });
    ASSERT_TRUE(err.HasValue());
    EXPECT_EQ(err.Value().message, "stop");
    EXPECT_EQ(calls, 1);
}

TEST(MofParseAllTest, OverlongLineIsRejected)
{
    // Construct a single line longer than kMaxLineLength to confirm the
    // bound is enforced. Use a slightly-over limit to keep the test fast.
    std::string oversized(ComplianceEngine::MOF::kMaxLineLength + 1, 'x');
    oversized += "\n";
    std::istringstream s(oversized);
    auto err = ParseAll(s, [&](Resource&&) { return Optional<Error>(); });
    ASSERT_TRUE(err.HasValue());
}

TEST(MofParseAllTest, AdversarialJunkDoesNotCrash)
{
    // Loose parser must tolerate arbitrary bytes without UB.
    std::string junk;
    for (int i = 0; i < 4096; ++i)
    {
        junk.push_back(static_cast<char>(i & 0xff));
        if ((i & 0x3f) == 0)
        {
            junk.push_back('\n');
        }
    }
    std::istringstream s(junk);
    auto err = ParseAll(s, [&](Resource&&) { return Optional<Error>(); });
    // Outcome doesn't matter; the contract is "returns, does not crash."
    SUCCEED();
    (void)err;
}
