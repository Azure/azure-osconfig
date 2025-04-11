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
using compliance::Status;

class EvaluatorTest : public ::testing::Test
{
protected:
    std::map<std::string, std::string> mParameters;
};

TEST_F(EvaluatorTest, Contructor)
{
    Evaluator evaluator(nullptr, mParameters, nullptr);
    auto auditResult = evaluator.ExecuteAudit();
    ASSERT_FALSE(auditResult);
    ASSERT_EQ(auditResult.Error().message, std::string("invalid json argument"));
    auto remediationResult = evaluator.ExecuteRemediation();
    ASSERT_FALSE(remediationResult);
    ASSERT_EQ(remediationResult.Error().message, std::string("invalid json argument"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_1)
{
    auto json = compliance::ParseJson("{}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator.ExecuteAudit();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("Rule name or value is null"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_2)
{
    auto json = compliance::ParseJson("{\"anyOf\":null}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("anyOf value is not an array"));

    json = compliance::ParseJson("{\"anyOf\":{}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator2(json_value_get_object(json.get()), mParameters, nullptr);
    result = evaluator2.ExecuteAudit();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("anyOf value is not an array"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_3)
{
    auto json = compliance::ParseJson("{\"allOf\":1234}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("allOf value is not an array"));

    json = compliance::ParseJson("{\"allOf\":{}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator2(json_value_get_object(json.get()), mParameters, nullptr);
    result = evaluator2.ExecuteAudit();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("allOf value is not an array"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_4)
{
    auto json = compliance::ParseJson("{\"not\":\"foo\"}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("not value is not an object"));

    json = compliance::ParseJson("{\"not\":[]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator2(json_value_get_object(json.get()), mParameters, nullptr);
    result = evaluator2.ExecuteAudit();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("not value is not an object"));
}

TEST_F(EvaluatorTest, ExecuteAudit_1)
{
    auto json = compliance::ParseJson("{\"allOf\":[]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
    EXPECT_TRUE(result.Value().payload.find("PASS") == 0);
}

TEST_F(EvaluatorTest, ExecuteAudit_2)
{
    auto json = compliance::ParseJson("{\"allOf\":[{\"foo\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("Unknown function"));
}

TEST_F(EvaluatorTest, ExecuteAudit_3)
{
    auto json = compliance::ParseJson("{\"allOf\":[{\"AuditSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
    EXPECT_TRUE(result.Value().payload.find("PASS") == 0);
}

TEST_F(EvaluatorTest, ExecuteAudit_4)
{
    auto json = compliance::ParseJson("{\"allOf\":[{\"AuditFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_5)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"AuditFailure\":{}}, {\"AuditSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_6)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"AuditSuccess\":{}}, {\"AuditFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_7)
{
    auto json = compliance::ParseJson("{\"allOf\":[{\"AuditFailure\":{}}, {\"AuditSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_8)
{
    auto json = compliance::ParseJson("{\"allOf\":[{\"AuditSuccess\":{}}, {\"AuditFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_9)
{
    auto json = compliance::ParseJson("{\"not\":{\"AuditSuccess\":{}}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_10)
{
    auto json = compliance::ParseJson("{\"not\":{\"AuditFailure\":{}}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_11)
{
    auto json = compliance::ParseJson("{\"not\":{\"not\":{\"AuditFailure\":{}}}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_12)
{
    auto json = compliance::ParseJson("{\"allOf\":[{\"foo\":[]}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("value is not an object"));
}

TEST_F(EvaluatorTest, ExecuteRemediation_1)
{
    auto json = compliance::ParseJson("{\"allOf\":[]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_2)
{
    auto json = compliance::ParseJson("{\"anyOf\":[]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_3)
{
    auto json = compliance::ParseJson("{\"allOf\":[{\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_4)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_5)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationFailure\":{}}, {\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_6)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationSuccess\":{}}, {\"RemediationFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_7)
{
    auto json = compliance::ParseJson("{\"allOf\":[{\"RemediationFailure\":{}}, {\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_8)
{
    auto json = compliance::ParseJson("{\"allOf\":[{\"RemediationSuccess\":{}}, {\"RemediationFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_9)
{
    auto json = compliance::ParseJson("{\"not\":{\"RemediationSuccess\":{}}}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteAudit_ProcedureMising_1)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationSuccess\":{}}, {\"AuditFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteAudit_ProcedureMising_2)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"AuditFailure\":{}}, {\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteAudit_ProcedureMising_3)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"AuditSuccess\":{}}, {\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteAudit();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_ProcedureMising_1)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"foo\":{}}, {\"RemediationFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteRemediation_ProcedureMising_2)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationSuccess\":{}}, {\"foo\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_AuditFallback_1)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationFailure\":{}}, {\"AuditSuccess\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_AuditFallback_2)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationFailure\":{}}, {\"AuditFailure\":{}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_1)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationParametrized\":{\"foo\":\"bar\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Missing 'result' parameter"));
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_2)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":\"bar\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_3)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":\"success\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_4)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":\"failure\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_5)
{
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":123}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Argument type is not a string"));
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_6)
{
    mParameters = {{"placeholder", "failure"}};
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":\"$placeholder\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_7)
{
    mParameters = {{"placeholder", "success"}};
    auto json = compliance::ParseJson("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":\"$placeholder\"}}]}");
    ASSERT_TRUE(json.get());
    Evaluator evaluator1(json_value_get_object(json.get()), mParameters, nullptr);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}
