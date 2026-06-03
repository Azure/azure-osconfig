// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"

#include <CopyFailMitigation.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

using ComplianceEngine::AuditCopyFailMitigation;
using ComplianceEngine::CopyFailMitigationParams;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::RemediateCopyFailMitigation;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

namespace
{
constexpr const char* cConfigPath = "/etc/osconfig/runtime-threat-mitigations/copyfail-afalg-lsm.allow";
constexpr const char* cStateDirectory = "/var/lib/osconfig/runtime-threat-mitigations/copyfail-afalg-lsm";
constexpr const char* cBpfFsDirectory = "/sys/fs/bpf";
constexpr const char* cLoaderScriptPath = "/usr/lib/osconfig/runtime-threat-mitigations/copyfail-afalg-lsm-load.sh";
constexpr const char* cSystemdUnitPath = "/etc/systemd/system/osconfig-copyfail-afalg-lsm.service";
constexpr const char* cAuditCommand = "bpftool prog show name copyfail_afalg >/dev/null 2>&1";

std::string ReadFile(const std::string& path)
{
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}
} // namespace

class CopyFailMitigationTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;

    void SetUp() override
    {
        mIndicators.Push("CopyFailMitigation");
    }
};

TEST_F(CopyFailMitigationTest, AuditNonCompliantWhenConfigMissing)
{
    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(cConfigPath))).WillRepeatedly(::testing::Return(Result<std::string>(Error("missing", ENOENT))));

    CopyFailMitigationParams params;
    auto result = AuditCopyFailMitigation(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(CopyFailMitigationTest, AuditNonCompliantWhenProgramMissing)
{
    CopyFailMitigationParams params;
    params.allowedExecutablePaths = std::string("/usr/bin/openssl|/opt/cloudflare/crypto-client");

    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(cConfigPath)))
        .WillRepeatedly(::testing::Return(Result<std::string>(std::string("/usr/bin/openssl\n/opt/cloudflare/crypto-client\n"))));
    EXPECT_CALL(mContext, ExecuteCommand(::testing::StrEq(cAuditCommand))).WillRepeatedly(::testing::Return(Result<std::string>(Error("program not found", ENOENT))));

    auto result = AuditCopyFailMitigation(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(CopyFailMitigationTest, AuditCompliantWhenConfigAndProgramPresent)
{
    CopyFailMitigationParams params;
    params.allowedExecutablePaths = std::string("/usr/bin/openssl");

    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(cConfigPath))).WillRepeatedly(::testing::Return(Result<std::string>(std::string("/usr/bin/openssl\n"))));
    EXPECT_CALL(mContext, ExecuteCommand(::testing::StrEq(cAuditCommand))).WillRepeatedly(::testing::Return(Result<std::string>(std::string())));

    auto result = AuditCopyFailMitigation(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(CopyFailMitigationTest, RejectsRelativeAllowListPath)
{
    CopyFailMitigationParams params;
    params.allowedExecutablePaths = std::string("relative/path");

    auto result = AuditCopyFailMitigation(params, mIndicators, mContext);

    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(CopyFailMitigationTest, RemediationWritesMitigationFilesAndRunsLoader)
{
    const auto root = mContext.GetTempdirPath() + std::string("/copyfail-root");
    mContext.SetSpecialFilePath(cConfigPath, root + cConfigPath);
    mContext.SetSpecialFilePath(cStateDirectory, root + cStateDirectory);
    mContext.SetSpecialFilePath(cBpfFsDirectory, root + cBpfFsDirectory);
    mContext.SetSpecialFilePath(cLoaderScriptPath, root + cLoaderScriptPath);
    mContext.SetSpecialFilePath(cSystemdUnitPath, root + cSystemdUnitPath);

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("copyfail-afalg-lsm-load.sh"))).WillOnce(::testing::Return(Result<std::string>(std::string("loaded"))));

    CopyFailMitigationParams params;
    params.allowedExecutablePaths = std::string("/usr/bin/openssl|/opt/cloudflare/crypto-client");
    auto result = RemediateCopyFailMitigation(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    ASSERT_EQ(result.Value(), Status::Compliant);

    const auto config = ReadFile(root + cConfigPath);
    ASSERT_EQ(config, "/usr/bin/openssl\n/opt/cloudflare/crypto-client\n");

    const auto object = ReadFile(root + std::string(cStateDirectory) + "/copyfail-afalg-lsm.bpf.o");
    ASSERT_FALSE(object.empty());

    const auto loader = ReadFile(root + cLoaderScriptPath);
    ASSERT_NE(loader.find("prog loadall"), std::string::npos);
    ASSERT_NE(loader.find("pinmaps"), std::string::npos);
    ASSERT_NE(loader.find("autoattach"), std::string::npos);
    ASSERT_NE(loader.find("allowed_paths"), std::string::npos);
    ASSERT_NE(loader.find("2f 75 73 72 2f 62 69 6e 2f 6f 70 65 6e 73 73 6c"), std::string::npos);

    const auto unit = ReadFile(root + cSystemdUnitPath);
    ASSERT_NE(unit.find("ExecStart="), std::string::npos);
    ASSERT_NE(unit.find("copyfail-afalg-lsm-load.sh"), std::string::npos);
}