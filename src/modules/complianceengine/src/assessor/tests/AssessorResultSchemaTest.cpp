// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Conformance checks tying the emitted canonical result to the shipped
// assessor-result.schema.json. There is no JSON-Schema validator wired into the
// build yet (the `validate` subcommand that performs full validation lands in a
// later phase), so this test drives the check from the schema's own `required`
// lists: it reads the shipped schema and asserts a generated result carries
// every field the schema declares required. That keeps the schema and the
// producer coupled without a validator dependency.

#include <BenchmarkFormatter.hpp>
#include <BenchmarkInfo.h>
#include <Evaluator.h>
#include <Mof.hpp>
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <parson.h>
#include <sstream>
#include <string>

#ifndef ASSESSOR_RESULT_SCHEMA_PATH
#error "ASSESSOR_RESULT_SCHEMA_PATH must be defined by the build."
#endif

using ComplianceEngine::Action;
using ComplianceEngine::Architecture;
using ComplianceEngine::LinuxDistribution;
using ComplianceEngine::Status;
using ComplianceEngine::BenchmarkFormatters::BenchmarkFormatter;
using ComplianceEngine::MOF::Resource;

namespace
{
std::string ReadFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

Resource MakeResource(const std::string& section, const std::string& title, const std::string& ruleId, const std::string& ruleName)
{
    Resource r;
    r.resourceID = title;
    r.ruleId = ruleId;
    r.ruleName = ruleName;
    r.benchmarkInfo.distribution = LinuxDistribution::Ubuntu;
    r.benchmarkInfo.version = "24.04";
    r.benchmarkInfo.benchmarkVersion = "v1.0.0";
    r.benchmarkInfo.section = section;
    return r;
}

// Fails the current test for every name in `required` missing from `object`.
void ExpectAllRequiredPresent(const JSON_Object* object, const JSON_Array* required, const std::string& context)
{
    ASSERT_NE(object, nullptr) << context << ": object missing";
    ASSERT_NE(required, nullptr) << context << ": schema 'required' array missing";
    const size_t count = json_array_get_count(required);
    for (size_t i = 0; i < count; ++i)
    {
        const char* name = json_array_get_string(required, i);
        ASSERT_NE(name, nullptr);
        EXPECT_TRUE(json_object_has_value(object, name)) << context << ": required field '" << name << "' is missing";
    }
}

// Produces a representative canonical result: host set, one compliant rule with
// no parameters and one non-compliant rule with parameters.
std::string GenerateResult()
{
    ComplianceEngine::DistributionInfo distInfo;
    distInfo.distribution = LinuxDistribution::Ubuntu;
    distInfo.architecture = Architecture::x86_64;
    distInfo.version = "24.04";
    auto formatterResult = BenchmarkFormatter::Begin(distInfo, Action::Audit);
    EXPECT_TRUE(formatterResult.HasValue());
    auto& formatter = formatterResult.Value();
    EXPECT_FALSE(formatter.AddEntry(MakeResource("1.1", "1.1 First", "id1", "First"), Status::Compliant, "[]", {}).HasValue());
    const std::map<std::string, std::string> params{{"mask", "0600"}};
    EXPECT_FALSE(formatter.AddEntry(MakeResource("1.2", "1.2 Second", "id2", "Second"), Status::NonCompliant, "[]", params).HasValue());
    auto result = std::move(formatter).Finish(Status::NonCompliant);
    EXPECT_TRUE(result.HasValue());
    return result.HasValue() ? result.Value() : std::string();
}
} // namespace

TEST(AssessorResultSchemaTest, SchemaFileIsValidDraft202012)
{
    const std::string schemaText = ReadFile(ASSESSOR_RESULT_SCHEMA_PATH);
    ASSERT_FALSE(schemaText.empty()) << "schema file not found at " << ASSESSOR_RESULT_SCHEMA_PATH;

    JSON_Value* schema = json_parse_string(schemaText.c_str());
    ASSERT_NE(schema, nullptr) << "schema is not valid JSON";
    JSON_Object* schemaObject = json_value_get_object(schema);
    ASSERT_NE(schemaObject, nullptr);

    const char* dialect = json_object_get_string(schemaObject, "$schema");
    ASSERT_NE(dialect, nullptr);
    EXPECT_NE(std::string(dialect).find("2020-12"), std::string::npos) << "expected draft 2020-12, got " << dialect;
    EXPECT_NE(json_object_get_array(schemaObject, "required"), nullptr);
    EXPECT_NE(json_object_dotget_array(schemaObject, "$defs.rule.required"), nullptr);

    json_value_free(schema);
}

TEST(AssessorResultSchemaTest, GeneratedResultSatisfiesSchemaRequiredFields)
{
    JSON_Value* schema = json_parse_string(ReadFile(ASSESSOR_RESULT_SCHEMA_PATH).c_str());
    ASSERT_NE(schema, nullptr);
    JSON_Object* schemaObject = json_value_get_object(schema);
    ASSERT_NE(schemaObject, nullptr);

    JSON_Value* result = json_parse_string(GenerateResult().c_str());
    ASSERT_NE(result, nullptr) << "generated result is not valid JSON";
    JSON_Object* resultObject = json_value_get_object(result);
    ASSERT_NE(resultObject, nullptr);

    // Top-level required fields.
    ExpectAllRequiredPresent(resultObject, json_object_get_array(schemaObject, "required"), "top-level");

    // host required fields.
    ExpectAllRequiredPresent(json_object_get_object(resultObject, "host"), json_object_dotget_array(schemaObject, "properties.host.required"), "host");

    // Each rule's required fields.
    const JSON_Array* ruleRequired = json_object_dotget_array(schemaObject, "$defs.rule.required");
    JSON_Array* rules = json_object_get_array(resultObject, "rules");
    ASSERT_NE(rules, nullptr);
    ASSERT_GT(json_array_get_count(rules), 0u);
    for (size_t i = 0; i < json_array_get_count(rules); ++i)
    {
        ExpectAllRequiredPresent(json_array_get_object(rules, i), ruleRequired, "rule[" + std::to_string(i) + "]");
    }

    // action must be one of the schema's enum values.
    const char* action = json_object_get_string(resultObject, "action");
    ASSERT_NE(action, nullptr);
    EXPECT_STREQ(action, "Audit");

    json_value_free(result);
    json_value_free(schema);
}
