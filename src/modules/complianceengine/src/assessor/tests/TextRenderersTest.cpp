// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <TextRenderers.hpp>
#include <gtest/gtest.h>
#include <string>

using ComplianceEngine::Assessor::RenderText;
using ComplianceEngine::Assessor::TextStyle;

namespace
{
bool Contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

const char* kResult = R"({"action":"Audit","timestamp":"2026-01-01T00:00:00Z","durationMs":5,"status":"NonCompliant",)"
                      R"("rules":[{"section":"1.1","ruleName":"RuleA","ruleId":"id-a","title":"Title A","status":"NonCompliant",)"
                      R"("parameters":{"mask":"0600"},)"
                      R"("indicators":[{"procedure":"P","status":"NonCompliant",)"
                      R"("indicators":[{"message":"bad thing","status":"NonCompliant"}]}]}]})";
} // namespace

TEST(TextRenderersTest, CommonHeaderAndFooterAreRendered)
{
    auto r = RenderText(kResult, TextStyle::CompactList);
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "Action: Audit"));
    EXPECT_TRUE(Contains(r.Value(), "Timestamp: 2026-01-01T00:00:00Z"));
    EXPECT_TRUE(Contains(r.Value(), "Rules:"));
    EXPECT_TRUE(Contains(r.Value(), "Duration: 5 ms"));
    EXPECT_TRUE(Contains(r.Value(), "Status: NonCompliant"));
    EXPECT_TRUE(Contains(r.Value(), "End of Report"));
}

TEST(TextRenderersTest, CompactListIsOneLinePerRuleWithoutIndicators)
{
    auto r = RenderText(kResult, TextStyle::CompactList);
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "  [NonCompliant] 1.1 RuleA"));
    // Compact output omits the indicator tree.
    EXPECT_FALSE(Contains(r.Value(), "bad thing"));
    EXPECT_FALSE(Contains(r.Value(), "ruleId="));
}

TEST(TextRenderersTest, NestedListRendersIndentedIndicatorTree)
{
    auto r = RenderText(kResult, TextStyle::NestedList);
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "  1.1 RuleA [NonCompliant]"));
    // depth 0 -> 4 spaces; depth 1 -> 6 spaces.
    EXPECT_TRUE(Contains(r.Value(), "    - P [NonCompliant]"));
    EXPECT_TRUE(Contains(r.Value(), "      - bad thing [NonCompliant]"));
}

TEST(TextRenderersTest, DebugRendersIdentityTitleParametersAndIndicators)
{
    auto r = RenderText(kResult, TextStyle::Debug);
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "1.1 RuleA (ruleId=id-a) [NonCompliant]"));
    EXPECT_TRUE(Contains(r.Value(), "    title: Title A"));
    EXPECT_TRUE(Contains(r.Value(), "    parameters: mask=0600"));
    EXPECT_TRUE(Contains(r.Value(), "    - P [NonCompliant]"));
    EXPECT_TRUE(Contains(r.Value(), "      - bad thing [NonCompliant]"));
}

TEST(TextRenderersTest, DebugSerializesNonStringParameterValues)
{
    // Numbers and booleans have no JSON string form; they must be serialized,
    // not dropped to an empty value (mirrors JUnit's NonStringParameter test).
    const char* result = R"({"action":"Audit","timestamp":"t","durationMs":0,"status":"NonCompliant",)"
                         R"("rules":[{"section":"1","ruleName":"R","ruleId":"id","title":"T","status":"NonCompliant",)"
                         R"("parameters":{"count":5,"enabled":true,"name":"root"},"indicators":[]}]})";
    auto r = RenderText(result, TextStyle::Debug);
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "count=5"));
    EXPECT_TRUE(Contains(r.Value(), "enabled=true"));
    EXPECT_TRUE(Contains(r.Value(), "name=root"));
    // The number/bool must not collapse to an empty value.
    EXPECT_FALSE(Contains(r.Value(), "count=,"));
    EXPECT_FALSE(Contains(r.Value(), "enabled=,"));
}

TEST(TextRenderersTest, InvalidJsonIsError)
{
    EXPECT_FALSE(RenderText("not json", TextStyle::CompactList).HasValue());
}

TEST(TextRenderersTest, MissingRulesArrayIsError)
{
    EXPECT_FALSE(RenderText(R"({"action":"Audit"})", TextStyle::Debug).HasValue());
}

TEST(TextRenderersTest, EmptyRulesRendersHeaderAndFooterOnly)
{
    auto r = RenderText(R"({"action":"Audit","timestamp":"t","durationMs":0,"status":"Compliant","rules":[]})", TextStyle::NestedList);
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "Rules:"));
    EXPECT_TRUE(Contains(r.Value(), "Status: Compliant"));
}
