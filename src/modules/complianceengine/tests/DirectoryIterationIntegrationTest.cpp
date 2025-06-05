// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonContext.h"
#include "DirectoryEntry.h"

#include <algorithm>
#include <fstream>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

using ComplianceEngine::CommonContext;
using ComplianceEngine::DirectoryEntry;
using ComplianceEngine::DirectoryEntryType;

class DirectoryIterationIntegrationTest : public ::testing::Test
{
protected:
    std::string mTestDir;
    CommonContext mContext;

    DirectoryIterationIntegrationTest()
        : mContext(nullptr)
    {
    }

    void SetUp() override
    {
        char tempTemplate[] = "/tmp/dir_test_XXXXXX";
        mTestDir = mkdtemp(tempTemplate);
        ASSERT_FALSE(mTestDir.empty());

        CreateTestFile(mTestDir + "/file1.txt", "content1");
        CreateTestFile(mTestDir + "/file2.txt", "content2");
        CreateTestDirectory(mTestDir + "/subdir");
        CreateTestFile(mTestDir + "/subdir/nested_file.txt", "nested content");
        CreateTestDirectory(mTestDir + "/subdir/nested_dir");
        CreateTestFile(mTestDir + "/subdir/nested_dir/deep_file.txt", "deep content");
    }

    void TearDown() override
    {
        if (!mTestDir.empty())
        {
            system(("rm -rf " + mTestDir).c_str());
        }
    }

private:
    void CreateTestFile(const std::string& path, const std::string& content)
    {
        std::ofstream file(path);
        file << content;
        file.close();
    }

    void CreateTestDirectory(const std::string& path)
    {
        mkdir(path.c_str(), 0755);
    }
};

TEST_F(DirectoryIterationIntegrationTest, NonRecursiveRealDirectory)
{
    auto result = mContext.GetDirectoryEntries(mTestDir, false);

    ASSERT_TRUE(result.HasValue());

    auto entries = std::move(result).Value();
    // Note: size() doesn't work for streaming iterators, so we count by iteration

    std::vector<std::string> filenames;
    for (const auto& entry : entries)
    {
        // Extract just the filename from the full path
        size_t lastSlash = entry.path.rfind('/');
        if (lastSlash != std::string::npos)
        {
            filenames.push_back(entry.path.substr(lastSlash + 1));
        }
    }

    // Should contain our test files and directory - at least 3 entries
    EXPECT_GE(filenames.size(), 3);
    bool hasFile1 = std::find(filenames.begin(), filenames.end(), std::string("file1.txt")) != filenames.end();
    bool hasFile2 = std::find(filenames.begin(), filenames.end(), std::string("file2.txt")) != filenames.end();
    bool hasSubdir = std::find(filenames.begin(), filenames.end(), std::string("subdir")) != filenames.end();

    EXPECT_TRUE(hasFile1);
    EXPECT_TRUE(hasFile2);
    EXPECT_TRUE(hasSubdir);

    // Should not contain nested files for non-recursive search
    bool hasNestedFile = std::find(filenames.begin(), filenames.end(), std::string("nested_file.txt")) != filenames.end();
    EXPECT_FALSE(hasNestedFile);
}

TEST_F(DirectoryIterationIntegrationTest, RecursiveRealDirectory)
{
    auto result = mContext.GetDirectoryEntries(mTestDir, true);

    ASSERT_TRUE(result.HasValue());

    auto entries = std::move(result).Value();
    // Note: size() doesn't work for streaming iterators, so we count manually

    bool foundNestedFile = false;
    bool foundDeepFile = false;
    int totalCount = 0;

    for (const auto& entry : entries)
    {
        totalCount++;
        if (entry.path.find("nested_file.txt") != std::string::npos)
        {
            foundNestedFile = true;
        }
        if (entry.path.find("deep_file.txt") != std::string::npos)
        {
            foundDeepFile = true;
        }
    }

    EXPECT_GE(totalCount, 5); // Should have at least 5 entries in recursive mode
    EXPECT_TRUE(foundNestedFile);
    EXPECT_TRUE(foundDeepFile);
}

TEST_F(DirectoryIterationIntegrationTest, NonExistentDirectory)
{
    auto result = mContext.GetDirectoryEntries("/this/path/does/not/exist", false);

    // Should return an empty result, not an error (fts behavior)
    ASSERT_TRUE(result.HasValue());
    auto entries = std::move(result).Value();

    // Count entries by iteration since empty() doesn't work reliably for streaming
    int count = 0;
    for (const auto& entry : entries)
    {
        (void)entry; // Suppress unused variable warning
        count++;
    }
    EXPECT_EQ(count, 0);
}

TEST_F(DirectoryIterationIntegrationTest, EmptyDirectory)
{
    std::string emptyDir = mTestDir + "/empty_subdir";
    mkdir(emptyDir.c_str(), 0755);

    auto result = mContext.GetDirectoryEntries(emptyDir, false);

    ASSERT_TRUE(result.HasValue());
    auto entries = std::move(result).Value();

    // Count entries by iteration since empty() doesn't work reliably for streaming
    int count = 0;
    for (const auto& entry : entries)
    {
        (void)entry; // Suppress unused variable warning
        count++;
    }
    EXPECT_EQ(count, 0);
}
