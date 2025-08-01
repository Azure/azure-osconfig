// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <BenchmarkInfo.h>
#include <MockContext.h>
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
