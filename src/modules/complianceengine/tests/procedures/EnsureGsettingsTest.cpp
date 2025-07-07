// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

using ComplianceEngine::AuditEnsureGsettings;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EnsureGsettings : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;

    const std::string gsettingsRangeCmd = "gsettings range ";
    const std::string gsettingsGetCmd = "gsettings get ";
    std::map<std::string, std::string> args;

    const std::string gsettingsTypeS = "type s\n";
    const std::string gsettingsTypeU = "type u\n";
    const std::string gsettingsTypeI = "type i\n";

    std::string GsettingsRangeCmd()
    {
        return gsettingsRangeCmd + "\"" + args["schema"] + "\" \"" + args["key"] + "\"";
    }

    std::string GsettingsGetCmd()
    {
        return gsettingsGetCmd + "\"" + args["schema"] + "\" \"" + args["key"] + "\"";
    }
    void SetUp() override
    {
        mIndicators.Push("EnsureGsettings");
    }

    void TearDown() override
    {
        args.clear();
    }
};

TEST_F(EnsureGsettings, AuditSuccessStringEqual)
{
    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-theme";
    args["keyType"] = "string";
    args["operation"] = "eq";
    args["value"] = "Adwaita";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeS)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("\"Adwaita\"")));
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditSuccessStringNotEqual)
{
    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-theme";
    args["keyType"] = "string";
    args["operation"] = "ne";
    args["value"] = "FOOOO";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeS)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("\"Adwaita\"")));
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
TEST_F(EnsureGsettings, AuditSuccessNumberTypeIEqual)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "number";
    args["operation"] = "eq";
    args["value"] = "1";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeI)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("1")));
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditSuccessNumberTypeUEqual)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "number";
    args["operation"] = "eq";
    args["value"] = "1";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeU)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("uint32 1\n")));
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditSuccessNumberTypeUOpreationLowerThan)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "number";
    args["operation"] = "lt";
    args["value"] = "10";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeU)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("uint32 9\n")));
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
TEST_F(EnsureGsettings, AuditSuccessNumberTypeUOpreationGreaterThan)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "number";
    args["operation"] = "gt";
    args["value"] = "42";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeU)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("uint32 420\n")));
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditSuccessNumberTypeIOpreationLowerThan)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "number";
    args["operation"] = "lt";
    args["value"] = "1337";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeI)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("42\n")));
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
TEST_F(EnsureGsettings, AuditSuccessNumberTypeUOpreationNotEqual)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "number";
    args["operation"] = "ne";
    args["value"] = "42";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeU)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("uint32 420\n")));
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditFailureNoArgs)
{

    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No schema arg provided");
}

TEST_F(EnsureGsettings, AuditFailureNoArgsKey)
{

    args["schema"] = "org.gnome.desktop.interface";
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No key arg provided");
}

TEST_F(EnsureGsettings, AuditFailureNoArgsKeyType)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No keyType arg provided");
}

TEST_F(EnsureGsettings, AuditFailureNoArgsOperation)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "string";
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No operation arg provided");
}

TEST_F(EnsureGsettings, AuditFailureNoValue)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "string";
    args["operation"] = "eq";
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No value arg provided");
}

TEST_F(EnsureGsettings, AuditFailureWongOperation)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "string";
    args["operation"] = "gt";
    args["value"] = "fooo bar qux";

    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Not supported operation gt");
}

TEST_F(EnsureGsettings, AuditFailureArgNotANumber)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "number";
    args["operation"] = "eq";
    args["value"] = "fooo bar qux";

    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid argument value: not a number " + args["value"]);
}

TEST_F(EnsureGsettings, AuditFailureReturnedNotNumber)
{

    args["schema"] = "org.gnome.desktop.interface";
    args["key"] = "cursor-size";
    args["keyType"] = "number";
    args["operation"] = "eq";
    args["value"] = "1337";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeI)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("MORE COFFEE")));
    auto result = AuditEnsureGsettings(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid operation value: not a number " + args["value"]);
}
