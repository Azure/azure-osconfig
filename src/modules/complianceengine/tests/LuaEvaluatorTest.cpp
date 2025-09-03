// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "LuaEvaluator.h"

#include "Indicators.h"
#include "MockContext.h"
#include "Result.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

using ComplianceEngine::Action;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::LuaEvaluator;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class LuaEvaluatorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mIndicators.Push("LuaEvaluatorTest");
    }

    IndicatorsTree mIndicators;
    MockContext mContext;
};

// Test basic LuaEvaluator construction and destruction
TEST_F(LuaEvaluatorTest, Constructor)
{
    EXPECT_NO_THROW({ LuaEvaluator evaluator; });
}

// Test basic script execution - return true (compliant)
TEST_F(LuaEvaluatorTest, BasicScript_ReturnTrue)
{
    LuaEvaluator evaluator;
    const std::string script = "return true";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test basic script execution - return false (non-compliant)
TEST_F(LuaEvaluatorTest, BasicScript_ReturnFalse)
{
    LuaEvaluator evaluator;
    const std::string script = "return false";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

// Test script execution with boolean and message
TEST_F(LuaEvaluatorTest, BasicScript_ReturnBooleanWithMessage)
{
    LuaEvaluator evaluator;
    const std::string script = "return true, 'Custom success message'";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
    EXPECT_EQ(mIndicators.GetRootNode()->indicators.back().message, "Custom success message");
}

// Test script execution returning error string
TEST_F(LuaEvaluatorTest, BasicScript_ReturnErrorString)
{
    LuaEvaluator evaluator;
    const std::string script = "return 'This is an error message'";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().message, "This is an error message");
}

// Test script execution with no return value
TEST_F(LuaEvaluatorTest, BasicScript_NoReturn)
{
    LuaEvaluator evaluator;
    const std::string script = "local x = 42"; // No return statement

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().message, "Lua script did not return a value");
}

// Test invalid Lua script (compilation error)
TEST_F(LuaEvaluatorTest, InvalidScript_CompilationError)
{
    LuaEvaluator evaluator;
    const std::string script = "return true end"; // Syntax error

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);

    ASSERT_FALSE(result.HasValue());
    EXPECT_THAT(result.Error().message, ::testing::HasSubstr("Lua script compilation failed"));
}

// Test runtime error in Lua script
TEST_F(LuaEvaluatorTest, InvalidScript_RuntimeError)
{
    LuaEvaluator evaluator;
    const std::string script = "error('Runtime error test')";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);

    ASSERT_FALSE(result.HasValue());
    EXPECT_THAT(result.Error().message, ::testing::HasSubstr("Lua script execution failed"));
    EXPECT_THAT(result.Error().message, ::testing::HasSubstr("Runtime error test"));
}

// Test invalid return type
TEST_F(LuaEvaluatorTest, InvalidScript_InvalidReturnType)
{
    LuaEvaluator evaluator;
    const std::string script = "return 42"; // Invalid return type (number)

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().message, "Invalid return type from LUA script");
}

// Test security - ensure dangerous functions are not available
TEST_F(LuaEvaluatorTest, Security_DangerousFunctionsBlocked)
{
    LuaEvaluator evaluator;

    // Test that io.open is not available
    std::string script = "if io.open then return 'io.open available' else return true end";
    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    // Test that os.execute is not available
    script = "if os.execute then return 'os.execute available' else return true end";
    result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    // Test that load is not available
    script = "if load then return 'load available' else return true end";
    result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    // Test that dofile is not available
    script = "if dofile then return 'dofile available' else return true end";
    result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test security - ensure safe functions are available
TEST_F(LuaEvaluatorTest, Security_SafeFunctionsAvailable)
{
    LuaEvaluator evaluator;

    // Test that basic safe functions are available
    std::string script = R"(
        local result = true
        result = result and (type ~= nil)
        result = result and (tostring ~= nil)
        result = result and (tonumber ~= nil)
        result = result and (pairs ~= nil)
        result = result and (ipairs ~= nil)
        result = result and (next ~= nil)
        result = result and (pcall ~= nil)
        result = result and (xpcall ~= nil)
        result = result and (select ~= nil)
        return result
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test security - ensure safe string functions are available
TEST_F(LuaEvaluatorTest, Security_SafeStringFunctionsAvailable)
{
    LuaEvaluator evaluator;

    std::string script = R"(
        local result = true
        result = result and (string.byte ~= nil)
        result = result and (string.char ~= nil)
        result = result and (string.find ~= nil)
        result = result and (string.format ~= nil)
        result = result and (string.gsub ~= nil)
        result = result and (string.len ~= nil)
        result = result and (string.lower ~= nil)
        result = result and (string.match ~= nil)
        result = result and (string.rep ~= nil)
        result = result and (string.reverse ~= nil)
        result = result and (string.sub ~= nil)
        result = result and (string.upper ~= nil)
        return result
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test security - ensure safe table functions are available
TEST_F(LuaEvaluatorTest, Security_SafeTableFunctionsAvailable)
{
    LuaEvaluator evaluator;

    std::string script = R"(
        local result = true
        result = result and (table.concat ~= nil)
        result = result and (table.insert ~= nil)
        result = result and (table.remove ~= nil)
        result = result and (table.sort ~= nil)
        return result
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test security - ensure safe math functions are available
TEST_F(LuaEvaluatorTest, Security_SafeMathFunctionsAvailable)
{
    LuaEvaluator evaluator;

    std::string script = R"(
        local result = true
        result = result and (math.abs ~= nil)
        result = result and (math.floor ~= nil)
        result = result and (math.ceil ~= nil)
        result = result and (math.max ~= nil)
        result = result and (math.min ~= nil)
        result = result and (math.pi ~= nil)
        return result
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test security - ensure safe os functions are available
TEST_F(LuaEvaluatorTest, Security_SafeOsFunctionsAvailable)
{
    LuaEvaluator evaluator;

    std::string script = R"(
        local result = true
        result = result and (os.time ~= nil)
        result = result and (os.date ~= nil)
        result = result and (os.clock ~= nil)
        result = result and (os.difftime ~= nil)
        return result
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test procedure wrapper functionality with audit function
TEST_F(LuaEvaluatorTest, ProcedureWrapper_AuditFunction)
{
    LuaEvaluator evaluator;

    // Test calling AuditSuccess (should be available and return success)
    std::string script = R"(
        if AuditAuditSuccess then
            local compliant, message = AuditAuditSuccess({message = "test message"})
            if compliant then
                return true, "AuditAuditSuccess returned compliant: " .. message
            else
                return false, "AuditAuditSuccess returned non-compliant: " .. message
            end
        else
            return false, "AuditAuditSuccess function not available"
        end
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test procedure wrapper return values format
TEST_F(LuaEvaluatorTest, ProcedureWrapper_ReturnValueFormat)
{
    LuaEvaluator evaluator;

    // Test that procedures return exactly two values: boolean and string
    std::string script = R"(
        local results = {AuditAuditSuccess({message = "test"})}
        local count = #results

        if count == 2 then
            local compliant = results[1]
            local message = results[2]

            if type(compliant) == "boolean" and type(message) == "string" then
                return true, "Procedure returned correct format: boolean and string"
            else
                return false, "Procedure returned wrong types: " .. type(compliant) .. ", " .. type(message)
            end
        else
            return false, "Procedure returned " .. count .. " values, expected 2"
        end
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test procedure wrapper functionality with audit action restriction
TEST_F(LuaEvaluatorTest, ProcedureWrapper_AuditModeRestriction)
{
    LuaEvaluator evaluator;

    // Test that remediation functions are not available in audit mode
    std::string script = R"(
        -- Check if the remediation function exists
        if RemediationSuccess == nil then
            return true, "Remediation function correctly not available in audit mode"
        else
            -- If it exists, it should throw an error when called
            local success, message = pcall(function()
                return RemediationSuccess({message = "test"})
            end)

            if success then
                return false, "Expected remediation function to be blocked in audit mode"
            else
                return true, "Remediation function correctly threw error in audit mode: " .. tostring(message)
            end
        end
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test procedure wrapper functionality with remediation action
TEST_F(LuaEvaluatorTest, ProcedureWrapper_RemediationMode)
{
    LuaEvaluator evaluator;

    // Test that both audit and remediation functions are available in remediation mode
    std::string script = R"(
        local audit_available = (AuditAuditSuccess ~= nil)
        local remediate_available = (RemediateRemediationSuccess ~= nil)

        if audit_available and remediate_available then
            return true, "Both audit and remediation functions available"
        else
            return false, "Functions not properly available in remediation mode"
        end
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Remediate);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test complex Lua logic with multiple operations
TEST_F(LuaEvaluatorTest, ComplexScript_MultipleOperations)
{
    LuaEvaluator evaluator;

    std::string script = R"(
        local function check_compliance()
            -- Simulate complex compliance logic
            local checks = {}
            checks[1] = true  -- Some check passed
            checks[2] = true  -- Another check passed
            checks[3] = false -- This check failed

            local passed = 0
            local total = 0
            for _, check in ipairs(checks) do
                total = total + 1
                if check then
                    passed = passed + 1
                end
            end

            local threshold = 0.8  -- 80% pass rate required
            local pass_rate = passed / total

            return pass_rate >= threshold, string.format("Pass rate: %.2f", pass_rate)
        end

        return check_compliance()
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant); // Should fail due to 66% pass rate
}

// Test Lua script with table parameters
TEST_F(LuaEvaluatorTest, ComplexScript_TableOperations)
{
    LuaEvaluator evaluator;

    std::string script = R"(
        local data = {
            servers = {"web1", "web2", "db1"},
            ports = {80, 443, 3306},
            configs = {
                web = {enabled = true, secure = true},
                db = {enabled = true, secure = false}
            }
        }

        -- Check if all configs are secure
        local all_secure = true
        for service, config in pairs(data.configs) do
            if not config.secure then
                all_secure = false
                break
            end
        end

        return all_secure, "Security configuration check completed"
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant); // Should fail due to db.secure = false
}

// Test error handling in Lua wrapper
TEST_F(LuaEvaluatorTest, ProcedureWrapper_ErrorHandling)
{
    LuaEvaluator evaluator;

    // Test calling AuditFailure (should return non-compliant)
    std::string script = R"(
        if AuditFailure then
            return AuditFailure({})
        else
            return false, "AuditFailure function not available"
        end
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

// Test procedure wrapper throwing Lua errors for procedure failures
TEST_F(LuaEvaluatorTest, ProcedureWrapper_ThrowsErrorOnProcedureFailure)
{
    LuaEvaluator evaluator;

    // Test calling RemediationParametrized with invalid parameter (should throw Lua error)
    std::string script = R"(
        local success, message = pcall(function()
            return RemediateRemediationParametrized({result = "invalid"})
        end)

        if success then
            return false, "Expected procedure to throw error but it didn't"
        else
            -- Check that the error message contains expected text
            if string.find(message, "Invalid 'result' parameter") then
                return true, "Procedure correctly threw error: " .. message
            else
                return false, "Unexpected error message: " .. message
            end
        end
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Remediate);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test procedure wrapper throwing error for missing required parameter
TEST_F(LuaEvaluatorTest, ProcedureWrapper_ThrowsErrorOnMissingParameter)
{
    LuaEvaluator evaluator;

    // Test calling RemediationParametrized without required parameter
    std::string script = R"(
        local success, message = pcall(function()
            return RemediateRemediationParametrized({})
        end)

        if success then
            return false, "Expected procedure to throw error for missing parameter"
        else
            if string.find(message, "Missing 'result' parameter") then
                return true, "Procedure correctly threw error for missing parameter"
            else
                return false, "Unexpected error message: " .. message
            end
        end
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Remediate);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test procedure wrapper throwing error for remediation restriction in audit mode
TEST_F(LuaEvaluatorTest, ProcedureWrapper_ThrowsErrorOnRemediationRestriction)
{
    LuaEvaluator evaluator;

    // Test calling remediation function in audit mode (should not be available or throw error)
    std::string script = R"(
        -- Check if the function exists at all in audit mode
        if RemediationSuccess == nil then
            return true, "RemediationSuccess function correctly not available in audit mode"
        end

        -- If it exists, it should throw an error when called
        local success, message = pcall(function()
            return RemediationSuccess({message = "test"})
        end)

        if success then
            return false, "Expected remediation function to be blocked in audit mode, but it succeeded"
        else
            return true, "Remediation function correctly threw error in audit mode: " .. tostring(message)
        end
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test multiple return values handling
TEST_F(LuaEvaluatorTest, MultipleReturnValues)
{
    LuaEvaluator evaluator;

    std::string script = R"(
        return false, "Custom non-compliance message", "extra value"
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

// Test script execution performance with multiple evaluations
TEST_F(LuaEvaluatorTest, Performance_MultipleEvaluations)
{
    LuaEvaluator evaluator;

    const std::string script = R"(
        local sum = 0
        for i = 1, 1000 do
            sum = sum + i
        end
        return sum == 500500
    )";

    // Run the same script multiple times to test performance and stability
    for (int i = 0; i < 10; ++i)
    {
        auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
        ASSERT_TRUE(result.HasValue());
        EXPECT_EQ(result.Value(), Status::Compliant);
    }
}

// Test non-copyable nature of LuaEvaluator
TEST_F(LuaEvaluatorTest, NonCopyable)
{
    // This test ensures that LuaEvaluator is not copyable
    // by verifying that copy constructor and assignment operator are deleted
    EXPECT_FALSE(std::is_copy_constructible<LuaEvaluator>::value);
    EXPECT_FALSE(std::is_copy_assignable<LuaEvaluator>::value);
}

// Test that LuaEvaluator can be moved (if move semantics are implemented)
TEST_F(LuaEvaluatorTest, Movable)
{
    // Test that we can create and use LuaEvaluator in a way that might require moves
    auto createEvaluator = []() -> std::unique_ptr<LuaEvaluator> { return std::unique_ptr<LuaEvaluator>(new LuaEvaluator()); };

    auto evaluator = createEvaluator();
    ASSERT_NE(evaluator, nullptr);

    const std::string script = "return true";
    IndicatorsTree mIndicators;
    mIndicators.Push("MovableTest");
    MockContext mContext;

    auto result = evaluator->Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test edge case: empty script
TEST_F(LuaEvaluatorTest, EdgeCase_EmptyScript)
{
    LuaEvaluator evaluator;
    const std::string script = "";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);

    // Empty script should return an error as nothing is returned.
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().message, "Lua script did not return a value");
}

// Test edge case: very long script
TEST_F(LuaEvaluatorTest, EdgeCase_LongScript)
{
    LuaEvaluator evaluator;

    // Generate a long but valid script
    std::string script = "local result = true\n";
    for (int i = 0; i < 1000; ++i)
    {
        script += "result = result and true\n";
    }
    script += "return result";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// Test script with unicode characters
TEST_F(LuaEvaluatorTest, EdgeCase_UnicodeScript)
{
    LuaEvaluator evaluator;

    const std::string script = R"(
        local message = "Test with unicode: αβγ ñ é"
        return true, message
    )";

    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}
