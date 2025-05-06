#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using compliance::AuditEnsureRootPath;
using compliance::Error;
using compliance::IndicatorsTree;
using compliance::Result;
using compliance::Status;
using ::testing::Return;

class EnsureRootPathTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree indicators;
    const std::string pathTemplate = "/tmp/pathTestXXXXXX";
    std::string path;

    void SetUp() override
    {
        char* tmppath = strdup(pathTemplate.c_str());
        mkdtemp(tmppath);
        ASSERT_TRUE(tmppath != nullptr);
        path = tmppath;
        free(tmppath);
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

    auto result = AuditEnsureRootPath({}, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureRootPathTest, AuditRootPathNonCompliantEmptyDirectory)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=/bin::/usr/bin:/sbin:/usr/sbin")));

    auto result = AuditEnsureRootPath({}, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureRootPathTest, AuditRootPathNonCompliantTrailingColon)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=/bin:/usr/bin:/sbin:/usr/sbin:")));

    auto result = AuditEnsureRootPath({}, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureRootPathTest, AuditRootPathNonCompliantCurrentDirectory)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=/bin:.:/usr/bin:/sbin:/usr/sbin")));

    auto result = AuditEnsureRootPath({}, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureRootPathTest, AuditRootPathNonCompliantDirectoryOwnership)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=" + path + "/bin:/usr/bin:/sbin:/usr/sbin")));

    ASSERT_EQ(chmod(path.c_str(), 0755), 0);
    // Either we can do it and we're root, or we can't and we're not - either way, it won't be root owned.
    chown(path.c_str(), 1000, 1000);

    auto result = AuditEnsureRootPath({}, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureRootPathTest, AuditRootPathNonCompliantDirectoryPermissions)
{
    EXPECT_CALL(mContext, ExecuteCommand("sudo -Hiu root env")).WillOnce(Return(Result<std::string>("PATH=/bin:/usr/bin:/sbin:/usr/sbin:" + path)));

    ASSERT_EQ(chmod(path.c_str(), 0777), 0);

    auto result = AuditEnsureRootPath({}, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
