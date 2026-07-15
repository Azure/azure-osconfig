// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <BenchmarkFormatter.hpp>
#include <BenchmarkInfo.h>
#include <Evaluator.h>
#include <JsonFormatter.hpp>
#include <Mof.hpp>
#include <gtest/gtest.h>
#include <map>
#include <parson.h>
#include <string>

using ComplianceEngine::Action;
using ComplianceEngine::LinuxDistribution;
using ComplianceEngine::Status;
using ComplianceEngine::BenchmarkFormatters::HostInfo;
using ComplianceEngine::BenchmarkFormatters::JsonFormatter;
using ComplianceEngine::MOF::Resource;

namespace
{
Resource MakeResource(const std::string& section, const std::string& resourceID, const std::string& ruleId, const std::string& ruleName)
{
    Resource r;
    r.resourceID = resourceID;
    r.ruleId = ruleId;
    r.ruleName = ruleName;
    r.benchmarkInfo.distribution = LinuxDistribution::Ubuntu;
    r.benchmarkInfo.version = "22.04";
    r.benchmarkInfo.benchmarkVersion = "v1.0.0";
    r.benchmarkInfo.section = section;
    return r;
}

// Owns a parsed JSON document and exposes the root object.
struct ParsedJson
{
    JSON_Value* value = nullptr;
    JSON_Object* object = nullptr;

    explicit ParsedJson(const std::string& text)
        : value(json_parse_string(text.c_str()))
    {
        if (value != nullptr)
        {
            object = json_value_get_object(value);
        }
    }
    ~ParsedJson()
    {
        if (value != nullptr)
        {
            json_value_free(value);
        }
    }
    ParsedJson(const ParsedJson&) = delete;
    ParsedJson& operator=(const ParsedJson&) = delete;
    ParsedJson(ParsedJson&& other) noexcept
        : value(other.value),
          object(other.object)
    {
        other.value = nullptr;
        other.object = nullptr;
    }
    ParsedJson& operator=(ParsedJson&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        if (value != nullptr)
        {
            json_value_free(value);
        }
        value = other.value;
        object = other.object;
        other.value = nullptr;
        other.object = nullptr;
        return *this;
    }
};
} // namespace

TEST(JsonFormatterTest, EnvelopeContainsRequiredTopLevelFields)
{
    JsonFormatter formatter;
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    auto result = formatter.Finish(Status::Compliant);
    ASSERT_TRUE(result.HasValue()) << result.Error().message;

    ParsedJson doc(result.Value());
    ASSERT_NE(doc.object, nullptr) << "output is not valid JSON: " << result.Value();

    EXPECT_NE(json_object_get_string(doc.object, "timestamp"), nullptr);
    EXPECT_STREQ(json_object_get_string(doc.object, "action"), "Audit");
    EXPECT_EQ(json_value_get_type(json_object_get_value(doc.object, "rules")), JSONArray);
    EXPECT_STREQ(json_object_get_string(doc.object, "status"), "Compliant");
    // durationMs is a number added by Finish.
    EXPECT_EQ(json_value_get_type(json_object_get_value(doc.object, "durationMs")), JSONNumber);
    // These were removed from the output.
    EXPECT_EQ(json_object_has_value(doc.object, "osconfigVersion"), 0);
    EXPECT_EQ(json_object_has_value(doc.object, "module"), 0);
}

TEST(JsonFormatterTest, RemediationActionIsLabelled)
{
    JsonFormatter formatter;
    ASSERT_FALSE(formatter.Begin(Action::Remediate).HasValue());
    auto result = formatter.Finish(Status::NonCompliant);
    ASSERT_TRUE(result.HasValue()) << result.Error().message;

    ParsedJson doc(result.Value());
    ASSERT_NE(doc.object, nullptr);
    EXPECT_STREQ(json_object_get_string(doc.object, "action"), "Remediation");
    EXPECT_STREQ(json_object_get_string(doc.object, "status"), "NonCompliant");
}

TEST(JsonFormatterTest, HostBlockOmittedWhenNotSet)
{
    JsonFormatter formatter;
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    auto result = formatter.Finish(Status::Compliant);
    ASSERT_TRUE(result.HasValue());

    ParsedJson doc(result.Value());
    ASSERT_NE(doc.object, nullptr);
    EXPECT_EQ(json_object_has_value(doc.object, "host"), 0) << "host must be absent when SetHostInfo was not called";
}

TEST(JsonFormatterTest, HostBlockIsEmittedWhenSet)
{
    JsonFormatter formatter;
    formatter.SetHostInfo(HostInfo{"aarch64", "ubuntu", "22.04"});
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    auto result = formatter.Finish(Status::Compliant);
    ASSERT_TRUE(result.HasValue());

    ParsedJson doc(result.Value());
    ASSERT_NE(doc.object, nullptr);
    JSON_Object* host = json_object_get_object(doc.object, "host");
    ASSERT_NE(host, nullptr) << "host block missing";
    EXPECT_STREQ(json_object_get_string(host, "arch"), "aarch64");
    EXPECT_STREQ(json_object_get_string(host, "distribution"), "ubuntu");
    EXPECT_STREQ(json_object_get_string(host, "distributionVersion"), "22.04");
}

TEST(JsonFormatterTest, AddEntryEmitsTitleRuleIdSectionRuleNameStatus)
{
    JsonFormatter formatter;
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    auto entry = MakeResource("1.1.1", "1.1.1 Ensure something", "1234abcd-0000-0000-0000-000000000000", "EnsureSomething");
    ASSERT_FALSE(formatter.AddEntry(entry, Status::Compliant, "[]", {}).HasValue());
    auto result = formatter.Finish(Status::Compliant);
    ASSERT_TRUE(result.HasValue()) << result.Error().message;

    ParsedJson doc(result.Value());
    ASSERT_NE(doc.object, nullptr);
    JSON_Array* rules = json_object_get_array(doc.object, "rules");
    ASSERT_NE(rules, nullptr);
    ASSERT_EQ(json_array_get_count(rules), 1u);
    JSON_Object* rule = json_array_get_object(rules, 0);
    ASSERT_NE(rule, nullptr);

    EXPECT_STREQ(json_object_get_string(rule, "title"), "1.1.1 Ensure something");
    EXPECT_STREQ(json_object_get_string(rule, "ruleId"), "1234abcd-0000-0000-0000-000000000000");
    EXPECT_STREQ(json_object_get_string(rule, "section"), "1.1.1");
    EXPECT_STREQ(json_object_get_string(rule, "ruleName"), "EnsureSomething");
    EXPECT_STREQ(json_object_get_string(rule, "status"), "Compliant");
    EXPECT_EQ(json_value_get_type(json_object_get_value(rule, "indicators")), JSONArray);
    EXPECT_EQ(json_value_get_type(json_object_get_value(rule, "parameters")), JSONObject);
    // The legacy alias must be gone.
    EXPECT_EQ(json_object_has_value(rule, "resourceID"), 0) << "resourceID must be renamed to title";
}

TEST(JsonFormatterTest, IndicatorsPayloadIsEmbeddedVerbatim)
{
    JsonFormatter formatter;
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    auto entry = MakeResource("2.3", "2.3 Rule", "id", "Rule");
    const std::string indicators = R"([{"message":"checked /etc/passwd","status":"Compliant"}])";
    ASSERT_FALSE(formatter.AddEntry(entry, Status::Compliant, indicators, {}).HasValue());
    auto result = formatter.Finish(Status::Compliant);
    ASSERT_TRUE(result.HasValue());

    ParsedJson doc(result.Value());
    ASSERT_NE(doc.object, nullptr);
    JSON_Array* rules = json_object_get_array(doc.object, "rules");
    ASSERT_EQ(json_array_get_count(rules), 1u);
    JSON_Object* rule = json_array_get_object(rules, 0);
    JSON_Array* ind = json_object_get_array(rule, "indicators");
    ASSERT_NE(ind, nullptr);
    ASSERT_EQ(json_array_get_count(ind), 1u);
    JSON_Object* first = json_array_get_object(ind, 0);
    EXPECT_STREQ(json_object_get_string(first, "message"), "checked /etc/passwd");
    EXPECT_STREQ(json_object_get_string(first, "status"), "Compliant");
}

TEST(JsonFormatterTest, NonCompliantEntryIsLabelled)
{
    JsonFormatter formatter;
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    auto entry = MakeResource("3.1", "3.1 Rule", "id", "Rule");
    ASSERT_FALSE(formatter.AddEntry(entry, Status::NonCompliant, "[]", {}).HasValue());
    auto result = formatter.Finish(Status::NonCompliant);
    ASSERT_TRUE(result.HasValue());

    ParsedJson doc(result.Value());
    JSON_Array* rules = json_object_get_array(doc.object, "rules");
    JSON_Object* rule = json_array_get_object(rules, 0);
    EXPECT_STREQ(json_object_get_string(rule, "status"), "NonCompliant");
}

TEST(JsonFormatterTest, NotApplicableEntryIsLabelled)
{
    JsonFormatter formatter;
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    auto entry = MakeResource("4.1", "4.1 Rule", "id", "Rule");
    ASSERT_FALSE(formatter.AddEntry(entry, Status::NotApplicable, "[]", {}).HasValue());
    auto result = formatter.Finish(Status::Compliant);
    ASSERT_TRUE(result.HasValue());

    ParsedJson doc(result.Value());
    JSON_Array* rules = json_object_get_array(doc.object, "rules");
    JSON_Object* rule = json_array_get_object(rules, 0);
    EXPECT_STREQ(json_object_get_string(rule, "status"), "NotApplicable");
}

TEST(JsonFormatterTest, MultipleEntriesArePreservedInOrder)
{
    JsonFormatter formatter;
    formatter.SetHostInfo(HostInfo{"x86_64", "ubuntu", "24.04"});
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    ASSERT_FALSE(formatter.AddEntry(MakeResource("1.1", "1.1 First", "id1", "First"), Status::Compliant, "[]", {}).HasValue());
    ASSERT_FALSE(formatter.AddEntry(MakeResource("1.2", "1.2 Second", "id2", "Second"), Status::NonCompliant, "[]", {}).HasValue());
    auto result = formatter.Finish(Status::NonCompliant);
    ASSERT_TRUE(result.HasValue());

    ParsedJson doc(result.Value());
    ASSERT_NE(doc.object, nullptr);
    JSON_Array* rules = json_object_get_array(doc.object, "rules");
    ASSERT_EQ(json_array_get_count(rules), 2u);
    EXPECT_STREQ(json_object_get_string(json_array_get_object(rules, 0), "ruleId"), "id1");
    EXPECT_STREQ(json_object_get_string(json_array_get_object(rules, 1), "ruleId"), "id2");
    EXPECT_STREQ(json_object_get_string(json_array_get_object(rules, 0), "section"), "1.1");
    EXPECT_STREQ(json_object_get_string(json_array_get_object(rules, 1), "section"), "1.2");
}

TEST(JsonFormatterTest, AddEntryRejectsNonArrayPayload)
{
    JsonFormatter formatter;
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    auto entry = MakeResource("1.1", "1.1 Rule", "id", "Rule");
    // A JSON object (not an array) must be rejected.
    EXPECT_TRUE(formatter.AddEntry(entry, Status::Compliant, "{}", {}).HasValue());
}

TEST(JsonFormatterTest, AddEntryRejectsMalformedPayload)
{
    JsonFormatter formatter;
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    auto entry = MakeResource("1.1", "1.1 Rule", "id", "Rule");
    EXPECT_TRUE(formatter.AddEntry(entry, Status::Compliant, "not json", {}).HasValue());
}

TEST(JsonFormatterTest, EffectiveParametersAreEmitted)
{
    JsonFormatter formatter;
    ASSERT_FALSE(formatter.Begin(Action::Audit).HasValue());
    auto entry = MakeResource("1.1", "1.1 Rule", "id", "Rule");
    const std::map<std::string, std::string> params{{"mask", "0600"}, {"owner", "root"}};
    ASSERT_FALSE(formatter.AddEntry(entry, Status::Compliant, "[]", params).HasValue());
    auto result = formatter.Finish(Status::Compliant);
    ASSERT_TRUE(result.HasValue());

    ParsedJson doc(result.Value());
    ASSERT_NE(doc.object, nullptr);
    JSON_Array* rules = json_object_get_array(doc.object, "rules");
    JSON_Object* rule = json_array_get_object(rules, 0);
    JSON_Object* p = json_object_get_object(rule, "parameters");
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(json_object_get_string(p, "mask"), "0600");
    EXPECT_STREQ(json_object_get_string(p, "owner"), "root");
}
