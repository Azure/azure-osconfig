#include "Evaluator.h"
#include "MockContext.h"

#include <EnsureApparmorProfiles.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using ComplianceEngine::AuditEnsureApparmorProfiles;
using ComplianceEngine::AuditEnsureApparmorProfilesParams;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ::testing::Return;

class EnsureApparmorProfilesTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree indicators;

    void SetUp() override
    {
        indicators.Push("EnsureApparmorProfiles");
    }
};

TEST_F(EnsureApparmorProfilesTest, AuditApparmorStatusCommandFails)
{
    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(Error("Command execution failed", -1))));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditNoProfilesLoaded)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "0 profiles are loaded.\n"
        "0 profiles are in enforce mode.\n"
        "0 profiles are in complain mode.\n"
        "0 processes have profiles defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditUnconfinedProcessesWithProfileDefined)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "35 profiles are loaded.\n"
        "16 profiles are in enforce mode.\n"
        "5 profiles are in complain mode.\n"
        "3 processes are unconfined but have a profile defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditComplainModeAllProfilesInComplainOrEnforce)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "35 profiles are loaded.\n"
        "16 profiles are in enforce mode.\n"
        "19 profiles are in complain mode.\n"
        "10 processes have profiles defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditComplainModeNotAllProfilesInComplainOrEnforce)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "35 profiles are loaded.\n"
        "16 profiles are in enforce mode.\n"
        "5 profiles are in complain mode.\n"
        "10 processes have profiles defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditEnforceModeAllProfilesInEnforce)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "35 profiles are loaded.\n"
        "35 profiles are in enforce mode.\n"
        "0 profiles are in complain mode.\n"
        "10 processes have profiles defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;
    params.enforce = true;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditEnforceModeNotAllProfilesInEnforce)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "35 profiles are loaded.\n"
        "30 profiles are in enforce mode.\n"
        "5 profiles are in complain mode.\n"
        "10 processes have profiles defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;
    params.enforce = true;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditEnforceModeWithComplainProfilesStillCompliant)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "35 profiles are loaded.\n"
        "25 profiles are in enforce mode.\n"
        "10 profiles are in complain mode.\n"
        "10 processes have profiles defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;
    params.enforce = true;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditMixedOutputParsing)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "Some random line.\n"
        "12 profiles are loaded.\n"
        "Another irrelevant line.\n"
        "8 profiles are in enforce mode.\n"
        "4 profiles are in complain mode.\n"
        "0 processes are unconfined but have a profile defined.\n"
        "Some other data.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditMinimalCompliantOutput)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "1 profiles are loaded.\n"
        "0 profiles are in enforce mode.\n"
        "1 profiles are in complain mode.\n"
        "0 processes are unconfined but have a profile defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditLargeNumbersInOutput)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "999 profiles are loaded.\n"
        "500 profiles are in enforce mode.\n"
        "499 profiles are in complain mode.\n"
        "0 processes are unconfined but have a profile defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditEmptyApparmorOutput)
{
    std::string apparmorOutput = "";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditOutputWithOnlyModuleInfo)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "Some other information.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditOutputWithPartialInformation)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "20 profiles are loaded.\n"
        "10 profiles are in enforce mode.\n";
    // Missing complain mode and unconfined processes lines

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditRealisticSampleOutput)
{
    // Based on the sample output provided by the user
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "35 profiles are loaded.\n"
        "16 profiles are in enforce mode.\n"
        "5 profiles are in complain mode.\n"
        "10 processes have profiles defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant); // 16+5=21 < 35 loaded profiles
}

TEST_F(EnsureApparmorProfilesTest, AuditEnforceModeWithRealisticSample)
{
    // Based on the sample output provided by the user
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "35 profiles are loaded.\n"
        "16 profiles are in enforce mode.\n"
        "5 profiles are in complain mode.\n"
        "10 processes have profiles defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;
    params.enforce = true;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant); // Only 16 out of 35 profiles in enforce mode
}

TEST_F(EnsureApparmorProfilesTest, AuditProfilesLoadedButNoModeSet)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "25 profiles are loaded.\n"
        "0 profiles are in enforce mode.\n"
        "0 profiles are in complain mode.\n"
        "0 processes are unconfined but have a profile defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant); // 0+0=0 < 25 loaded profiles
}

TEST_F(EnsureApparmorProfilesTest, AuditEdgeCaseZeroLoadedProfilesAllowed)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "0 profiles are loaded.\n"
        "0 profiles are in enforce mode.\n"
        "0 profiles are in complain mode.\n"
        "0 processes are unconfined but have a profile defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant); // No profiles loaded should be non-compliant
}

TEST_F(EnsureApparmorProfilesTest, AuditArgumentHandling)
{
    std::string apparmorOutput =
        "apparmor module is loaded.\n"
        "10 profiles are loaded.\n"
        "10 profiles are in enforce mode.\n"
        "0 profiles are in complain mode.\n"
        "0 processes are unconfined but have a profile defined.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;
    params.enforce = true;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureApparmorProfilesTest, AuditDifferentLineOrderings)
{
    std::string apparmorOutput =
        "35 profiles are loaded.\n"
        "apparmor module is loaded.\n"
        "0 processes are unconfined but have a profile defined.\n"
        "16 profiles are in enforce mode.\n"
        "19 profiles are in complain mode.\n";

    EXPECT_CALL(mContext, ExecuteCommand("apparmor_status")).WillOnce(Return(Result<std::string>(apparmorOutput)));

    AuditEnsureApparmorProfilesParams params;

    auto result = AuditEnsureApparmorProfiles(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant); // 16+19=35 equals loaded profiles
}
