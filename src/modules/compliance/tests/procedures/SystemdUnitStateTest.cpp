// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>

using compliance::AuditEnsureSysctl;
using compliance::CompactListFormatter;
using compliance::Error;
using compliance::IndicatorsTree;
using compliance::Result;
using compliance::Status;

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

    std::map<std::string, std::string> args;

    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, argTestNoSateChec)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "foo.service";

    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, argTestInvalidStateScheckArg)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "foo.service";
    args["improper arg state to check for systedm service"] = "are you sure";

    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
std::string systemCtlCmd = "systemctl show ";

TEST_F(SystemdUnitStateTest, argTestActiveStateAnyMatch)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = ".*";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "ActiveState=inactive\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateNotMatch)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = "notMatch";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "ActiveState=inactive\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateNoOuptu)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = "notMatch";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "NotanActiveStateActiveState=inactive\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActive)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = "active";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "ActiveState=active\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateAny)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = "active";
    args["LoadState"] = ".*";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "ActiveState=active\nLoadState=masked";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateNotPresent)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = "active";
    args["LoadState"] = ".*";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "ActiveState=active\nExtraState=foo\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateMasked)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = "active";
    args["LoadState"] = "masked";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "ActiveState=active\nLoadState=masked\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateMaskedUnitFileStateAny)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = "active";
    args["LoadState"] = "masked";
    args["UnitFileState"] = ".*";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += "-p UnitFileState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "ActiveState=active\nLoadState=masked\nUnitFileState=masked";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateMaskedUnitFileStateAnyDiffrentOrder)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = "active";
    args["LoadState"] = "masked";
    args["UnitFileState"] = ".*";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += "-p UnitFileState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "LoadState=masked\nUnitFileState=masked\nActiveState=active";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, argTestActiveStateActiveLoadStateMaskedUnitFileStateOutputMissing)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = "active";
    args["LoadState"] = "masked";
    args["UnitFileState"] = ".*";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += "-p LoadState ";
    executeCmd += "-p UnitFileState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "LoadState=masked\nNotAnUnitFileState=masked\nActiveState=active";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, argTestUnit)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooTimer.timer";
    args["Unit"] = "foo.service";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p Unit ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "Unit=foo.service\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdUnitStateTest, partialMatchFails)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = "active";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "ActiveState=inactive";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdUnitStateTest, partialMatchSucceeds)
{

    std::map<std::string, std::string> args;
    args["unitName"] = "fooArg.service";
    args["ActiveState"] = ".*active";

    auto executeCmd = systemCtlCmd;
    executeCmd += "-p ActiveState ";
    executeCmd += args["unitName"];

    std::string fooServceAnyOutput = "ActiveState=inactive";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(executeCmd))).WillOnce(::testing::Return(Result<std::string>(fooServceAnyOutput)));
    auto result = AuditSystemdUnitState(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
