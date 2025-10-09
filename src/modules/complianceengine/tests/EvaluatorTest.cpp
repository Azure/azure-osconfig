// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"

#include "JsonWrapper.h"
#include "MockContext.h"
#include "parson.h"

#include <gtest/gtest.h>

using ComplianceEngine::action_func_t;
using ComplianceEngine::Error;
using ComplianceEngine::Evaluator;
using ComplianceEngine::JsonWrapper;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EvaluatorTest : public ::testing::Test
{
protected:
    std::map<std::string, std::string> mParameters;
    MockContext mContext;
    ComplianceEngine::DebugFormatter mFormatter;
};

TEST_F(EvaluatorTest, Contructor)
{
    Evaluator evaluator("test", nullptr, mParameters, mContext);
    auto auditResult = evaluator.ExecuteAudit(mFormatter);
    ASSERT_FALSE(auditResult);
    ASSERT_EQ(auditResult.Error().message, std::string("invalid json argument"));
    auto remediationResult = evaluator.ExecuteRemediation();
    ASSERT_FALSE(remediationResult);
    ASSERT_EQ(remediationResult.Error().message, std::string("invalid json argument"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_1)
{
    auto json = JsonWrapper::FromString("{}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("Rule name or value is null"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_2)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":null}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("anyOf value is not an array"));

    json = JsonWrapper::FromString("{\"anyOf\":{}}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator2("test", json_value_get_object(json->get()), mParameters, mContext);
    result = evaluator2.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("anyOf value is not an array"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_3)
{
    auto json = JsonWrapper::FromString("{\"allOf\":1234}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("allOf value is not an array"));

    json = JsonWrapper::FromString("{\"allOf\":{}}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator2("test", json_value_get_object(json->get()), mParameters, mContext);
    result = evaluator2.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("allOf value is not an array"));
}

TEST_F(EvaluatorTest, ExecuteAudit_InvalidJSON_4)
{
    auto json = JsonWrapper::FromString("{\"not\":\"foo\"}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("not value is not an object"));

    json = JsonWrapper::FromString("{\"not\":[]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator2("test", json_value_get_object(json->get()), mParameters, mContext);
    result = evaluator2.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("not value is not an object"));
}

TEST_F(EvaluatorTest, ExecuteAudit_1)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_2)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[{\"foo\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("Unknown function 'foo'"));
}

TEST_F(EvaluatorTest, ExecuteAudit_3)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[{\"AuditSuccess\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_4)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[{\"AuditFailure\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_5)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"AuditFailure\":{}}, {\"AuditSuccess\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_6)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"AuditSuccess\":{}}, {\"AuditFailure\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_7)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[{\"AuditFailure\":{}}, {\"AuditSuccess\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_8)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[{\"AuditSuccess\":{}}, {\"AuditFailure\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_9)
{
    auto json = JsonWrapper::FromString("{\"not\":{\"AuditSuccess\":{}}}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_10)
{
    auto json = JsonWrapper::FromString("{\"not\":{\"AuditFailure\":{}}}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_11)
{
    auto json = JsonWrapper::FromString("{\"not\":{\"not\":{\"AuditFailure\":{}}}}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteAudit_12)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[{\"foo\":[]}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.Error().message, std::string("invalid argument"));
}

TEST_F(EvaluatorTest, ExecuteRemediation_1)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_2)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_3)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[{\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_4)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_5)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationFailure\":{}}, {\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_6)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationSuccess\":{}}, {\"RemediationFailure\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_7)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[{\"RemediationFailure\":{}}, {\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_8)
{
    auto json = JsonWrapper::FromString("{\"allOf\":[{\"RemediationSuccess\":{}}, {\"RemediationFailure\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_9)
{
    auto json = JsonWrapper::FromString("{\"not\":{\"RemediationSuccess\":{}}}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteAudit_ProcedureMising_1)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationSuccess\":{}}, {\"AuditFailure\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteAudit_ProcedureMising_2)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"AuditFailure\":{}}, {\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteAudit_ProcedureMising_3)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"AuditSuccess\":{}}, {\"RemediationSuccess\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteAudit(mFormatter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_ProcedureMising_1)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"foo\":{}}, {\"RemediationFailure\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteRemediation_ProcedureMising_2)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationSuccess\":{}}, {\"foo\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_AuditFallback_1)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationFailure\":{}}, {\"AuditSuccess\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_AuditFallback_2)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationFailure\":{}}, {\"AuditFailure\":{}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_1)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationParametrized\":{\"foo\":\"bar\"}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Missing 'result' parameter"));
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_2)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":\"bar\"}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_3)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":\"success\"}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_4)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":\"failure\"}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_5)
{
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":123}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Argument type is not a string"));
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_6)
{
    mParameters = {{"placeholder", "failure"}};
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":\"$placeholder\"}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);
}

TEST_F(EvaluatorTest, ExecuteRemediation_Parameters_7)
{
    mParameters = {{"placeholder", "success"}};
    auto json = JsonWrapper::FromString("{\"anyOf\":[{\"RemediationParametrized\":{\"result\":\"$placeholder\"}}]}");
    ASSERT_TRUE(json.HasValue());
    ASSERT_TRUE(json->get());
    Evaluator evaluator1("test", json_value_get_object(json->get()), mParameters, mContext);

    auto result = evaluator1.ExecuteRemediation();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}
