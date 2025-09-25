#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <fstream>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

using ComplianceEngine::AuditEnsureNoWritables;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EnsureNoWritablesTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree indicators;
    std::string rootDir;

    void SetUp() override
    {
        indicators.Push("EnsureNoWritables");
        rootDir = mContext.GetTempdirPath();
        rootDir += "/rootfs"; // scanner root
    }
};

TEST_F(EnsureNoWritablesTest, WorldWritableFileNonCompliant)
{
    std::string badFile = rootDir + "/badfile";
    {
        std::ofstream ofs(badFile);
        ofs << "data";
    }
    ASSERT_EQ(::chmod(badFile.c_str(), 0666), 0);
    // Prime scanner after artifact creation
    (void)mContext.GetFilesystemScanner().GetFullFilesystem();

    auto result = AuditEnsureNoWritables({}, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureNoWritablesTest, WorldWritableDirWithoutStickyNonCompliant)
{
    std::string badDir = rootDir + "/badnosticky";
    ASSERT_EQ(::mkdir(badDir.c_str(), 0777), 0);
    // Some umasks strip bits; enforce world-writable without sticky explicitly
    ASSERT_EQ(::chmod(badDir.c_str(), 0777), 0);

    struct stat st;
    ASSERT_EQ(::lstat(badDir.c_str(), &st), 0);
    ASSERT_TRUE(S_ISDIR(st.st_mode));
    ASSERT_TRUE((st.st_mode & S_IWOTH) != 0);
    ASSERT_FALSE((st.st_mode & S_ISVTX));

    // Prime scanner after artifact creation
    auto full = mContext.GetFilesystemScanner().GetFullFilesystem();
    ASSERT_TRUE(full);
    bool found = full.Value()->entries.find(badDir) != full.Value()->entries.end();
    ASSERT_TRUE(found) << "Directory not found in scanner entries";

    auto result = AuditEnsureNoWritables({}, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureNoWritablesTest, CompliantWhenNoViolations)
{
    std::string stickyDir = rootDir + "/goodsticky";
    ASSERT_EQ(::mkdir(stickyDir.c_str(), 01777), 0);
    std::string okFile = rootDir + "/okfile";
    {
        std::ofstream ofs(okFile);
        ofs << "ok";
    }
    ASSERT_EQ(::chmod(okFile.c_str(), 0644), 0);
    // Prime scanner after creating objects
    (void)mContext.GetFilesystemScanner().GetFullFilesystem();

    auto result = AuditEnsureNoWritables({}, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
