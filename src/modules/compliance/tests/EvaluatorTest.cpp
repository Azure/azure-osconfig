// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"

#include "JsonWrapper.h"
#include "parson.h"

#include <gtest/gtest.h>

using compliance::action_func_t;
using compliance::Error;
using compliance::Evaluator;
using compliance::JsonWrapper;
using compliance::Result;

static Result<bool> remediationFailure(std::map<std::string, std::string>, std::ostringstream&)
{
    return false;
}

static Result<bool> remediationSuccess(std::map<std::string, std::string>, std::ostringstream&)
{
    return true;
}

static Result<bool> auditFailure(std::map<std::string, std::string>, std::ostringstream&)
{
    return false;
}

static Result<bool> auditSuccess(std::map<std::string, std::string>, std::ostringstream&)
{
    return true;
}

static Result<bool> remediationParametrized(std::map<std::string, std::string> arguments, std::ostringstream&)
{
    auto it = arguments.find("result");
    if (it == arguments.end())
    {
        return Error("Missing 'result' parameter");
    }

    if (it->second == "success")
    {
        return true;
    }
    else if (it->second == "failure")
    {
        return false;
    }

    return Error("Invalid 'result' parameter");
}

class EvaluatorTest : public ::testing::Test
{
protected:
    std::map<std::string, std::pair<action_func_t, action_func_t>> mProcedureMap;
    std::map<std::string, std::string> mParameters;

    void SetUp() override
    {
        mProcedureMap = {{"auditSuccess", {auditSuccess, nullptr}}, {"auditFailure", {auditFailure, nullptr}},
            {"remediationSuccess", {nullptr, remediationSuccess}}, {"remediationFailure", {nullptr, remediationFailure}},
            {"remediationParametrized", {nullptr, remediationParametrized}}};
    }
};

TEST_F(EvaluatorTest, Contructor)
{
    Evaluator evaluator(nullptr, mParameters, nullptr);
    auto result = evaluator.ExecuteAudit(nullptr, nullptr);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("Payload or payloadSizeBytes is null"));
    result = evaluator.ExecuteRemediation();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("invalid json argument"));
}

TEST_F(EvaluatorTest, ExecuteAuditInvalidArguments)
{
    auto json = compliance::parseJSON("{}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator(json_value_get_object(json.get()), mParameters, nullptr);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator.ExecuteAudit(nullptr, nullptr);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("Payload or payloadSizeBytes is null"));

    result = evaluator.ExecuteAudit(&payload, nullptr);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("Payload or payloadSizeBytes is null"));

    result = evaluator.ExecuteAudit(nullptr, &payloadSizeBytes);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("Payload or payloadSizeBytes is null"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_1)
{
    auto json = compliance::parseJSON("{}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator(json_value_get_object(json.get()), mParameters, nullptr);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("Rule name or value is null"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_2)
{
    auto json = compliance::parseJSON("{\"anyOf\":null}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("anyOf value is not an array"));

    json = compliance::parseJSON("{\"anyOf\":{}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator2(json_value_get_object(json.get()), mParameters, nullptr);
    result = evaluator2.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("anyOf value is not an array"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_3)
{
    auto json = compliance::parseJSON("{\"allOf\":1234}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("allOf value is not an array"));

    json = compliance::parseJSON("{\"allOf\":{}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator2(json_value_get_object(json.get()), mParameters, nullptr);
    result = evaluator2.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("allOf value is not an array"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_4)
{
    auto json = compliance::parseJSON("{\"not\":\"foo\"}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("not value is not an object"));

    json = compliance::parseJSON("{\"not\":[]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator2(json_value_get_object(json.get()), mParameters, nullptr);
    result = evaluator2.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("not value is not an object"));
}

TEST_F(EvaluatorTest, ExecuteAudit_1)
{
    auto json = compliance::parseJSON("{\"allOf\":[]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
    ASSERT_NE(payload, nullptr);
    EXPECT_TRUE(payloadSizeBytes >= 4);
    EXPECT_EQ(0, strncmp(payload, "\"PASS", 5));
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteAudit_2)
{
    auto json = compliance::parseJSON("{\"allOf\":[{\"foo\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("Unknown function"));
}

TEST_F(EvaluatorTest, ExecuteAudit_3)
{
    auto json = compliance::parseJSON("{\"allOf\":[{\"auditSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
    ASSERT_NE(payload, nullptr);
    EXPECT_TRUE(payloadSizeBytes >= 4);
    EXPECT_EQ(0, strncmp(payload, "\"PASS", 5));
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteAudit_4)
{
    auto json = compliance::parseJSON("{\"allOf\":[{\"auditFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteAudit_5)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"auditFailure\":{}}, {\"auditSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteAudit_6)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"auditSuccess\":{}}, {\"auditFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteAudit_7)
{
    auto json = compliance::parseJSON("{\"allOf\":[{\"auditFailure\":{}}, {\"auditSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteAudit_8)
{
    auto json = compliance::parseJSON("{\"allOf\":[{\"auditSuccess\":{}}, {\"auditFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteAudit_9)
{
    auto json = compliance::parseJSON("{\"not\":{\"auditSuccess\":{}}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteAudit_10)
{
    auto json = compliance::parseJSON("{\"not\":{\"auditFailure\":{}}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteAudit_11)
{
    auto json = compliance::parseJSON("{\"not\":{\"not\":{\"auditFailure\":{}}}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteAudit_12)
{
    auto json = compliance::parseJSON("{\"allOf\":[{\"foo\":[]}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, std::string("value is not an object"));
}

TEST_F(EvaluatorTest, ExecuteRemediation_1)
{
    auto json = compliance::parseJSON("{\"allOf\":[]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
}

TEST_F(EvaluatorTest, ExecuteRemediation_2)
{
    auto json = compliance::parseJSON("{\"anyOf\":[]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
}

TEST_F(EvaluatorTest, ExecuteRemediation_3)
{
    auto json = compliance::parseJSON("{\"allOf\":[{\"remediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
}

TEST_F(EvaluatorTest, ExecuteRemediation_4)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
}

TEST_F(EvaluatorTest, ExecuteRemediation_5)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationFailure\":{}}, {\"remediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
}

TEST_F(EvaluatorTest, ExecuteRemediation_6)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationSuccess\":{}}, {\"remediationFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
}

TEST_F(EvaluatorTest, ExecuteRemediation_7)
{
    auto json = compliance::parseJSON("{\"allOf\":[{\"remediationFailure\":{}}, {\"remediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
}

TEST_F(EvaluatorTest, ExecuteRemediation_8)
{
    auto json = compliance::parseJSON("{\"allOf\":[{\"remediationSuccess\":{}}, {\"remediationFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
}

TEST_F(EvaluatorTest, ExecuteRemediation_9)
{
    auto json = compliance::parseJSON("{\"not\":{\"remediationSuccess\":{}}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteAudit_ProcedureMising_1)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationSuccess\":{}}, {\"auditFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteAudit_ProcedureMising_2)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"auditFailure\":{}}, {\"remediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteAudit_ProcedureMising_3)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"auditSuccess\":{}}, {\"remediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    auto result = evaluator1.ExecuteAudit(&payload, &payloadSizeBytes);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
    free(payload);
}

TEST_F(EvaluatorTest, ExecuteRemediation_ProcedureMising_1)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"foo\":{}}, {\"remediationFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteRemediation_ProcedureMising_2)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationSuccess\":{}}, {\"foo\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
}

TEST_F(EvaluatorTest, ExecuteRemediation_AuditFallback_1)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationFailure\":{}}, {\"auditSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
}

TEST_F(EvaluatorTest, ExecuteRemediation_AuditFallback_2)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationFailure\":{}}, {\"auditFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_1)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationParametrized\":{\"foo\":\"bar\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Missing 'result' parameter"));
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_2)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationParametrized\":{\"result\":\"bar\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_3)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationParametrized\":{\"result\":\"success\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_4)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationParametrized\":{\"result\":\"failure\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_5)
{
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationParametrized\":{\"result\":123}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Argument type is not a string"));
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_6)
{
    mParameters = {{"placeholder", "failure"}};
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationParametrized\":{\"result\":\"$placeholder\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_7)
{
    mParameters = {{"placeholder", "success"}};
    auto json = compliance::parseJSON("{\"anyOf\":[{\"remediationParametrized\":{\"result\":\"$placeholder\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
    evaluator1.setProcedureMap(mProcedureMap);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
}
