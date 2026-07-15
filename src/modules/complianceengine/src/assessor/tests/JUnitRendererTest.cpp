// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <JUnitRenderer.hpp>
#include <gtest/gtest.h>
#include <string>

using ComplianceEngine::Assessor::RenderJUnit;

namespace
{
bool Contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}
} // namespace

TEST(JUnitRendererTest, EmptyRulesProduceEmptySuite)
{
    auto r = RenderJUnit(R"({"action":"Audit","rules":[]})", "mysuite");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
    EXPECT_TRUE(Contains(r.Value(), "<testsuite name=\"mysuite\" tests=\"0\" failures=\"0\" skipped=\"0\">"));
    EXPECT_TRUE(Contains(r.Value(), "</testsuites>"));
}

TEST(JUnitRendererTest, CompliantRuleIsBarePassingTestcase)
{
    const std::string json = R"({"rules":[{"section":"1.1","ruleName":"RuleA","status":"Compliant","indicators":[]}]})";
    auto r = RenderJUnit(json, "s");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "<testcase classname=\"1.1\" name=\"RuleA\"/>"));
    EXPECT_FALSE(Contains(r.Value(), "<failure"));
    EXPECT_TRUE(Contains(r.Value(), "tests=\"1\" failures=\"0\""));
}

TEST(JUnitRendererTest, NonCompliantRuleHasFailureWithIndicatorBody)
{
    const std::string json = R"({"rules":[{"section":"2.3","ruleName":"RuleB","status":"NonCompliant",)"
                             R"("indicators":[{"procedure":"AuditFailure","status":"NonCompliant",)"
                             R"("indicators":[{"message":"bad thing","status":"NonCompliant"}]}]}]})";
    auto r = RenderJUnit(json, "s");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "<testcase classname=\"2.3\" name=\"RuleB\">"));
    EXPECT_TRUE(Contains(r.Value(), "<failure message=\"Rule is non-compliant\" type=\"NonCompliant\">"));
    EXPECT_TRUE(Contains(r.Value(), "Indicators:"));
    EXPECT_TRUE(Contains(r.Value(), "- AuditFailure [NonCompliant]"));
    EXPECT_TRUE(Contains(r.Value(), "- bad thing [NonCompliant]"));
    EXPECT_TRUE(Contains(r.Value(), "tests=\"1\" failures=\"1\""));
}

TEST(JUnitRendererTest, NestedIndicatorsAreIndentedByDepth)
{
    const std::string json = R"({"rules":[{"section":"1","ruleName":"R","status":"NonCompliant",)"
                             R"("indicators":[{"message":"top","status":"Compliant",)"
                             R"("indicators":[{"message":"child","status":"Compliant"}]}]}]})";
    auto r = RenderJUnit(json, "s");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    // depth 0 -> two leading spaces; depth 1 -> four leading spaces.
    EXPECT_TRUE(Contains(r.Value(), "  - top [Compliant]"));
    EXPECT_TRUE(Contains(r.Value(), "    - child [Compliant]"));
}

TEST(JUnitRendererTest, ParametersAreRenderedWhenPresent)
{
    const std::string json = R"({"rules":[{"section":"1","ruleName":"R","status":"NonCompliant",)"
                             R"("parameters":{"mask":"0600","owner":"root"},"indicators":[]}]})";
    auto r = RenderJUnit(json, "s");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "Parameters:"));
    EXPECT_TRUE(Contains(r.Value(), "- mask: 0600"));
    EXPECT_TRUE(Contains(r.Value(), "- owner: root"));
}

TEST(JUnitRendererTest, XmlSpecialCharsAreEscapedInAttributesAndBody)
{
    const std::string json = R"({"rules":[{"section":"1&1","ruleName":"A & B <c> \"d\"","status":"NonCompliant",)"
                             R"("indicators":[{"message":"m<&>\"'","status":"NonCompliant"}]}]})";
    auto r = RenderJUnit(json, "s");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "classname=\"1&amp;1\""));
    EXPECT_TRUE(Contains(r.Value(), "name=\"A &amp; B &lt;c&gt; &quot;d&quot;\""));
    // Body content is escaped too.
    EXPECT_TRUE(Contains(r.Value(), "m&lt;&amp;&gt;&quot;&apos;"));
    // No raw unescaped angle brackets from the rule data leak into the document.
    EXPECT_FALSE(Contains(r.Value(), "<c>"));
}

TEST(JUnitRendererTest, SuiteNameIsEscaped)
{
    auto r = RenderJUnit(R"({"rules":[]})", "a\"b");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "name=\"a&quot;b\""));
}

TEST(JUnitRendererTest, MixedRulesCountFailuresCorrectly)
{
    const std::string json = R"({"rules":[)"
                             R"({"section":"1","ruleName":"A","status":"Compliant","indicators":[]},)"
                             R"({"section":"2","ruleName":"B","status":"NonCompliant","indicators":[]},)"
                             R"({"section":"3","ruleName":"C","status":"NonCompliant","indicators":[]}]})";
    auto r = RenderJUnit(json, "s");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "tests=\"3\" failures=\"2\""));
}

TEST(JUnitRendererTest, NotApplicableRuleIsSkipped)
{
    const std::string json = R"({"rules":[{"section":"4.1","ruleName":"R","status":"NotApplicable",)"
                             R"("indicators":[{"message":"n/a on this distro","status":"NotApplicable"}]}]})";
    auto r = RenderJUnit(json, "s");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "<skipped message=\"Rule is not applicable\">"));
    EXPECT_TRUE(Contains(r.Value(), "- n/a on this distro [NotApplicable]"));
    EXPECT_TRUE(Contains(r.Value(), "skipped=\"1\""));
    EXPECT_FALSE(Contains(r.Value(), "<failure"));
}

TEST(JUnitRendererTest, InvalidJsonIsError)
{
    EXPECT_FALSE(RenderJUnit("not json", "s").HasValue());
}

TEST(JUnitRendererTest, MissingRulesArrayIsError)
{
    EXPECT_FALSE(RenderJUnit(R"({"action":"Audit"})", "s").HasValue());
}

TEST(JUnitRendererTest, NonObjectRootIsError)
{
    EXPECT_FALSE(RenderJUnit(R"([1,2,3])", "s").HasValue());
}

TEST(JUnitRendererTest, ControlCharactersAreNeutralised)
{
    // A control character (U+0001) in a message must not corrupt the XML; it is
    // replaced with a space.
    const std::string json = R"({"rules":[{"section":"1","ruleName":"R","status":"NonCompliant",)"
                             R"("indicators":[{"message":"a\u0001b","status":"NonCompliant"}]}]})";
    auto r = RenderJUnit(json, "s");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "- a b [NonCompliant]"));
}

TEST(JUnitRendererTest, NonStringParameterIsSerialised)
{
    const std::string json = R"({"rules":[{"section":"1","ruleName":"R","status":"NonCompliant",)"
                             R"("parameters":{"count":5,"enabled":true},"indicators":[]}]})";
    auto r = RenderJUnit(json, "s");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "- count: 5"));
    EXPECT_TRUE(Contains(r.Value(), "- enabled: true"));
}

TEST(JUnitRendererTest, NonObjectIndicatorEntriesAreSkipped)
{
    // A malformed (non-object) indicator entry is skipped without crashing.
    const std::string json = R"({"rules":[{"section":"1","ruleName":"R","status":"NonCompliant",)"
                             R"("indicators":["junk",{"message":"real","status":"Compliant"}]}]})";
    auto r = RenderJUnit(json, "s");
    ASSERT_TRUE(r.HasValue()) << r.Error().message;
    EXPECT_TRUE(Contains(r.Value(), "- real [Compliant]"));
}
