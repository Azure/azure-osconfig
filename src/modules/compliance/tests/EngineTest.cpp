// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Engine.h"

#include "Evaluator.h"
#include "parson.h"

#include <gtest/gtest.h>

using compliance::action_func_t;
using compliance::Engine;
using compliance::Error;
using compliance::JsonWrapper;
using compliance::Result;
using compliance::Status;

class ComplianceEngineTest : public ::testing::Test
{
public:
    ComplianceEngineTest()
        : mEngine(nullptr)
    {
    }

protected:
    Engine mEngine;
};

TEST_F(ComplianceEngineTest, MmiGet_InvalidArgument_1)
{
    auto result = mEngine.MmiGet(nullptr);
    ASSERT_FALSE(result);
}

TEST_F(ComplianceEngineTest, MmiGet_InvalidArgument_2)
{
    auto result = mEngine.MmiGet("");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Invalid object name"));
}

TEST_F(ComplianceEngineTest, MmiGet_InvalidArgument_3)
{
    auto result = mEngine.MmiGet("audit");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Rule name is empty"));
}

TEST_F(ComplianceEngineTest, MmiGet_InvalidArgument_4)
{
    auto result = mEngine.MmiGet("auditX");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Rule not found"));
}

TEST_F(ComplianceEngineTest, MmiGet_InvalidArgument_5)
{
    auto result = mEngine.MmiGet("auditNoAudit");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Rule not found"));
}

TEST_F(ComplianceEngineTest, MmiSet_InvalidArgument_1)
{
    auto result = mEngine.MmiSet(nullptr, "");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Invalid argument"));
}

TEST_F(ComplianceEngineTest, MmiSet_InvalidArgument_5)
{
    auto result = mEngine.MmiSet("procedure", "");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Rule name is empty"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_1)
{
    auto result = mEngine.MmiSet("procedureX", "");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Failed to parse JSON object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_2)
{
    std::string payload = "dGVzdA=="; // 'test' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Failed to parse JSON object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_3)
{
    std::string payload = "e30="; // '{}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Missing 'audit' object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_4)
{
    std::string payload = "W10="; // '[]' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Failed to parse JSON object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_5)
{
    std::string payload = "eyJhdWRpdCI6W119"; // '{"audit":[]}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("The 'audit' value is not an object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_6)
{
    std::string payload = "eyJhdWRpdCI6e30sICJwYXJhbWV0ZXJzIjoxMjN9"; // '{"audit":{}, "parameters":123}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("The 'parameters' value is not an object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_7)
{
    std::string payload = "eyJhdWRpdCI6e30sICJwYXJhbWV0ZXJzIjp7IksiOnt9fX0="; // '{"audit":{}, "parameters":{"K":{}}}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Failed to get parameter name and value"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_8)
{
    std::string payload = "eyJhdWRpdCI6e30sICJyZW1lZGlhdGUiOltdfQ=="; // '{"audit":{}, "remediate":[]}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("The 'remediate' value is not an object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_1)
{
    std::string payload = "eyJhdWRpdCI6e319"; // '{"audit":{}}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_2)
{
    std::string payload = "eyJhdWRpdCI6e30sICJyZW1lZGlhdGUiOnt9fQ=="; // '{"audit":{}, "remediate":{}}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_3)
{
    std::string payload = R"({"audit":{}, "remediate":{}})";
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_4)
{
    std::string payload = R"({ "audit": { } })";
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ComplianceEngineTest, MmiSet_initAudit_InvalidArgument_1)
{
    auto result = mEngine.MmiSet("initX", "");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Out-of-order operation: procedure must be set first"));
}

TEST_F(ComplianceEngineTest, MmiSet_initAudit_InvalidArgument_2)
{
    auto result = mEngine.MmiSet("init", "");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Rule name is empty"));
}

TEST_F(ComplianceEngineTest, MmiSet_initAudit_InvalidArgument_3)
{
    std::string payload = "eyJhdWRpdCI6e319"; // '{"audit":{}}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_TRUE(result && result.Value() == Status::Compliant);

    payload = "K=V";
    result = mEngine.MmiSet("initX", payload);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("User parameter 'K' not found"));
}

TEST_F(ComplianceEngineTest, MmiSet_initAudit_1)
{
    std::string payload = "eyJhdWRpdCI6e30sICJwYXJhbWV0ZXJzIjp7IksiOiJ2In19"; // '{"audit":{}, "parameters":{"K":"v"}}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_TRUE(result && result.Value() == Status::Compliant);

    payload = "K=V";
    result = mEngine.MmiSet("initX", payload);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_InvalidArgument_1)
{
    auto result = mEngine.MmiSet("remediateX", "");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Out-of-order operation: procedure must be set first"));
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_InvalidArgument_2)
{
    auto result = mEngine.MmiSet("remediate", "");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Rule name is empty"));
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_InvalidArgument_3)
{
    std::string payload = "eyJhdWRpdCI6e319"; // '{"audit":{}}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_TRUE(result && result.Value() == Status::Compliant);

    result = mEngine.MmiSet("remediateX", "");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Failed to get 'remediate' object"));
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_InvalidArgument_4)
{
    std::string payload = "eyJhdWRpdCI6e30sInJlbWVkaWF0ZSI6e319"; // '{"audit":{},"remediate":{}}' in base64
    auto result = mEngine.MmiSet("procedureX", payload);
    ASSERT_TRUE(result && result.Value() == Status::Compliant);

    payload = "K=V";
    result = mEngine.MmiSet("remediateX", payload);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("User parameter 'K' not found"));
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_1)
{
    // '{"audit":{},"remediate":{"X":{}},"parameters":{"K":"v"}}' in base64
    std::string payload = "eyJhdWRpdCI6e30sInJlbWVkaWF0ZSI6eyJYIjp7fX0sInBhcmFtZXRlcnMiOnsiSyI6InYifX0=";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // Result is reported by the Evaluator class
    auto result = mEngine.MmiSet("remediateX", "");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Unknown function"));
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_2)
{
    // '{"audit":{},"remediate":{"allOf":[]}}' in base64
    std::string payload = "eyJhdWRpdCI6e30sInJlbWVkaWF0ZSI6eyJhbGxPZiI6W119fQ==";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // Result is reported by the Evaluator class
    auto result = mEngine.MmiSet("remediateX", "");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_3)
{
    // '{"audit":{},"remediate":{"anyOf":[]}}' in base64
    std::string payload = "eyJhdWRpdCI6e30sInJlbWVkaWF0ZSI6eyJhbnlPZiI6W119fQ==";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // Result is reported by the Evaluator class
    auto result = mEngine.MmiSet("remediateX", "");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(ComplianceEngineTest, MmiGet_1)
{
    // '{"audit":{"X":{}}}' in base64
    std::string payload = "eyJhdWRpdCI6eyJYIjp7fX19";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // Result is reported by the Evaluator class
    auto result = mEngine.MmiGet("auditX");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Unknown function"));
}

TEST_F(ComplianceEngineTest, MmiGet_2)
{
    // '{"audit":{"allOf":[]}}' in base64
    std::string payload = "eyJhdWRpdCI6eyJhbGxPZiI6W119fQ==";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // Result is reported by the Evaluator class
    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::Compliant);
}

TEST_F(ComplianceEngineTest, MmiGet_3)
{
    // '{"audit":{"anyOf":[]}}' in base64
    std::string payload = "eyJhdWRpdCI6eyJhbnlPZiI6W119fQ==";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // Result is reported by the Evaluator class
    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().status, Status::NonCompliant);
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_1)
{
    std::string payload = R"({"audit":{},"parameters":{"KEY":"VALUE"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    auto result = mEngine.MmiSet("initX", "KEY=value");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_3)
{
    std::string payload = R"({"audit":{},"parameters":{"KEY":"VALUE"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    auto result = mEngine.MmiSet("initX", "1st=value");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Invalid key: first character must not be a digit"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_4)
{
    std::string payload = R"({"audit":{},"parameters":{"KEY":"VALUE"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // KEY_ not found in parameters, but is valid
    auto result = mEngine.MmiSet("initX", "KEY_=value");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("User parameter 'KEY_' not found"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_5)
{
    std::string payload = R"({"audit":{},"parameters":{"KEY":"VALUE"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // $ is not accepted in the key
    auto result = mEngine.MmiSet("initX", "KEY_$=value");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Invalid key: only alphanumeric and underscore characters are allowed"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_6)
{
    std::string payload = R"({"audit":{},"parameters":{"KEY":"VALUE"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // check if spaces are trimmed from the key
    auto result = mEngine.MmiSet("initX", "KEY_1 =  value");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Invalid key-value pair: '=' expected"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_7)
{
    std::string payload = R"({"audit":{},"parameters":{"KEY":"VALUE"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // check if spaces are trimmed from the key
    auto result = mEngine.MmiSet("initX", "KEY_1=  value");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.Error().message, std::string("Invalid key-value pair: missing value"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_1)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1"}},"parameters":{"KEY1":"VALUE1", "KEY2":"VALUE2"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    ASSERT_TRUE(mEngine.MmiSet("initX", "KEY1=value"));

    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().payload, R"(PASS{ auditGetParamValues: KEY1=value } == TRUE)");
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_2)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1", "KEY2": "$KEY2"}},"parameters":{"KEY1":"VALUE1", "KEY2":"VALUE2"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    ASSERT_TRUE(mEngine.MmiSet("initX", " KEY1=value  KEY2=value2   "));
    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().payload, R"(PASS{ auditGetParamValues: KEY1=value, KEY2=value2 } == TRUE)");
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_3)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1", "KEY2": "$KEY2"}},"parameters":{"KEY1":"VALUE1", "KEY2":"VALUE2"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    ASSERT_TRUE(mEngine.MmiSet("initX", R"( KEY1="  value" KEY2=value2   )"));
    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().payload, R"(PASS{ auditGetParamValues: KEY1=  value, KEY2=value2 } == TRUE)");
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_4)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1", "KEY2": "$KEY2"}},"parameters":{"KEY1":"VALUE1", "KEY2":"VALUE2"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // Check if the escaping backslash is erased and the single quote is preserved
    ASSERT_TRUE(mEngine.MmiSet("initX", R"(KEY1=" v " KEY2="value2\"")"));
    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().payload, R"(PASS{ auditGetParamValues: KEY1= v , KEY2=value2" } == TRUE)");
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_5)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1", "KEY2": "$KEY2"}},"parameters":{"KEY1":"VALUE1", "KEY2":"VALUE2"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // Check if the backslash is properly escaped
    ASSERT_TRUE(mEngine.MmiSet("initX", R"(KEY1=" v " KEY2="\\value2")"));
    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().payload, R"(PASS{ auditGetParamValues: KEY1= v , KEY2=\value2 } == TRUE)");
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_6)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1", "KEY2": "$KEY2"}},"parameters":{"KEY1":"VALUE1", "KEY2":"VALUE2"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // We don't treat the pair of backslashes as an escape sequence, so the result is expected to contain only one backslash
    ASSERT_TRUE(mEngine.MmiSet("initX", R"(KEY1=" v " KEY2="value2\\")"));
    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().payload, R"(PASS{ auditGetParamValues: KEY1= v , KEY2=value2\ } == TRUE)");
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_7)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1"}},"parameters":{"KEY1":"VALUE1"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    ASSERT_TRUE(mEngine.MmiSet("initX", R"(KEY1="")"));
    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().payload, R"(PASS{ auditGetParamValues: KEY1= } == TRUE)");
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_8)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1"}},"parameters":{"KEY1":"VALUE1"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // Unterminated value
    ASSERT_FALSE(mEngine.MmiSet("initX", R"(KEY1=")"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_9)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1"}},"parameters":{"KEY1":"VALUE1"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    ASSERT_FALSE(mEngine.MmiSet("initX", R"(KEY1=""")"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_10)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1"}},"parameters":{"KEY1":"VALUE1"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    ASSERT_FALSE(mEngine.MmiSet("initX", R"(KEY1="x)"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_11)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"k1": "$KEY1"}},"parameters":{"k1":"VALUE1"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // middle spaces handling
    ASSERT_FALSE(mEngine.MmiSet("initX", R"(k1 )"));
    ASSERT_FALSE(mEngine.MmiSet("initX", R"(k1= )"));
    ASSERT_FALSE(mEngine.MmiSet("initX", R"(k1=)"));
    ASSERT_FALSE(mEngine.MmiSet("initX", R"(k1 =)"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_12)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"k1": "$KEY1"}},"parameters":{"k1":"VALUE1"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // invalid escape character
    ASSERT_FALSE(mEngine.MmiSet("initX", R"(k1="x\y")"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_13)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"k1": "$KEY1"}},"parameters":{"k1":"VALUE1"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    // invalid escape sequence - backslash at the end of the string
    ASSERT_FALSE(mEngine.MmiSet("initX", R"(k1="x\)"));
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_14)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1", "KEY2": "$KEY2", "KEY3": "$KEY3"}},"parameters":{"KEY1":"v1", "KEY2":"v2", "KEY3":"v3"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    ASSERT_TRUE(mEngine.MmiSet("initX", R"(KEY1="x" KEY2='y' KEY3=z)"));
    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().payload, R"(PASS{ auditGetParamValues: KEY1=x, KEY2=y, KEY3=z } == TRUE)");
}

TEST_F(ComplianceEngineTest, MmiSet_externalParams_value_15)
{
    std::string payload = R"({"audit":{"auditGetParamValues":{"KEY1": "$KEY1", "KEY2": "$KEY2"}},"parameters":{"KEY1":"v1", "KEY2":"v2"}})";
    ASSERT_TRUE(mEngine.MmiSet("procedureX", payload));

    ASSERT_TRUE(mEngine.MmiSet("initX", R"(KEY1="'x'" KEY2='"y"')"));
    auto result = mEngine.MmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.Value().payload, R"(PASS{ auditGetParamValues: KEY1='x', KEY2="y" } == TRUE)");
}
