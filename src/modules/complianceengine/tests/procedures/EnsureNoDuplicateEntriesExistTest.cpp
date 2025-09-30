#include "Evaluator.h"
#include "MockContext.h"

#include <EnsureNoDuplicateEntriesExist.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

using ComplianceEngine::AuditEnsureNoDuplicateEntriesExist;
using ComplianceEngine::EnsureNoDuplicateEntriesExistParams;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::map;
using std::string;
using ::testing::Return;

class EnsureNoDuplicateEntriesExistTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    std::string mTempdir;

    void SetUp() override
    {
        char tmppath[MAXPATHLEN] = "/tmp/EnsureNoDuplicateEntriesExistTestXXXXXX";
        ASSERT_TRUE(nullptr != mkdtemp(tmppath));
        mTempdir = tmppath;
        mIndicators.Push("EnsureNoDuplicateEntriesExist");
    }

    void TearDown() override
    {
        if (0 != rmdir(mTempdir.c_str()))
        {
            OsConfigLogError(mContext.GetLogHandle(), "Failed to remove temporary directory %s: %s", mTempdir.c_str(), strerror(errno));
        }
    }

    std::string CreateTestFile(const std::string& content)
    {
        std::string filename = mTempdir + "/testfile.txt";
        std::ofstream file(filename);
        if (!file.is_open())
        {
            OsConfigLogError(mContext.GetLogHandle(), "Failed to create test file %s: %s", filename.c_str(), strerror(errno));
            return {};
        }
        file << content;
        file.close();
        return filename;
    }

    void RemoveTestFile(const std::string& filename)
    {
        if (remove(filename.c_str()) != 0)
        {
            OsConfigLogError(mContext.GetLogHandle(), "Failed to remove test file %s: %s", filename.c_str(), strerror(errno));
        }
    }
};

TEST_F(EnsureNoDuplicateEntriesExistTest, InvalidArguments_1)
{
    EnsureNoDuplicateEntriesExistParams params;
    params.filename = "testfile.txt";
    params.delimiter = ",,";
    auto result = AuditEnsureNoDuplicateEntriesExist(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Delimiter must be a single character");
}

TEST_F(EnsureNoDuplicateEntriesExistTest, InvalidArguments_2)
{
    EnsureNoDuplicateEntriesExistParams params;
    params.filename = "testfile.txt";
    params.delimiter = ",";
    params.column = -1; // Negative value
    auto result = AuditEnsureNoDuplicateEntriesExist(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, EINVAL);
    ASSERT_EQ(result.Error().message, "Column must be a non-negative integer");
}

TEST_F(EnsureNoDuplicateEntriesExistTest, MissingInputFile)
{
    EnsureNoDuplicateEntriesExistParams params;
    params.filename = "testfile.txt";
    params.delimiter = ",";
    params.column = 0;
    auto result = AuditEnsureNoDuplicateEntriesExist(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().code, ENOENT);
    ASSERT_EQ(result.Error().message, "Failed to open file: testfile.txt");
}

TEST_F(EnsureNoDuplicateEntriesExistTest, EmptyInputFile)
{
    auto filename = CreateTestFile("");
    EnsureNoDuplicateEntriesExistParams params;
    params.filename = filename;
    params.delimiter = ",";
    params.column = 0;
    auto result = AuditEnsureNoDuplicateEntriesExist(params, mIndicators, mContext);
    RemoveTestFile(filename);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureNoDuplicateEntriesExistTest, NoDuplicateEntries)
{
    auto filename = CreateTestFile("value1,value2,value3\nvalue4,value5,value6\n");
    EnsureNoDuplicateEntriesExistParams params;
    params.filename = filename;
    params.delimiter = ",";
    params.column = 0;
    auto result = AuditEnsureNoDuplicateEntriesExist(params, mIndicators, mContext);
    RemoveTestFile(filename);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureNoDuplicateEntriesExistTest, DuplicateEntries)
{
    auto filename = CreateTestFile("value1,value2,value3\nvalue1,value5,value6\n");
    EnsureNoDuplicateEntriesExistParams params;
    params.filename = filename;
    params.delimiter = ",";
    params.column = 0;
    auto result = AuditEnsureNoDuplicateEntriesExist(params, mIndicators, mContext);
    RemoveTestFile(filename);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureNoDuplicateEntriesExistTest, NoDuplicateEntries_SecondColumn)
{
    auto filename = CreateTestFile("value1,value2,value3\nvalue1,value5,value6\nvalue2,value8,value9\n");
    EnsureNoDuplicateEntriesExistParams params;
    params.filename = filename;
    params.delimiter = ",";
    params.column = 1;
    auto result = AuditEnsureNoDuplicateEntriesExist(params, mIndicators, mContext);
    RemoveTestFile(filename);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureNoDuplicateEntriesExistTest, DuplicateEntries_SecondColumn)
{
    auto filename = CreateTestFile("value1,value2,value3\nvalue1,value5,value6\nvalue2,value2,value9\n");
    EnsureNoDuplicateEntriesExistParams params;
    params.filename = filename;
    params.delimiter = ",";
    params.column = 1;
    auto result = AuditEnsureNoDuplicateEntriesExist(params, mIndicators, mContext);
    RemoveTestFile(filename);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureNoDuplicateEntriesExistTest, NoDuplicateEntries_MessageWithoutContext)
{
    auto filename = CreateTestFile("value1,value2,value3\nvalue4,value5,value6\n");
    EnsureNoDuplicateEntriesExistParams params;
    params.filename = filename;
    params.delimiter = ",";
    params.column = 0;
    AuditEnsureNoDuplicateEntriesExist(params, mIndicators, mContext);
    RemoveTestFile(filename);
    ASSERT_TRUE(mIndicators.GetRootNode() != nullptr);
    ASSERT_FALSE(mIndicators.GetRootNode()->indicators.empty());
    auto formattedMessage = mIndicators.Back().indicators.back().message;
    ASSERT_TRUE(formattedMessage.find("No duplicate entries found in") != std::string::npos);
}

TEST_F(EnsureNoDuplicateEntriesExistTest, NoDuplicateEntries_MessageWithContext)
{
    auto filename = CreateTestFile("value1,value2,value3\nvalue4,value5,value6\n");
    EnsureNoDuplicateEntriesExistParams params;
    params.filename = filename;
    params.delimiter = ",";
    params.column = 0;
    params.context = "test entries";
    AuditEnsureNoDuplicateEntriesExist(params, mIndicators, mContext);
    RemoveTestFile(filename);
    ASSERT_TRUE(mIndicators.GetRootNode() != nullptr);
    ASSERT_FALSE(mIndicators.GetRootNode()->indicators.empty());
    auto formattedMessage = mIndicators.Back().indicators.back().message;
    ASSERT_TRUE(formattedMessage.find("No duplicate test entries found in") != std::string::npos);
}
