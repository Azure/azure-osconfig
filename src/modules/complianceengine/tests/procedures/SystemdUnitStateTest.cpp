// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "MockContext.h"

#include <SystemdUnitState.h>
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>

using ComplianceEngine::AuditSystemdUnitState;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Pattern;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ComplianceEngine::SystemdUnitStateParams;

class SystemdUnitStateTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;

    void SetUp() override
    {
        mIndicators.Push("SystemdUnitState");
    }

    void TearDown() override
    {
    }
};

TEST_F(SystemdUnitStateTest, NullTest)
{

    SystemdUnitStateParams params;
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(SystemdUnitStateTest, argTestNoStateCheck)
{

    SystemdUnitStateParams params;
    params.unitName = "foo.service";

    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
}
std::string systemCtlCmd = "systemctl show ";

TEST_F(SystemdUnitStateTest, argTestActiveStateAnyMatch)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make(".*");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "ActiveState=inactive\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateNotMatch)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make("notMatch");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "ActiveState=inactive\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateNoOuptu)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make("notMatch");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "NotanActiveStateActiveState=inactive\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActive)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make("active");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "ActiveState=active\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateAny)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make("active");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());
    pattern = Pattern::Make(".*");
    ASSERT_TRUE(pattern.HasValue());
    params.LoadState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "ActiveState=active\nLoadState=masked";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateNotPresent)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make("active");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());
    pattern = Pattern::Make(".*");
    ASSERT_TRUE(pattern.HasValue());
    params.LoadState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "ActiveState=active\nExtraState=foo\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateMasked)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make("active");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());
    pattern = Pattern::Make("masked");
    ASSERT_TRUE(pattern.HasValue());
    params.LoadState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "ActiveState=active\nLoadState=masked\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateMaskedUnitFileStateAny)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make("active");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());
    pattern = Pattern::Make("masked");
    ASSERT_TRUE(pattern.HasValue());
    params.LoadState = std::move(pattern.Value());
    pattern = Pattern::Make(".*");
    ASSERT_TRUE(pattern.HasValue());
    params.UnitFileState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += "-p UnitFileState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "ActiveState=active\nLoadState=masked\nUnitFileState=masked";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateMaskedUnitFileStateAnyDiffrentOrder)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make("active");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());
    pattern = Pattern::Make("masked");
    ASSERT_TRUE(pattern.HasValue());
    params.LoadState = std::move(pattern.Value());
    pattern = Pattern::Make(".*");
    ASSERT_TRUE(pattern.HasValue());
    params.UnitFileState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += "-p UnitFileState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "LoadState=masked\nUnitFileState=masked\nActiveState=active";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateMaskedUnitFileStateOutputMissing)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make("active");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());
    pattern = Pattern::Make("masked");
    ASSERT_TRUE(pattern.HasValue());
    params.LoadState = std::move(pattern.Value());
    pattern = Pattern::Make(".*");
    ASSERT_TRUE(pattern.HasValue());
    params.UnitFileState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += "-p UnitFileState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "LoadState=masked\nNotAnUnitFileState=masked\nActiveState=active";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, argTestUnit)
{

    SystemdUnitStateParams params;
    params.unitName = "fooTimer.timer";
    auto pattern = Pattern::Make("foo.service");
    ASSERT_TRUE(pattern.HasValue());
    params.Unit = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p Unit ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "Unit=foo.service\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, partialMatchFails)
{

    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make("active");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "ActiveState=inactive";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, partialMatchSucceeds)
{
    SystemdUnitStateParams params;
    params.unitName = "fooArg.service";
    auto pattern = Pattern::Make(".*active");
    ASSERT_TRUE(pattern.HasValue());
    params.ActiveState = std::move(pattern.Value());

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += params.unitName;

    std::string fooServceAnyOutput = "ActiveState=inactive";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
