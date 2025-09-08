// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"

#include <EnsureGsettings.h>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>

using ComplianceEngine::AuditEnsureGsettings;
using ComplianceEngine::EnsureGsettingsParams;
using ComplianceEngine::Error;
using ComplianceEngine::GsettingsKeyType;
using ComplianceEngine::GsettingsOperationType;
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
    const std::string gsettingsWritableCmd = "gsettings writable ";
    EnsureGsettingsParams mParams;

    const std::string gsettingsTypeS = "type s\n";
    const std::string gsettingsTypeU = "type u\n";
    const std::string gsettingsTypeI = "type i\n";

    std::string GsettingsRangeCmd()
    {
        return gsettingsRangeCmd + "\"" + mParams.schema + "\" \"" + mParams.key + "\"";
    }

    std::string GsettingsGetCmd()
    {
        return gsettingsGetCmd + "\"" + mParams.schema + "\" \"" + mParams.key + "\"";
    }

    std::string GsettingsWritableCmd()
    {
        return gsettingsWritableCmd + "\"" + mParams.schema + "\" \"" + mParams.key + "\"";
    }

    void SetUp() override
    {
        mIndicators.Push("EnsureGsettings");
    }
};

TEST_F(EnsureGsettings, AuditSuccessStringEqual)
{
    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-theme";
    mParams.keyType = GsettingsKeyType::String;
    mParams.operation = GsettingsOperationType::Equal;
    mParams.value = "Adwaita";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeS)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("\"Adwaita\"")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditSuccessStringNotEqual)
{
    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-theme";
    mParams.keyType = GsettingsKeyType::String;
    mParams.operation = GsettingsOperationType::NotEqual;
    mParams.value = "FOOOO";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeS)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("\"Adwaita\"")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
TEST_F(EnsureGsettings, AuditSuccessNumberTypeIEqual)
{
    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-size";
    mParams.keyType = GsettingsKeyType::Number;
    mParams.operation = GsettingsOperationType::Equal;
    mParams.value = "1";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeI)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("1")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditSuccessNumberTypeUEqual)
{

    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-size";
    mParams.keyType = GsettingsKeyType::Number;
    mParams.operation = GsettingsOperationType::Equal;
    mParams.value = "1";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeU)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("uint32 1\n")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditSuccessNumberTypeUOpreationLowerThan)
{

    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-size";
    mParams.keyType = GsettingsKeyType::Number;
    mParams.operation = GsettingsOperationType::LessThan;
    mParams.value = "10";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeU)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("uint32 9\n")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
TEST_F(EnsureGsettings, AuditSuccessNumberTypeUOpreationGreaterThan)
{

    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-size";
    mParams.keyType = GsettingsKeyType::Number;
    mParams.operation = GsettingsOperationType::GreaterThan;
    mParams.value = "42";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeU)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("uint32 420\n")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditSuccessNumberTypeIOpreationLowerThan)
{

    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-size";
    mParams.keyType = GsettingsKeyType::Number;
    mParams.operation = GsettingsOperationType::LessThan;
    mParams.value = "1337";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeI)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("42\n")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
TEST_F(EnsureGsettings, AuditSuccessNumberTypeUOpreationNotEqual)
{

    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-size";
    mParams.keyType = GsettingsKeyType::Number;
    mParams.operation = GsettingsOperationType::NotEqual;
    mParams.value = "42";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeU)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("uint32 420\n")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditFailureWongOperation)
{

    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-size";
    mParams.keyType = GsettingsKeyType::String;
    mParams.operation = GsettingsOperationType::GreaterThan;
    mParams.value = "fooo bar qux";

    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Unsupported operation gt");
}

TEST_F(EnsureGsettings, AuditFailureArgNotANumber)
{

    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-size";
    mParams.keyType = GsettingsKeyType::Number;
    mParams.operation = GsettingsOperationType::Equal;
    mParams.value = "fooo bar qux";

    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid argument value: not a number " + mParams.value);
}

TEST_F(EnsureGsettings, AuditFailureReturnedNotNumber)
{

    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-size";
    mParams.keyType = GsettingsKeyType::Number;
    mParams.operation = GsettingsOperationType::Equal;
    mParams.value = "1337";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeI)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsGetCmd())).WillOnce(::testing::Return(Result<std::string>("MORE COFFEE")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid operation value: not a number " + mParams.value);
}

TEST_F(EnsureGsettings, AuditSuccessIsUnlockedTrue)
{
    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-theme";
    mParams.keyType = GsettingsKeyType::String;
    mParams.operation = GsettingsOperationType::IsUnlocked;
    mParams.value = "true";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeS)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsWritableCmd())).WillOnce(::testing::Return(Result<std::string>("true\n")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureGsettings, AuditSuccessIsUnlockedFalse)
{
    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-theme";
    mParams.keyType = GsettingsKeyType::String;
    mParams.operation = GsettingsOperationType::IsUnlocked;
    mParams.value = "false";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeS)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsWritableCmd())).WillOnce(::testing::Return(Result<std::string>("true\n")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureGsettings, AuditSuccessIsUnlockedValueChrzaszczyrzewoszyce)
{
    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-theme";
    mParams.keyType = GsettingsKeyType::String;
    mParams.operation = GsettingsOperationType::IsUnlocked;
    mParams.value = "chrzaszczyrzewoszyce";

    EXPECT_CALL(mContext, ExecuteCommand(GsettingsRangeCmd())).WillOnce(::testing::Return(Result<std::string>(gsettingsTypeS)));
    EXPECT_CALL(mContext, ExecuteCommand(GsettingsWritableCmd())).WillOnce(::testing::Return(Result<std::string>("false\n")));
    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureGsettings, AuditFailureIsUnlockedKeyTypeNumberU)
{
    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-theme";
    mParams.keyType = GsettingsKeyType::Number;
    mParams.operation = GsettingsOperationType::IsUnlocked;
    mParams.value = "42";

    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(!result.HasValue());
    ASSERT_EQ(result.Error().message, "Not supported keyType number for is-unlocked operation");
}

TEST_F(EnsureGsettings, AuditFailureIsUnlockedKeyTypeNumberI)
{
    mParams.schema = "org.gnome.desktop.interface";
    mParams.key = "cursor-theme";
    mParams.keyType = GsettingsKeyType::Number;
    mParams.operation = GsettingsOperationType::IsUnlocked;
    mParams.value = "42";

    auto result = AuditEnsureGsettings(mParams, mIndicators, mContext);
    ASSERT_TRUE(!result.HasValue());
    ASSERT_EQ(result.Error().message, "Not supported keyType number for is-unlocked operation");
}
