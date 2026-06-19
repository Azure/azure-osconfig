// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Tests for InputSecurity helpers.
//
// Files and directories are created inside a per-test mkdtemp() tree managed
// by MockContext. The fixture removes the entire tree on teardown, so a crash
// or early exit never leaves stale paths that clash with a subsequent run —
// unlike hardcoded /tmp filenames that survive process death.
//
// Tests that require root (e.g. chown to root) are skipped when running as
// non-root.

#include <InputSecurity.hpp>
#include <MockContext.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using ComplianceEngine::Assessor::OpenVerifiedInput;
using ComplianceEngine::Assessor::RefusePathTraversal;
using ComplianceEngine::Assessor::RefuseUnsafeLogFile;
using ComplianceEngine::Assessor::RefuseWritableParentDir;

namespace
{

// Minimal helper: creates a regular file at `path` with the given mode.
// Returns true on success.
bool CreateFile(const std::string& path, mode_t mode)
{
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, mode);
    if (fd < 0)
    {
        return false;
    }
    ::close(fd);
    return true;
}

} // namespace

// Base fixture: owns a MockContext whose mkdtemp() tree is removed on
// TearDown. Helper methods delegate to it so test bodies stay concise.
// Per-suite subclasses preserve GTest suite names (e.g. OpenVerifiedInputTest.*).
class InputSecurityFixture : public ::testing::Test
{
protected:
    // Returns the absolute path for `name` inside this test's temp directory.
    std::string TempPath(const std::string& name) const
    {
        return mCtx.GetTempdirPath() + "/" + name;
    }

    // Creates a subdirectory with the given mode and returns its full path.
    std::string MakeSubdir(const std::string& name, mode_t mode = 0755) const
    {
        std::string path = TempPath(name);
        EXPECT_EQ(0, ::mkdir(path.c_str(), mode)) << std::strerror(errno);
        return path;
    }

    MockContext mCtx;
};

class RefuseWritableParentDirTest : public InputSecurityFixture
{
};
class OpenVerifiedInputTest : public InputSecurityFixture
{
};
class RefuseUnsafeLogFileTest : public InputSecurityFixture
{
};

// ---------------------------------------------------------------------------
// RefusePathTraversal — pure string logic; no filesystem access needed.
// ---------------------------------------------------------------------------

TEST(RefusePathTraversalTest, PlainPathIsAccepted)
{
    EXPECT_FALSE(RefusePathTraversal("/etc/foo/bar.mof", nullptr));
}

TEST(RefusePathTraversalTest, BareDotDotIsRefused)
{
    EXPECT_TRUE(RefusePathTraversal("..", nullptr));
}

TEST(RefusePathTraversalTest, DotDotPrefixIsRefused)
{
    EXPECT_TRUE(RefusePathTraversal("../etc/passwd", nullptr));
}

TEST(RefusePathTraversalTest, DotDotEmbeddedIsRefused)
{
    EXPECT_TRUE(RefusePathTraversal("/var/lib/../../etc/passwd", nullptr));
}

TEST(RefusePathTraversalTest, DotDotSuffixIsRefused)
{
    EXPECT_TRUE(RefusePathTraversal("/var/lib/..", nullptr));
}

TEST(RefusePathTraversalTest, SingleDotIsAccepted)
{
    // "." is not a traversal component
    EXPECT_FALSE(RefusePathTraversal("./foo.mof", nullptr));
}

TEST(RefusePathTraversalTest, EmptyPathIsAccepted)
{
    EXPECT_FALSE(RefusePathTraversal("", nullptr));
}

TEST(RefusePathTraversalTest, RelativeEmbeddedDotDotIsRefused)
{
    // A traversal component is rejected even without a leading slash.
    EXPECT_TRUE(RefusePathTraversal("foo/../bar.mof", nullptr));
}

TEST(RefusePathTraversalTest, FilenamesContainingDotsAreAccepted)
{
    // ".." must only be treated as traversal when it is a whole path
    // component (bounded by slashes or string ends). Filenames that merely
    // contain dot characters are legitimate and must not be over-rejected;
    // a naive substring search for ".." would wrongly refuse all of these.
    EXPECT_FALSE(RefusePathTraversal("/etc/foo..bar.mof", nullptr)); // embedded dots
    EXPECT_FALSE(RefusePathTraversal("/etc/...mof", nullptr));       // leading triple-dot name
    EXPECT_FALSE(RefusePathTraversal("/etc/..foo", nullptr));        // name starting with ".."
    EXPECT_FALSE(RefusePathTraversal("/etc/foo..", nullptr));        // name ending with ".."
    EXPECT_FALSE(RefusePathTraversal("...", nullptr));               // bare triple-dot
}

// ---------------------------------------------------------------------------
// RefuseWritableParentDir
// ---------------------------------------------------------------------------

TEST_F(RefuseWritableParentDirTest, ParentRootOwnedNotWritableIsAccepted)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string dir = MakeSubdir("safe_parent", 0755);
    ASSERT_EQ(0, ::chown(dir.c_str(), 0, 0)) << std::strerror(errno);
    const std::string file = dir + "/test.mof";
    ASSERT_TRUE(CreateFile(file, 0600)) << std::strerror(errno);
    ASSERT_EQ(0, ::chown(file.c_str(), 0, 0)) << std::strerror(errno);

    EXPECT_FALSE(RefuseWritableParentDir(file, nullptr));
}

TEST_F(RefuseWritableParentDirTest, WorldWritableParentIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string dir = MakeSubdir("writable_parent", 0755);
    ASSERT_EQ(0, ::chown(dir.c_str(), 0, 0)) << std::strerror(errno);
    ASSERT_EQ(0, ::chmod(dir.c_str(), 0777)) << std::strerror(errno); // world-writable; set after creation to bypass umask
    const std::string file = dir + "/test.mof";
    ASSERT_TRUE(CreateFile(file, 0600)) << std::strerror(errno);
    ASSERT_EQ(0, ::chown(file.c_str(), 0, 0)) << std::strerror(errno);

    EXPECT_TRUE(RefuseWritableParentDir(file, nullptr));
}

TEST_F(RefuseWritableParentDirTest, NonRootOwnedParentIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "cannot chown to arbitrary uid as non-root";
    }
    const std::string dir = MakeSubdir("nonroot_parent", 0755);
    ASSERT_EQ(0, ::chown(dir.c_str(), 1000, 1000)) << std::strerror(errno); // non-root owner
    const std::string file = dir + "/test.mof";
    ASSERT_TRUE(CreateFile(file, 0600)) << std::strerror(errno);

    EXPECT_TRUE(RefuseWritableParentDir(file, nullptr));
}

TEST_F(RefuseWritableParentDirTest, GroupWritableOnlyParentIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    // Mode 0775 sets S_IWGRP but not S_IWOTH. This isolates the group-writable
    // bit: a test using 0777 would still pass if the check were narrowed to
    // S_IWOTH only, so this case specifically locks in the S_IWGRP check.
    const std::string dir = MakeSubdir("grpwrite_parent", 0755);
    ASSERT_EQ(0, ::chown(dir.c_str(), 0, 0)) << std::strerror(errno);
    ASSERT_EQ(0, ::chmod(dir.c_str(), 0775)) << std::strerror(errno); // group-writable; set after creation to bypass umask
    const std::string file = dir + "/test.mof";
    ASSERT_TRUE(CreateFile(file, 0600)) << std::strerror(errno);
    ASSERT_EQ(0, ::chown(file.c_str(), 0, 0)) << std::strerror(errno);

    EXPECT_TRUE(RefuseWritableParentDir(file, nullptr));
}

TEST_F(RefuseWritableParentDirTest, NonExistentParentIsRefused)
{
    // The parent directory does not exist, so stat() fails. The function must
    // fail closed (refuse) rather than silently accept. No root required.
    const std::string file = TempPath("no_such_dir/test.mof");
    EXPECT_TRUE(RefuseWritableParentDir(file, nullptr));
}

TEST_F(RefuseWritableParentDirTest, RootDirectoryIsAccepted)
{
    // "/" is root-owned and not world-writable on any sane system.
    // No file is created; the function only stats the parent directory.
    EXPECT_FALSE(RefuseWritableParentDir("/foo.mof", nullptr));
}

// ---------------------------------------------------------------------------
// OpenVerifiedInput
// ---------------------------------------------------------------------------

TEST_F(OpenVerifiedInputTest, SafeFileIsOpened)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string path = TempPath("safe_file.mof");
    ASSERT_TRUE(CreateFile(path, 0600)) << std::strerror(errno);
    // Write known content so we can confirm the returned fd is readable and
    // positioned at the start of the file.
    {
        const int wfd = ::open(path.c_str(), O_WRONLY | O_TRUNC);
        ASSERT_GE(wfd, 0) << std::strerror(errno);
        const char payload[] = "hello";
        ASSERT_EQ(5, ::write(wfd, payload, 5)) << std::strerror(errno);
        ::close(wfd);
    }
    ASSERT_EQ(0, ::chown(path.c_str(), 0, 0)) << std::strerror(errno);

    const auto result = OpenVerifiedInput(path, nullptr);
    ASSERT_TRUE(result.HasValue());
    const int fd = result.Value();

    // O_NONBLOCK is set only to avoid blocking on a FIFO during open(); the
    // implementation must clear it once the file is confirmed regular so the
    // caller's read loop has plain blocking semantics. Verify it is cleared.
    const int flags = ::fcntl(fd, F_GETFL);
    ASSERT_GE(flags, 0) << std::strerror(errno);
    EXPECT_EQ(0, flags & O_NONBLOCK);

    // The fd must be readable and start at offset 0.
    char buf[8] = {0};
    const ssize_t n = ::read(fd, buf, sizeof(buf));
    EXPECT_EQ(5, n) << std::strerror(errno);
    EXPECT_EQ(std::string("hello"), std::string(buf, (n > 0) ? static_cast<size_t>(n) : 0));
    ::close(fd);
}

TEST_F(OpenVerifiedInputTest, NonExistentPathIsRefused)
{
    // The generic open()-failure branch (distinct from the ELOOP symlink
    // branch) must surface an error; the errno is propagated as the error
    // code. No root required.
    const std::string path = TempPath("does_not_exist.mof");
    const auto result = OpenVerifiedInput(path, nullptr);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(ENOENT, result.Error().code);
}

TEST_F(OpenVerifiedInputTest, DirectoryIsRefused)
{
    // A directory opens successfully but fails the S_ISREG check. This covers
    // the non-regular-file rejection without root (the FIFO case is root-gated,
    // so non-root runs would otherwise have no coverage of this branch). The
    // S_ISREG check runs before the ownership check, so a non-root-owned
    // directory is still refused here as "not a regular file".
    const std::string dir = MakeSubdir("a_directory", 0755);
    const auto result = OpenVerifiedInput(dir, nullptr);
    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(ENOTSUP, result.Error().code);
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

TEST_F(OpenVerifiedInputTest, SymlinkIsRefused)
{
    const std::string target = TempPath("target.mof");
    const std::string link = TempPath("link.mof");
    ASSERT_TRUE(CreateFile(target, 0600)) << std::strerror(errno);
    ASSERT_EQ(0, ::symlink(target.c_str(), link.c_str())) << std::strerror(errno);

    const auto result = OpenVerifiedInput(link, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (!result.HasValue())
    {
        // O_NOFOLLOW makes the kernel refuse the final-component symlink
        // atomically with ELOOP, closing the lstat-then-open TOCTOU window.
        EXPECT_EQ(ELOOP, result.Error().code);
    }
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

TEST_F(OpenVerifiedInputTest, GroupWritableFileIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string path = TempPath("grpwrite.mof");
    ASSERT_TRUE(CreateFile(path, 0600)) << std::strerror(errno);
    ASSERT_EQ(0, ::chmod(path.c_str(), 0660)) << std::strerror(errno); // group-writable; set after creation to bypass umask
    ASSERT_EQ(0, ::chown(path.c_str(), 0, 0)) << std::strerror(errno);

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (!result.HasValue())
    {
        EXPECT_EQ(EPERM, result.Error().code);
    }
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

TEST_F(OpenVerifiedInputTest, WorldWritableFileIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string path = TempPath("worldwrite.mof");
    ASSERT_TRUE(CreateFile(path, 0600)) << std::strerror(errno);
    ASSERT_EQ(0, ::chmod(path.c_str(), 0666)) << std::strerror(errno); // world-writable; set after creation to bypass umask
    ASSERT_EQ(0, ::chown(path.c_str(), 0, 0)) << std::strerror(errno);

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (!result.HasValue())
    {
        EXPECT_EQ(EPERM, result.Error().code);
    }
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

TEST_F(OpenVerifiedInputTest, NonRootOwnedFileIsRefused)
{
    const std::string path = TempPath("nonroot.mof");
    ASSERT_TRUE(CreateFile(path, 0600)) << std::strerror(errno);
    // When running as root, explicitly chown to a non-root uid.
    // When running as non-root, the file is already owned by the current user.
    if (::geteuid() == 0)
    {
        ASSERT_EQ(0, ::chown(path.c_str(), 1000, 1000)) << std::strerror(errno);
    }

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (!result.HasValue())
    {
        EXPECT_EQ(EPERM, result.Error().code);
    }
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

TEST_F(OpenVerifiedInputTest, FifoIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string path = TempPath("test.fifo");
    if (::mkfifo(path.c_str(), 0600) != 0)
    {
        GTEST_SKIP() << "mkfifo failed";
    }
    ASSERT_EQ(0, ::chown(path.c_str(), 0, 0)) << std::strerror(errno);

    // A root-owned 0600 FIFO passes the ownership and permission checks but
    // must still be refused because it is not a regular file (it could block
    // the read forever or stream unbounded data).
    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (!result.HasValue())
    {
        EXPECT_EQ(ENOTSUP, result.Error().code);
    }
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

// ---------------------------------------------------------------------------
// RefuseUnsafeLogFile
// ---------------------------------------------------------------------------

TEST_F(RefuseUnsafeLogFileTest, NonExistentPathInSafeParentIsAccepted)
{
    // "/" is root-owned and not world-writable; a not-yet-existing log file
    // there is acceptable (it will be created in the validated directory).
    EXPECT_FALSE(RefuseUnsafeLogFile("/ipsec_test_new_log.log", nullptr));
}

TEST_F(RefuseUnsafeLogFileTest, PathTraversalIsRefused)
{
    EXPECT_TRUE(RefuseUnsafeLogFile("/var/log/../../etc/passwd", nullptr));
}

TEST_F(RefuseUnsafeLogFileTest, SymlinkIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string dir = MakeSubdir("log_dir", 0755);
    ASSERT_EQ(0, ::chown(dir.c_str(), 0, 0)) << std::strerror(errno);
    const std::string target = dir + "/target";
    const std::string link = dir + "/link.log";
    ASSERT_TRUE(CreateFile(target, 0600)) << std::strerror(errno);
    ASSERT_EQ(0, ::chown(target.c_str(), 0, 0)) << std::strerror(errno);
    ASSERT_EQ(0, ::symlink(target.c_str(), link.c_str())) << std::strerror(errno);

    EXPECT_TRUE(RefuseUnsafeLogFile(link, nullptr));
}

TEST_F(RefuseUnsafeLogFileTest, WritableParentIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string dir = MakeSubdir("log_writable_dir", 0755);
    ASSERT_EQ(0, ::chown(dir.c_str(), 0, 0)) << std::strerror(errno);
    ASSERT_EQ(0, ::chmod(dir.c_str(), 0777)) << std::strerror(errno); // world-writable; set after creation to bypass umask

    EXPECT_TRUE(RefuseUnsafeLogFile(dir + "/new.log", nullptr));
}

TEST_F(RefuseUnsafeLogFileTest, RootOwnedRegularFileIsAccepted)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string dir = MakeSubdir("log_safe_dir", 0755);
    ASSERT_EQ(0, ::chown(dir.c_str(), 0, 0)) << std::strerror(errno);
    const std::string file = dir + "/safe.log";
    ASSERT_TRUE(CreateFile(file, 0600)) << std::strerror(errno);
    ASSERT_EQ(0, ::chown(file.c_str(), 0, 0)) << std::strerror(errno);

    EXPECT_FALSE(RefuseUnsafeLogFile(file, nullptr));
}

TEST_F(RefuseUnsafeLogFileTest, NonRootOwnedExistingFileIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    // Parent is root-owned and safe (passes RefuseWritableParentDir), but the
    // existing log file itself is owned by a non-root user. This exercises the
    // file-level ownership branch, which is distinct from the parent-directory
    // ownership check.
    const std::string dir = MakeSubdir("log_nonroot_owner_dir", 0755);
    ASSERT_EQ(0, ::chown(dir.c_str(), 0, 0)) << std::strerror(errno);
    const std::string file = dir + "/owned.log";
    ASSERT_TRUE(CreateFile(file, 0600)) << std::strerror(errno);
    ASSERT_EQ(0, ::chown(file.c_str(), 1000, 1000)) << std::strerror(errno);

    EXPECT_TRUE(RefuseUnsafeLogFile(file, nullptr));
}

TEST_F(RefuseUnsafeLogFileTest, GroupWritableExistingFileIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    // Parent is root-owned and safe, and the existing log file is root-owned,
    // but the file is group-writable. This exercises the file-level mode
    // branch, which is distinct from the parent-directory writability check
    // (covered by WritableParentIsRefused). Mode 0660 sets S_IWGRP only.
    const std::string dir = MakeSubdir("log_grpwrite_file_dir", 0755);
    ASSERT_EQ(0, ::chown(dir.c_str(), 0, 0)) << std::strerror(errno);
    const std::string file = dir + "/grpwrite.log";
    ASSERT_TRUE(CreateFile(file, 0600)) << std::strerror(errno);
    ASSERT_EQ(0, ::chmod(file.c_str(), 0660)) << std::strerror(errno); // group-writable; set after creation to bypass umask
    ASSERT_EQ(0, ::chown(file.c_str(), 0, 0)) << std::strerror(errno);

    EXPECT_TRUE(RefuseUnsafeLogFile(file, nullptr));
}

TEST_F(RefuseUnsafeLogFileTest, NonRegularExistingFileIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    // A root-owned 0600 FIFO in a safe parent passes the ownership and mode
    // checks but must be refused because it is not a regular file. This
    // exercises the S_ISREG branch of RefuseUnsafeLogFile (separate from the
    // one in OpenVerifiedInput).
    const std::string dir = MakeSubdir("log_fifo_dir", 0755);
    ASSERT_EQ(0, ::chown(dir.c_str(), 0, 0)) << std::strerror(errno);
    const std::string fifo = dir + "/log.fifo";
    if (::mkfifo(fifo.c_str(), 0600) != 0)
    {
        GTEST_SKIP() << "mkfifo failed";
    }
    ASSERT_EQ(0, ::chown(fifo.c_str(), 0, 0)) << std::strerror(errno);

    EXPECT_TRUE(RefuseUnsafeLogFile(fifo, nullptr));
}
