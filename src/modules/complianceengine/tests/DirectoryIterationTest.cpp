// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DirectoryEntry.h"
#include "MockContext.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ComplianceEngine::DirectoryEntries;
using ComplianceEngine::DirectoryEntry;
using ComplianceEngine::DirectoryEntryType;
using ComplianceEngine::Result;
using ::testing::Return;

class DirectoryIterationTest : public ::testing::Test
{
protected:
    MockContext mContext;

    void SetUp() override
    {
    }
};

TEST_F(DirectoryIterationTest, NonRecursiveDirectoryIteration)
{
    std::vector<DirectoryEntry> mockEntries = {DirectoryEntry("/test/file1.txt", DirectoryEntryType::RegularFile),
        DirectoryEntry("/test/file2.txt", DirectoryEntryType::RegularFile), DirectoryEntry("/test/subdir", DirectoryEntryType::Directory)};

    EXPECT_CALL(mContext, GetDirectoryEntries("/test", false)).WillOnce([mockEntries](const std::string&, bool) {
        return Result<DirectoryEntries>(DirectoryEntries(mockEntries));
    });

    auto result = mContext.GetDirectoryEntries("/test", false);

    ASSERT_TRUE(result.HasValue());

    auto entries = std::move(result).Value();
    ASSERT_EQ(entries.size(), 3);

    std::vector<std::string> paths;
    for (const auto& entry : entries)
    {
        paths.push_back(entry.path);
    }

    EXPECT_EQ(paths.size(), 3);
    EXPECT_EQ(paths[0], "/test/file1.txt");
    EXPECT_EQ(paths[1], "/test/file2.txt");
    EXPECT_EQ(paths[2], "/test/subdir");
}

TEST_F(DirectoryIterationTest, RecursiveDirectoryIteration)
{
    std::vector<DirectoryEntry> mockEntries = {DirectoryEntry("/test/file1.txt", DirectoryEntryType::RegularFile),
        DirectoryEntry("/test/subdir", DirectoryEntryType::Directory), DirectoryEntry("/test/subdir/nested_file.txt", DirectoryEntryType::RegularFile),
        DirectoryEntry("/test/subdir/another_dir", DirectoryEntryType::Directory),
        DirectoryEntry("/test/subdir/another_dir/deep_file.txt", DirectoryEntryType::RegularFile)};

    EXPECT_CALL(mContext, GetDirectoryEntries("/test", true)).WillOnce([mockEntries](const std::string&, bool) {
        return Result<DirectoryEntries>(DirectoryEntries(mockEntries));
    });

    auto result = mContext.GetDirectoryEntries("/test", true);

    ASSERT_TRUE(result.HasValue());

    auto entries = std::move(result).Value();
    ASSERT_EQ(entries.size(), 5);

    auto it = entries.begin();
    EXPECT_EQ(it->path, "/test/file1.txt");
    EXPECT_EQ(it->type, DirectoryEntryType::RegularFile);

    ++it;
    EXPECT_EQ(it->path, "/test/subdir");
    EXPECT_EQ(it->type, DirectoryEntryType::Directory);

    int fileCount = 0;
    for (const auto& entry : entries)
    {
        if (entry.type == DirectoryEntryType::RegularFile)
        {
            fileCount++;
        }
    }
    EXPECT_EQ(fileCount, 3);
}

TEST_F(DirectoryIterationTest, DirectoryIterationError)
{
    EXPECT_CALL(mContext, GetDirectoryEntries("/nonexistent", false)).WillOnce([](const std::string&, bool) {
        return Result<DirectoryEntries>(ComplianceEngine::Error("Directory not found", -1));
    });

    auto result = mContext.GetDirectoryEntries("/nonexistent", false);

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error().message, "Directory not found");
    EXPECT_EQ(result.Error().code, -1);
}

TEST_F(DirectoryIterationTest, EmptyDirectoryIteration)
{
    std::vector<DirectoryEntry> emptyEntries;

    EXPECT_CALL(mContext, GetDirectoryEntries("/empty", false)).WillOnce([emptyEntries](const std::string&, bool) {
        return Result<DirectoryEntries>(DirectoryEntries(emptyEntries));
    });

    auto result = mContext.GetDirectoryEntries("/empty", false);

    ASSERT_TRUE(result.HasValue());

    auto entries = std::move(result).Value();
    EXPECT_TRUE(entries.empty());
    EXPECT_EQ(entries.size(), 0);

    int count = 0;
    for (const auto& entry : entries)
    {
        (void)entry; // Suppress unused variable warning
        count++;
    }
    EXPECT_EQ(count, 0);
}

TEST_F(DirectoryIterationTest, DifferentFileTypes)
{
    std::vector<DirectoryEntry> mockEntries = {DirectoryEntry("/test/regular.txt", DirectoryEntryType::RegularFile),
        DirectoryEntry("/test/subdir", DirectoryEntryType::Directory), DirectoryEntry("/test/symlink", DirectoryEntryType::SymbolicLink),
        DirectoryEntry("/test/other", DirectoryEntryType::Other)};

    EXPECT_CALL(mContext, GetDirectoryEntries("/test", false)).WillOnce([mockEntries](const std::string&, bool) {
        return Result<DirectoryEntries>(DirectoryEntries(mockEntries));
    });

    auto result = mContext.GetDirectoryEntries("/test", false);

    ASSERT_TRUE(result.HasValue());

    auto entries = std::move(result).Value();
    ASSERT_EQ(entries.size(), 4);

    std::map<DirectoryEntryType, int> typeCounts;
    for (const auto& entry : entries)
    {
        typeCounts[entry.type]++;
    }

    EXPECT_EQ(typeCounts[DirectoryEntryType::RegularFile], 1);
    EXPECT_EQ(typeCounts[DirectoryEntryType::Directory], 1);
    EXPECT_EQ(typeCounts[DirectoryEntryType::SymbolicLink], 1);
    EXPECT_EQ(typeCounts[DirectoryEntryType::Other], 1);
}
