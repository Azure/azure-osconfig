#include "MockContext.h"

#include <EnsureRootPath.h>
#include <gtest/gtest.h>
#include <string>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

using ComplianceEngine::AuditEnsureRootPath;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ::testing::Return;

class EnsureRootPathTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree indicators;
    std::string path;

    void SetUp() override
    {
        char tmppath[MAXPATHLEN] = "/tmp/pathTestXXXXXX";
        ASSERT_TRUE(nullptr != mkdtemp(tmppath));
        path = tmppath;
        indicators.Push("EnsureRootPath");
    }

    void TearDown() override
    {
        rmdir(path.c_str());
    }
};

TEST_F(EnsureRootPathTest, AuditRootPathCompliant)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=/bin:/usr/bin:/sbin:/usr/sbin")));

    auto result = AuditEnsureRootPath(indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureRootPathTest, AuditRootPathNonCompliantEmptyDirectory)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=/bin::/usr/bin:/sbin:/usr/sbin")));

    auto result = AuditEnsureRootPath(indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureRootPathTest, AuditRootPathNonCompliantTrailingColon)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=/bin:/usr/bin:/sbin:/usr/sbin:")));

    auto result = AuditEnsureRootPath(indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureRootPathTest, AuditRootPathNonCompliantCurrentDirectory)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=/bin:.:/usr/bin:/sbin:/usr/sbin")));

    auto result = AuditEnsureRootPath(indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureRootPathTest, AuditRootPathNonCompliantDirectoryOwnership)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=" + path + "/bin:/usr/bin:/sbin:/usr/sbin")));

    ASSERT_EQ(chmod(path.c_str(), 0755), 0);
    // Either we can do it and we're root, or we can't and we're not - either way, it won't be root owned.
    chown(path.c_str(), 1000, 1000);

    auto result = AuditEnsureRootPath(indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureRootPathTest, AuditRootPathNonCompliantDirectoryPermissions)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=/bin:/usr/bin:/sbin:/usr/sbin:" + path)));

    ASSERT_EQ(chmod(path.c_str(), 0777), 0);

    auto result = AuditEnsureRootPath(indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
