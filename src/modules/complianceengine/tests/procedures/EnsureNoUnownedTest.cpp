// Test for EnsureNoUnowned procedure
#include "EnsureNoUnowned.h"

#include "Evaluator.h"
#include "MockContext.h"

#include <fstream>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

using ComplianceEngine::AuditEnsureNoUnowned;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Status;

class EnsureNoUnownedTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree indicators;
    std::string rootDir;

    void SetUp() override
    {
        if (0 != getuid())
        {
            GTEST_SKIP() << "This test suite requires root privileges ";
        }

        indicators.Push("EnsureNoUnowned");
        rootDir = mContext.GetFilesystemScannerRoot();
    }
};

TEST_F(EnsureNoUnownedTest, CompliantWhenAllOwned)
{
    std::string filePath = rootDir + "/ownedfile";
    {
        std::ofstream ofs(filePath);
        ofs << "data";
    }
    ASSERT_EQ(::chmod(filePath.c_str(), 0644), 0);
    (void)mContext.GetFilesystemScanner().GetFullFilesystem();
    auto result = AuditEnsureNoUnowned(indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureNoUnownedTest, NonCompliantOnUnknownUid)
{
    // To simulate an unknown UID, we create a file then chown it to a high UID
    // that (very likely) doesn't exist in the test /etc/passwd. We cannot be
    // certain in all environments, but choose a large arbitrary UID.
    std::string filePath = rootDir + "/stray";
    {
        std::ofstream ofs(filePath);
        ofs << "x";
    }
    ASSERT_EQ(::chmod(filePath.c_str(), 0644), 0);
    uid_t fake = 61000; // typical not-present high UID
    // Attempt chown; if it fails due to EPERM (not root), skip test gracefully.
    if (::chown(filePath.c_str(), fake, (gid_t)-1) != 0)
    {
        GTEST_SKIP() << "Skipping NonCompliant test (requires chown permission)";
    }

    // Prime scanner after mutation
    (void)mContext.GetFilesystemScanner().GetFullFilesystem();
    auto result = AuditEnsureNoUnowned(indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
