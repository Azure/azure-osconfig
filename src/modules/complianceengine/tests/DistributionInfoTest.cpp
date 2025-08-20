// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <DistributionInfo.h>
#include <MockContext.h>
#include <gtest/gtest.h>

using ComplianceEngine::DistributionInfo;
using ComplianceEngine::Error;
using ComplianceEngine::Result;

class DistributionInfoTest : public ::testing::Test
{
protected:
    MockContext mContext;
};

TEST_F(DistributionInfoTest, NonExistentFile)
{
    auto result = DistributionInfo::ParseEtcOsRelease("/tmp/somenoneexistentfilename");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, ENOENT);
    ASSERT_EQ(result.Error().message, "Failed to open /tmp/somenoneexistentfilename");
}

TEST_F(DistributionInfoTest, EmptyFile)
{
    auto filePath = mContext.MakeTempfile("");
    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, filePath + " does not contain 'ID' field");
}

TEST_F(DistributionInfoTest, ValidEtcOsReleaseFile)
{
    std::string content = "NAME=\"Ubuntu\"\nVERSION=\"20.04 LTS (Focal Fossa)\"\nID=ubuntu# comment 1\nVERSION_ID=\"20.04\"\n# comment2\n";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().osType, ComplianceEngine::OSType::Linux);
    EXPECT_EQ(result.Value().architecture, ComplianceEngine::Architecture::x86_64);
    EXPECT_EQ(result.Value().distribution, ComplianceEngine::LinuxDistribution::Ubuntu);
    EXPECT_EQ(result.Value().version, "20.04");
    EXPECT_EQ(std::to_string(result.Value()), R"(OS="Linux" ARCH="x86_64" DISTRO="ubuntu" VERSION="20.04")");
}

TEST_F(DistributionInfoTest, ValidEtcOsReleaseFile_WithComments)
{
    std::string content = "NAME=\"Ubuntu\"#comment 1\n\nID=ubuntu# comment 2\nVERSION_ID=\"20.04\"\n# comment3\n";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().osType, ComplianceEngine::OSType::Linux);
    EXPECT_EQ(result.Value().architecture, ComplianceEngine::Architecture::x86_64);
    EXPECT_EQ(result.Value().distribution, ComplianceEngine::LinuxDistribution::Ubuntu);
    EXPECT_EQ(result.Value().version, "20.04");
    EXPECT_EQ(std::to_string(result.Value()), R"(OS="Linux" ARCH="x86_64" DISTRO="ubuntu" VERSION="20.04")");
}

TEST_F(DistributionInfoTest, InvalidKey_1)
{
    auto filePath = mContext.MakeTempfile("=x\n");
    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unexpected '=' at the start of a key");
}

TEST_F(DistributionInfoTest, InvalidKey_2)
{
    auto filePath = mContext.MakeTempfile("a b=x\n");
    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unexpected space in a key");
}

TEST_F(DistributionInfoTest, InvalidKey_3)
{
    auto filePath = mContext.MakeTempfile("a b#=x\n");
    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unexpected space in a key");
}

TEST_F(DistributionInfoTest, InvalidKey_4)
{
    auto filePath = mContext.MakeTempfile("abc#=x\n");
    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unexpected comment character '#' in a key");
}

TEST_F(DistributionInfoTest, InvalidKey_5)
{
    auto filePath = mContext.MakeTempfile("abc");
    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unexpected end of input while parsing a key");
}

TEST_F(DistributionInfoTest, InvalidKey_6)
{
    auto filePath = mContext.MakeTempfile("abc ");
    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unexpected end of input while parsing a key");
}

TEST_F(DistributionInfoTest, InvalidValue_1)
{
    auto filePath = mContext.MakeTempfile(" a =\"\n");
    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unexpected end of input while parsing a quoted value");
}

TEST_F(DistributionInfoTest, InvalidValue_2)
{
    auto filePath = mContext.MakeTempfile(" a =X\"\n");
    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unexpected quote character past the start of value");
}

TEST_F(DistributionInfoTest, InvalidEtcOsReleaseFile_1)
{
    std::string content = "NAME=\"Ubuntu\"\nVERSION=\"20.04 LTS (Focal Fossa)\"\nID=\"x\"\nVERSION_ID=\"20.04\"\n";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unsupported Linux distribution: x");
}

TEST_F(DistributionInfoTest, InvalidEtcOsReleaseFile_2)
{
    std::string content = "NAME=\"Ubuntu\"\nVERSION=\"20.04 LTS #\"\nID=\"ubuntu\"";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_TRUE(result.Error().message.find("does not contain 'VERSION_ID' field") != std::string::npos);
}

TEST_F(DistributionInfoTest, ValidOverrideFile_1)
{
    std::string content = "OS=Linux\nARCH=x86_64\nDISTRO=ubuntu\nVERSION=20.04\n";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseOverrideFile(filePath);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().osType, ComplianceEngine::OSType::Linux);
    EXPECT_EQ(result.Value().architecture, ComplianceEngine::Architecture::x86_64);
    EXPECT_EQ(result.Value().distribution, ComplianceEngine::LinuxDistribution::Ubuntu);
    EXPECT_EQ(result.Value().version, "20.04");
    EXPECT_EQ(std::to_string(result.Value()), R"(OS="Linux" ARCH="x86_64" DISTRO="ubuntu" VERSION="20.04")");
}

TEST_F(DistributionInfoTest, ValidOverrideFile_2)
{
    std::string content = R"(OS="Linux" ARCH="x86_64" DISTRO="ubuntu" VERSION="20.04")";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseOverrideFile(filePath);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().osType, ComplianceEngine::OSType::Linux);
    EXPECT_EQ(result.Value().architecture, ComplianceEngine::Architecture::x86_64);
    EXPECT_EQ(result.Value().distribution, ComplianceEngine::LinuxDistribution::Ubuntu);
    EXPECT_EQ(result.Value().version, "20.04");
    EXPECT_EQ(std::to_string(result.Value()), content);
}

TEST_F(DistributionInfoTest, InalidOverrideFile_1)
{
    std::string content = R"(OS="Linux" ARCH="x86_64" DISTRO="ubuntu" )";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseOverrideFile(filePath);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EINVAL);
    EXPECT_EQ(result.Error().message, filePath + " file does not contain 'VERSION' field");
}

TEST_F(DistributionInfoTest, InalidOverrideFile_2)
{
    std::string content = R"(OS="Linux" ARCH="x86_64")";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseOverrideFile(filePath);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EINVAL);
    EXPECT_EQ(result.Error().message, filePath + " file does not contain 'DISTRO' field");
}

TEST_F(DistributionInfoTest, InalidOverrideFile_3)
{
    std::string content = R"(OS="Linux")";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseOverrideFile(filePath);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EINVAL);
    EXPECT_EQ(result.Error().message, filePath + " file does not contain 'ARCH' field");
}

TEST_F(DistributionInfoTest, InalidOverrideFile_4)
{
    std::string content = R"()";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseOverrideFile(filePath);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EINVAL);
    EXPECT_EQ(result.Error().message, filePath + " file does not contain 'OS' field");
}

TEST_F(DistributionInfoTest, InalidOverrideFile_5)
{
    std::string content = R"(OS="Linus")";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseOverrideFile(filePath);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EINVAL);
    EXPECT_EQ(result.Error().message, "Unsupported OS type: Linus");
}

TEST_F(DistributionInfoTest, InalidOverrideFile_6)
{
    std::string content = R"(OS="Linux" ARCH=RISC-V)";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseOverrideFile(filePath);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EINVAL);
    EXPECT_EQ(result.Error().message, "Unsupported architecture: RISC-V");
}

TEST_F(DistributionInfoTest, InalidOverrideFile_7)
{
    std::string content = R"(OS="Linux" ARCH=x86_64 DISTRO="kubuntu")";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseOverrideFile(filePath);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EINVAL);
    EXPECT_EQ(result.Error().message, "Unsupported Linux distribution: kubuntu");
}

TEST_F(DistributionInfoTest, InalidOverrideFile_8)
{
    std::string content = R"(OS="Linux" ARCH="x86_64" DISTRO="ubuntu" VERSIO N=x)";
    auto filePath = mContext.MakeTempfile(content);

    auto result = DistributionInfo::ParseOverrideFile(filePath);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().code, EINVAL);
    EXPECT_EQ(result.Error().message, "Unexpected space in a key");
}
