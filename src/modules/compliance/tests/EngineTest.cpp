// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Engine.h"
#include "Evaluator.h"
#include "parson.h"

#include <gtest/gtest.h>

using compliance::Result;
using compliance::Error;
using compliance::Engine;
using compliance::JsonWrapper;
using compliance::action_func_t;

static Result<bool> auditFailure(std::map<std::string, std::string>, std::ostringstream&)
{
    return false;
}

static Result<bool> auditSuccess(std::map<std::string, std::string>, std::ostringstream&)
{
    return true;
}

class ComplianceEngineTest : public ::testing::Test
{
protected:
    Engine mEngine;
    std::map<std::string, std::pair<action_func_t, action_func_t>> mProcedureMap;
    // std::map<std::string, std::string> mParameters;

    void SetUp() override
    {
        mProcedureMap = {
            {"success", {auditSuccess, auditSuccess}},
            {"failure", {auditFailure, auditFailure}},
            {"NoAudit", {nullptr, auditSuccess}},
        };
        // mEngine.setProcedureMap(mProcedureMap);
    }
};

TEST_F(ComplianceEngineTest, MmiGet_InvalidArgument_1)
{
    auto result = mEngine.mmiGet(nullptr);
    ASSERT_FALSE(result);
}

TEST_F(ComplianceEngineTest, MmiGet_InvalidArgument_2)
{
    auto result = mEngine.mmiGet("");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Invalid object name"));
}

TEST_F(ComplianceEngineTest, MmiGet_InvalidArgument_3)
{
    auto result = mEngine.mmiGet("audit");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Rule name is empty"));
}

TEST_F(ComplianceEngineTest, MmiGet_InvalidArgument_4)
{
    auto result = mEngine.mmiGet("auditX");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Rule not found"));
}

TEST_F(ComplianceEngineTest, MmiGet_InvalidArgument_5)
{
    auto result = mEngine.mmiGet("auditNoAudit");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Rule not found"));
}

TEST_F(ComplianceEngineTest, MmiSet_InvalidArgument_1)
{
    auto result = mEngine.mmiSet(nullptr, "", 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Invalid argument"));
}

TEST_F(ComplianceEngineTest, MmiSet_InvalidArgument_2)
{
    auto result = mEngine.mmiSet("", nullptr, 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Invalid argument"));
}

TEST_F(ComplianceEngineTest, MmiSet_InvalidArgument_3)
{
    auto result = mEngine.mmiSet("", "", -1);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Invalid argument"));
}

TEST_F(ComplianceEngineTest, MmiSet_InvalidArgument_4)
{
    auto result = mEngine.mmiSet("", "", 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Invalid object name"));
}

TEST_F(ComplianceEngineTest, MmiSet_InvalidArgument_5)
{
    auto result = mEngine.mmiSet("procedure", "", 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Rule name is empty"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_1)
{
    auto result = mEngine.mmiSet("procedureX", "", 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Failed to parse JSON"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_2)
{
    std::string payload = "dGVzdA=="; // 'test' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Failed to parse JSON"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_3)
{
    std::string payload = "e30="; // '{}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Missing 'audit' object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_4)
{
    std::string payload = "W10="; // '[]' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Failed to parse JSON object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_5)
{
    std::string payload = "eyJhdWRpdCI6W119"; // '{"audit":[]}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("The 'audit' value is not an object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_6)
{
    std::string payload = "eyJhdWRpdCI6e30sICJwYXJhbWV0ZXJzIjoxMjN9"; // '{"audit":{}, "parameters":123}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("The 'parameters' value is not an object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_7)
{
    std::string payload = "eyJhdWRpdCI6e30sICJwYXJhbWV0ZXJzIjp7IksiOnt9fX0="; // '{"audit":{}, "parameters":{"K":{}}}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Failed to get parameter name and value"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_InvalidArgument_8)
{
    std::string payload = "eyJhdWRpdCI6e30sICJyZW1lZGlhdGUiOltdfQ=="; // '{"audit":{}, "remediate":[]}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("The 'remediate' value is not an object"));
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_1)
{
    std::string payload = "eyJhdWRpdCI6e319"; // '{"audit":{}}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.value());
}

TEST_F(ComplianceEngineTest, MmiSet_setProcedure_2)
{
    std::string payload = "eyJhdWRpdCI6e30sICJyZW1lZGlhdGUiOnt9fQ=="; // '{"audit":{}, "remediate":{}}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.value());
}

TEST_F(ComplianceEngineTest, MmiSet_initAudit_InvalidArgument_1)
{
    auto result = mEngine.mmiSet("initX", "", 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Out-of-order operation: procedure must be set first"));
}

TEST_F(ComplianceEngineTest, MmiSet_initAudit_InvalidArgument_2)
{
    auto result = mEngine.mmiSet("init", "", 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Rule name is empty"));
}

TEST_F(ComplianceEngineTest, MmiSet_initAudit_InvalidArgument_3)
{
    std::string payload = "eyJhdWRpdCI6e319"; // '{"audit":{}}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_TRUE(result && result.value());

    payload = "K=V";
    result = mEngine.mmiSet("initX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("User parameter 'K' not found"));
}

TEST_F(ComplianceEngineTest, MmiSet_initAudit_1)
{
    std::string payload = "eyJhdWRpdCI6e30sICJwYXJhbWV0ZXJzIjp7IksiOiJ2In19"; // '{"audit":{}, "parameters":{"K":"v"}}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_TRUE(result && result.value());

    payload = "K=V";
    result = mEngine.mmiSet("initX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_TRUE(result);
    EXPECT_TRUE(result.value());
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_InvalidArgument_1)
{
    auto result = mEngine.mmiSet("remediateX", "", 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Out-of-order operation: procedure must be set first"));
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_InvalidArgument_2)
{
    auto result = mEngine.mmiSet("remediate", "", 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Rule name is empty"));
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_InvalidArgument_3)
{
    std::string payload = "eyJhdWRpdCI6e319"; // '{"audit":{}}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_TRUE(result && result.value());

    result = mEngine.mmiSet("remediateX", "", 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Failed to get 'remediate' object"));
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_InvalidArgument_4)
{
    std::string payload = "eyJhdWRpdCI6e30sInJlbWVkaWF0ZSI6e319"; // '{"audit":{},"remediate":{}}' in base64
    auto result = mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_TRUE(result && result.value());

    payload = "K=V";
    result = mEngine.mmiSet("remediateX", payload.c_str(), static_cast<int>(payload.size()));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("User parameter 'K' not found"));
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_1)
{
    // '{"audit":{},"remediate":{"X":{}},"parameters":{"K":"v"}}' in base64
    std::string payload = "eyJhdWRpdCI6e30sInJlbWVkaWF0ZSI6eyJYIjp7fX0sInBhcmFtZXRlcnMiOnsiSyI6InYifX0=";
    ASSERT_TRUE(mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size())));

    // Result is reported by the Evaluator class
    auto result = mEngine.mmiSet("remediateX", "", 0);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Unknown function"));
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_2)
{
    // '{"audit":{},"remediate":{"allOf":[]}}' in base64
    std::string payload = "eyJhdWRpdCI6e30sInJlbWVkaWF0ZSI6eyJhbGxPZiI6W119fQ==";
    ASSERT_TRUE(mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size())));

    // Result is reported by the Evaluator class
    auto result = mEngine.mmiSet("remediateX", "", 0);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), true);
}

TEST_F(ComplianceEngineTest, MmiSet_executeRemediation_3)
{
    // '{"audit":{},"remediate":{"anyOf":[]}}' in base64
    std::string payload = "eyJhdWRpdCI6e30sInJlbWVkaWF0ZSI6eyJhbnlPZiI6W119fQ==";
    ASSERT_TRUE(mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size())));

    // Result is reported by the Evaluator class
    auto result = mEngine.mmiSet("remediateX", "", 0);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), false);
}

TEST_F(ComplianceEngineTest, MmiGet_1)
{
    // '{"audit":{"X":{}}}' in base64
    std::string payload = "eyJhdWRpdCI6eyJYIjp7fX19";
    ASSERT_TRUE(mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size())));

    // Result is reported by the Evaluator class
    auto result = mEngine.mmiGet("auditX");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, std::string("Unknown function"));
}

TEST_F(ComplianceEngineTest, MmiGet_2)
{
    // '{"audit":{"allOf":[]}}' in base64
    std::string payload = "eyJhdWRpdCI6eyJhbGxPZiI6W119fQ==";
    ASSERT_TRUE(mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size())));

    // Result is reported by the Evaluator class
    auto result = mEngine.mmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().result, true);
}

TEST_F(ComplianceEngineTest, MmiGet_3)
{
    // '{"audit":{"anyOf":[]}}' in base64
    std::string payload = "eyJhdWRpdCI6eyJhbnlPZiI6W119fQ==";
    ASSERT_TRUE(mEngine.mmiSet("procedureX", payload.c_str(), static_cast<int>(payload.size())));

    // Result is reported by the Evaluator class
    auto result = mEngine.mmiGet("auditX");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().result, false);
}
