// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <BenchmarkInfo.h>
#include <MockContext.h>
#include <fnmatch.h>
#include <gtest/gtest.h>

using ComplianceEngine::CISBenchmarkInfo;
using ComplianceEngine::DistributionInfo;
using ComplianceEngine::Error;
using ComplianceEngine::LinuxDistribution;
using ComplianceEngine::Result;

class BenchmarkInfoTest : public ::testing::Test
{
protected:
    MockContext mContext;
};

TEST_F(BenchmarkInfoTest, Invalid_1)
{
    auto result = CISBenchmarkInfo::Parse("");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Invalid payload key format: must start with '/'");
}

TEST_F(BenchmarkInfoTest, Invalid_2)
{
    auto result = CISBenchmarkInfo::Parse("/");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Invalid payload key format: missing benchmark type");
}

TEST_F(BenchmarkInfoTest, Invalid_3)
{
    auto result = CISBenchmarkInfo::Parse("/x");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unsupported benchmark type: 'x'");
}

TEST_F(BenchmarkInfoTest, Invalid_4)
{
    auto result = CISBenchmarkInfo::Parse("/cis");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Invalid CIS benchmark payload key format: missing distribution");
}

TEST_F(BenchmarkInfoTest, Invalid_5)
{
    auto result = CISBenchmarkInfo::Parse("/cis/x");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Unsupported Linux distribution: x");
}

TEST_F(BenchmarkInfoTest, Invalid_6)
{
    auto result = CISBenchmarkInfo::Parse("/cis/ubuntu");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Invalid CIS benchmark payload key format: missing distribution version");
}

TEST_F(BenchmarkInfoTest, Invalid_7)
{
    auto result = CISBenchmarkInfo::Parse("/cis/ubuntu//");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Invalid CIS benchmark payload key format: missing distribution version");
}

TEST_F(BenchmarkInfoTest, Invalid_8)
{
    auto result = CISBenchmarkInfo::Parse("/cis/ubuntu/someversion/");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Invalid CIS benchmark payload key format: missing benchmark version");
}

TEST_F(BenchmarkInfoTest, Invalid_9)
{
    auto result = CISBenchmarkInfo::Parse("/cis/ubuntu/someversion//");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Invalid CIS benchmark payload key format: missing benchmark version");
}

TEST_F(BenchmarkInfoTest, Invalid_10)
{
    auto result = CISBenchmarkInfo::Parse("/cis/ubuntu/20.04/v1.0.0");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Invalid CIS benchmark payload key format: missing benchmark section");
}

TEST_F(BenchmarkInfoTest, Invalid_11)
{
    auto result = CISBenchmarkInfo::Parse("/cis/ubuntu/20.04/v1.0.0/");
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Invalid CIS benchmark payload key format: missing benchmark section");
}

TEST_F(BenchmarkInfoTest, Valid_1)
{
    auto result = CISBenchmarkInfo::Parse("/cis/ubuntu/20.04/v1.0.0/x/y/z");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().distribution, LinuxDistribution::Ubuntu);
    EXPECT_EQ(result.Value().version, std::string("20.04"));
    EXPECT_EQ(result.Value().benchmarkVersion, std::string("v1.0.0"));
    EXPECT_EQ(result.Value().section, "x/y/z");
    EXPECT_EQ(std::to_string(result.Value()), "/cis/ubuntu/20.04/v1.0.0/x/y/z");
}

TEST_F(BenchmarkInfoTest, Valid_Stig)
{
    auto result = CISBenchmarkInfo::Parse("/stig/rhel/9/V2R7/SV-257777");
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value().distribution, LinuxDistribution::RHEL);
    EXPECT_EQ(result.Value().version, std::string("9"));
    EXPECT_EQ(result.Value().benchmarkVersion, std::string("V2R7"));
    EXPECT_EQ(result.Value().section, "SV-257777");
}

TEST_F(BenchmarkInfoTest, Match_1)
{
    auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/ubuntu/20.04/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    auto filePath = mContext.MakeTempfile("ID=ubuntu\nVERSION_ID=20.04");
    auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());
    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, Match_2)
{
    auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/ubuntu/20.04/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    auto filePath = mContext.MakeTempfile("ID=ubuntu\nVERSION_ID=16.04");
    auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());
    EXPECT_FALSE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, Match_3)
{
    auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/ubuntu/22.04/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    auto filePath = mContext.MakeTempfile("ID=ubuntu\nVERSION_ID=20.04");
    auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());
    EXPECT_FALSE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, Match_4)
{
    auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/ubuntu/22.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    auto filePath = mContext.MakeTempfile("ID=ubuntu\nVERSION_ID=22.1124");
    auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());
    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, Match_5)
{
    auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/ubuntu/22.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    auto filePath = mContext.MakeTempfile("ID=ubuntu\nVERSION_ID=24.04");
    auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());
    EXPECT_FALSE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, InvalidGlobbing_1)
{
    auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/ubuntu/[/v1.0.0/x/y/z");
    ASSERT_FALSE(benchmarkInfo.HasValue());
    EXPECT_EQ("Invalid benchmark version: [. Globbing characters [ ] { } are not allowed.", benchmarkInfo.Error().message);
}

TEST_F(BenchmarkInfoTest, InvalidGlobbing_2)
{
    auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/ubuntu/foo]/v1.0.0/x/y/z");
    ASSERT_FALSE(benchmarkInfo.HasValue());
    EXPECT_EQ("Invalid benchmark version: foo]. Globbing characters [ ] { } are not allowed.", benchmarkInfo.Error().message);
}

TEST_F(BenchmarkInfoTest, InvalidGlobbing_3)
{
    auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/ubuntu/bar{}/v1.0.0/x/y/z");
    ASSERT_FALSE(benchmarkInfo.HasValue());
    EXPECT_EQ("Invalid benchmark version: bar{}. Globbing characters [ ] { } are not allowed.", benchmarkInfo.Error().message);
}

TEST_F(BenchmarkInfoTest, SanitizedGlobbing_1)
{
    auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/ubuntu/foo?bar*baz/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ("fooxbarbaz", benchmarkInfo->SanitizedVersion());
    EXPECT_EQ(fnmatch("foo?bar*baz", "fooxbarbaz", 0), 0);
}

TEST_F(BenchmarkInfoTest, SanitizedGlobbing_EscapedDot)
{
    // Real benchmarks encode versions like "3\.*". The sanitized version must
    // drop the fnmatch escape backslash so the produced literal still satisfies
    // the benchmark's fnmatch() check (used for the override-file suggestion).
    auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/azurelinux/3\\.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ("3\\.*", benchmarkInfo->version);
    EXPECT_EQ("3.", benchmarkInfo->SanitizedVersion());
    EXPECT_EQ(fnmatch(benchmarkInfo->version.c_str(), benchmarkInfo->SanitizedVersion().c_str(), 0), 0);
}

TEST_F(BenchmarkInfoTest, DistroMatrix_AlmaLinux)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/almalinux/9\\.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::AlmaLinux);
    EXPECT_EQ(benchmarkInfo->version, "9\\.*");
    const auto filePath = mContext.MakeTempfile("ID=almalinux\nVERSION_ID=9.6");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, DistroMatrix_AmazonLinux)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/amzn/2/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::AmazonLinux);
    EXPECT_EQ(benchmarkInfo->version, "2");

    const auto filePath = mContext.MakeTempfile("ID=amzn\nVERSION_ID=2");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, DistroMatrix_AzureLinux)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/azurelinux/3\\.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::AzureLinux);
    EXPECT_EQ(benchmarkInfo->version, "3\\.*");

    const auto filePath = mContext.MakeTempfile("ID=azurelinux\nVERSION_ID=3.0");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, DistroMatrix_CentOS)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/centos/8/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::Centos);
    EXPECT_EQ(benchmarkInfo->version, "8");

    const auto filePath = mContext.MakeTempfile("ID=centos\nVERSION_ID=8");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, DistroMatrix_Debian)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/debian/12/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::Debian);
    EXPECT_EQ(benchmarkInfo->version, "12");

    const auto filePath = mContext.MakeTempfile("ID=debian\nVERSION_ID=12");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, DistroMatrix_OracleLinux)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/ol/7\\.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::OracleLinux);
    EXPECT_EQ(benchmarkInfo->version, "7\\.*");

    const auto filePath = mContext.MakeTempfile("ID=ol\nVERSION_ID=7.9");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, DistroMatrix_RedHat)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/rhel/9\\.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::RHEL);
    EXPECT_EQ(benchmarkInfo->version, "9\\.*");

    const auto filePath = mContext.MakeTempfile("ID=rhel\nVERSION_ID=9.6");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, DistroMatrix_RockyLinux)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/rocky/9\\.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::RockyLinux);
    EXPECT_EQ(benchmarkInfo->version, "9\\.*");

    const auto filePath = mContext.MakeTempfile("ID=rocky\nVERSION_ID=9.3");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

TEST_F(BenchmarkInfoTest, DistroMatrix_Suse)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/sles/15\\.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::SUSE);
    EXPECT_EQ(benchmarkInfo->version, "15\\.*");

    const auto filePath = mContext.MakeTempfile("ID=sles\nVERSION_ID=15.5");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

// Azure Linux 4 reports ID=azurelinux with a simple major.minor VERSION_ID.
// The augmentation engine keys the benchmark to the `4.*` version glob.
TEST_F(BenchmarkInfoTest, DistroMatrix_AzureLinux4)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/azurelinux/4.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::AzureLinux);
    EXPECT_EQ(benchmarkInfo->version, "4.*");

    const auto filePath = mContext.MakeTempfile("ID=azurelinux\nVERSION_ID=4.0");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

// Azure Container Linux 4 also reports ID=azurelinux, but its VERSION_ID carries
// a date-based patch component (e.g. 4.0.20260709). The `4.*` glob must still
// match so the Azure Container Linux 4 benchmark is considered applicable.
TEST_F(BenchmarkInfoTest, DistroMatrix_AzureContainerLinux4)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/azurelinux/4.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());
    EXPECT_EQ(benchmarkInfo->distribution, LinuxDistribution::AzureLinux);

    const auto filePath = mContext.MakeTempfile("ID=azurelinux\nVERSION_ID=4.0.20260709");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_TRUE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

// A `4.*` benchmark must not match an Azure Linux 3 system: the major version
// glob differs even though the distribution is the same.
TEST_F(BenchmarkInfoTest, Match_AzureLinux4_DoesNotMatchAzureLinux3)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/azurelinux/4.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());

    const auto filePath = mContext.MakeTempfile("ID=azurelinux\nVERSION_ID=3.0");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_FALSE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}

// When the distribution IDs differ, Match must return false regardless of the
// version glob. This is the branch the assessor relies on to skip a benchmark
// that targets a different OS than the one it is running on.
TEST_F(BenchmarkInfoTest, Match_DistributionMismatch)
{
    const auto benchmarkInfo = CISBenchmarkInfo::Parse("/cis/azurelinux/4.*/v1.0.0/x/y/z");
    ASSERT_TRUE(benchmarkInfo.HasValue());

    const auto filePath = mContext.MakeTempfile("ID=ubuntu\nVERSION_ID=24.04");
    const auto distributionInfo = DistributionInfo::ParseEtcOsRelease(filePath);
    ASSERT_TRUE(distributionInfo.HasValue());

    EXPECT_FALSE(benchmarkInfo.Value().Match(distributionInfo.Value()));
}
