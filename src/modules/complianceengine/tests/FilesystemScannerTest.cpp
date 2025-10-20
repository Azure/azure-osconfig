// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "FilesystemScanner.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using ComplianceEngine::Error;
using ComplianceEngine::FilesystemScanner;
using ComplianceEngine::Result;

namespace
{
std::string MakeTempDir()
{
    char templ[] = "/tmp/fs_scanner_testXXXXXX";
    char* p = ::mkdtemp(templ);
    if (!p)
    {
        throw std::runtime_error("mkdtemp failed");
    }
    return std::string(p);
}

void TouchFile(const std::string& path)
{
    std::ofstream ofs(path.c_str());
    ofs << "data";
    ofs.close();
}
} // namespace

class FilesystemScannerTest : public ::testing::Test
{
protected:
    std::string rootDir;
    std::string cachePath;
    std::string lockPath;

    void SetUp() override
    {
        rootDir = MakeTempDir();
        // create some files
        ::mkdir((rootDir + "/sub").c_str(), 0755);
        TouchFile(rootDir + "/a.txt");
        TouchFile(rootDir + "/sub/b.txt");
        cachePath = rootDir + "/cache.txt"; // place cache within temp dir
        lockPath = rootDir + "/lock.lck";
    }

    void TearDown() override
    {
        // Best-effort cleanup
        ::unlink(cachePath.c_str());
        ::unlink(lockPath.c_str());
        ::unlink((rootDir + "/a.txt").c_str());
        ::unlink((rootDir + "/sub/b.txt").c_str());
        ::rmdir((rootDir + "/sub").c_str());
        ::rmdir(rootDir.c_str());
    }
};

TEST_F(FilesystemScannerTest, InitialCacheBuildWaitsAndSucceeds)
{
    // soft=5, hard=10, wait=3 seconds (ample time for small scan)
    FilesystemScanner scanner(rootDir, cachePath, lockPath, 5, 10, 3);
    auto res = scanner.GetFullFilesystem();
    // First call may return either value (if background finished within wait) or error
    if (!res)
    {
        // If error due to background not done yet, call again after short sleep
        ::usleep(400 * 1000); // 400ms
        res = scanner.GetFullFilesystem();
    }
    ASSERT_TRUE(res) << "Cache should be available after wait window: " << (res ? "" : res.Error().message);
    ASSERT_GT(res.Value()->entries.size(), 0u);
}

TEST_F(FilesystemScannerTest, SoftTimeoutTriggersBackgroundButReturnsData)
{
    FilesystemScanner scanner(rootDir, cachePath, lockPath, 1, 100, 0); // short soft timeout
    // Build cache
    auto res = scanner.GetFullFilesystem();
    if (!res)
    {
        ::usleep(300 * 1000);
        res = scanner.GetFullFilesystem();
    }
    ASSERT_TRUE(res);
    auto firstEnd = res.Value()->scan_end_time;
    ASSERT_GT(firstEnd, 0);
    // Wait past soft timeout but before hard
    ::sleep(2);
    auto res2 = scanner.GetFullFilesystem();
    ASSERT_TRUE(res2);                                // soft timeout returns stale data
    EXPECT_EQ(res2.Value()->scan_end_time, firstEnd); // cache unchanged yet
}

TEST_F(FilesystemScannerTest, HardTimeoutCausesErrorUntilRefreshFinishes)
{
    FilesystemScanner scanner(rootDir, cachePath, lockPath, 1, 2, 0); // soft=1, hard=2
    auto res = scanner.GetFullFilesystem();
    if (!res)
    {
        ::usleep(500 * 1000);
        res = scanner.GetFullFilesystem();
    }
    ASSERT_TRUE(res);
    ::sleep(3); // exceed hard timeout
    auto res2 = scanner.GetFullFilesystem();
    ASSERT_FALSE(res2); // should be error due to hard timeout and no wait
    // Give background scan time to complete
    ::usleep(400 * 1000);
    auto res3 = scanner.GetFullFilesystem();
    ASSERT_TRUE(res3); // new cache available
    EXPECT_NE(res3.Value()->scan_end_time, res.Value()->scan_end_time);
}

TEST_F(FilesystemScannerTest, HardTimeoutWithWaitMayReturnFreshCache)
{
    FilesystemScanner scanner(rootDir, cachePath, lockPath, 1, 2, 2); // wait up to 2s
    auto res = scanner.GetFullFilesystem();
    if (!res)
    {
        ::usleep(400 * 1000);
        res = scanner.GetFullFilesystem();
    }
    ASSERT_TRUE(res);
    auto oldEnd = res.Value()->scan_end_time;
    ::sleep(3); // exceed hard timeout
    auto res2 = scanner.GetFullFilesystem();
    if (!res2)
    {
        // wait path failed to obtain new cache in time
        ::usleep(800 * 1000);
        res2 = scanner.GetFullFilesystem();
    }
    ASSERT_TRUE(res2);
    EXPECT_NE(oldEnd, res2.Value()->scan_end_time);
}

TEST_F(FilesystemScannerTest, LoadCacheSkipsOverHardTimeout)
{
    FilesystemScanner scanner(rootDir, cachePath, lockPath, 1, 2, 0);
    // Build initial cache
    auto res = scanner.GetFullFilesystem();
    if (!res)
    {
        ::usleep(300 * 1000);
        res = scanner.GetFullFilesystem();
    }
    ASSERT_TRUE(res);
    // Manually modify header to simulate old cache beyond hard timeout
    {
        std::ofstream ofs(cachePath.c_str(), std::ios::out | std::ios::trunc);
        long oldStart = (long)::time(nullptr) - 100;
        long oldEnd = oldStart - 1; // ensure earlier
        ofs << "# FilesystemScanCache-V1 " << oldStart << ' ' << oldEnd << "\n";
        ofs.close();
    }
    // Second scanner to test LoadCache rejection
    FilesystemScanner scanner2(rootDir, cachePath, lockPath, 1, 2, 0);
    auto res2 = scanner2.GetFullFilesystem();
    ASSERT_FALSE(res2); // should treat stale cache as unusable and error (background scan kicked)
}

TEST_F(FilesystemScannerTest, LegacyCacheFormatStillLoads)
{
    // Manually create a legacy-format cache file (same format as current) with a couple entries
    // This ensures that changes to internal storage (vector -> map) did not alter on-disk parsing assumptions.
    FilesystemScanner scanner(rootDir, cachePath, lockPath, 5, 10, 1);
    // Craft cache file with start/end times that are fresh
    time_t now = ::time(nullptr);
    {
        std::ofstream ofs(cachePath.c_str(), std::ios::out | std::ios::trunc);
        ofs << "# FilesystemScanCache-V1 " << static_cast<long>(now - 1) << ' ' << static_cast<long>(now - 1) << "\n";
        // Two fake entries rooted at test root; use current stat info from rootDir
        struct stat stRoot;
        ASSERT_EQ(::lstat(rootDir.c_str(), &stRoot), 0);
        ofs << rootDir << ' ' << static_cast<unsigned long long>(stRoot.st_dev) << ' ' << static_cast<unsigned long long>(stRoot.st_ino) << ' '
            << static_cast<unsigned>(stRoot.st_mode) << ' ' << static_cast<unsigned>(stRoot.st_nlink) << ' ' << static_cast<long long>(stRoot.st_uid)
            << ' ' << static_cast<long>(stRoot.st_gid) << ' ' << static_cast<long long>(stRoot.st_size) << ' ' << static_cast<long>(stRoot.st_blksize)
            << ' ' << static_cast<long long>(stRoot.st_blocks) << "\n";
        // Add a dummy child path entry referencing same stats (acceptable for test purpose)
        ofs << rootDir << "/dummy" << ' ' << static_cast<unsigned long long>(stRoot.st_dev) << ' ' << static_cast<unsigned long long>(stRoot.st_ino)
            << ' ' << static_cast<unsigned>(stRoot.st_mode) << ' ' << static_cast<unsigned>(stRoot.st_nlink) << ' '
            << static_cast<long long>(stRoot.st_uid) << ' ' << static_cast<long>(stRoot.st_gid) << ' ' << static_cast<long long>(stRoot.st_size) << ' '
            << static_cast<long>(stRoot.st_blksize) << ' ' << static_cast<long long>(stRoot.st_blocks) << "\n";
    }
    auto res = scanner.GetFullFilesystem();
    if (!res)
    {
        // Allow background wait window
        ::usleep(300 * 1000);
        res = scanner.GetFullFilesystem();
    }
    ASSERT_TRUE(res);
    // Expect at least the two synthetic entries to be recognized.
    size_t found = 0;
    for (const auto& kv : res.Value()->entries)
    {
        if (kv.first == rootDir || kv.first == rootDir + "/dummy")
        {
            ++found;
        }
    }
    EXPECT_EQ(found, (size_t)2);
}
