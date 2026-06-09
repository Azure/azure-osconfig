// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Tests for InputSecurity helpers.
//
// Most tests create temporary files/directories under /tmp using POSIX APIs
// so they can set ownership and permissions precisely. Tests that require
// root (e.g. chown to root) are skipped when running as non-root.

#include <InputSecurity.hpp>
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

// ---------------------------------------------------------------------------
// RefusePathTraversal
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

// ---------------------------------------------------------------------------
// RefuseWritableParentDir
// ---------------------------------------------------------------------------

TEST(RefuseWritableParentDirTest, ParentRootOwnedNotWritableIsAccepted)
{
    // /tmp is always present; we create a file there but check that the
    // function returns false for a file whose parent is not group/world-writable.
    // /tmp is typically 1777; create a subdirectory with safe permissions.
    const std::string dir = "/tmp/ipsec_test_safe_parent";
    ::mkdir(dir.c_str(), 0755);
    // This will only pass when we can chown the dir to root.
    if (::chown(dir.c_str(), 0, 0) != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    ::chmod(dir.c_str(), 0755);
    const std::string file = dir + "/test.mof";
    CreateFile(file, 0600);
    ::chown(file.c_str(), 0, 0);

    EXPECT_FALSE(RefuseWritableParentDir(file, nullptr));
    ::unlink(file.c_str());
    ::rmdir(dir.c_str());
}

TEST(RefuseWritableParentDirTest, WorldWritableParentIsRefused)
{
    const std::string dir = "/tmp/ipsec_test_writable_parent";
    ::mkdir(dir.c_str(), 0755);
    if (::chown(dir.c_str(), 0, 0) != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    ::chmod(dir.c_str(), 0777); // world-writable; set after creation to bypass umask
    const std::string file = dir + "/test.mof";
    CreateFile(file, 0600);
    ::chown(file.c_str(), 0, 0);

    EXPECT_TRUE(RefuseWritableParentDir(file, nullptr));
    ::unlink(file.c_str());
    ::rmdir(dir.c_str());
}

TEST(RefuseWritableParentDirTest, NonRootOwnedParentIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "cannot create root-owned directories as non-root";
    }
    const std::string dir = "/tmp/ipsec_test_nonroot_parent";
    ::mkdir(dir.c_str(), 0755);
    ::chown(dir.c_str(), 1000, 1000); // non-root owner
    const std::string file = dir + "/test.mof";
    CreateFile(file, 0600);

    EXPECT_TRUE(RefuseWritableParentDir(file, nullptr));
    ::unlink(file.c_str());
    ::rmdir(dir.c_str());
}

TEST(RefuseWritableParentDirTest, RootDirectoryIsAccepted)
{
    // "/" is root-owned and not world-writable on any sane system.
    EXPECT_FALSE(RefuseWritableParentDir("/foo.mof", nullptr));
}

// ---------------------------------------------------------------------------
// OpenVerifiedInput
// ---------------------------------------------------------------------------

TEST(OpenVerifiedInputTest, SafeFileIsOpened)
{
    const std::string path = "/tmp/ipsec_test_safe_file.mof";
    CreateFile(path, 0600);
    if (::chown(path.c_str(), 0, 0) != 0)
    {
        ::unlink(path.c_str());
        GTEST_SKIP() << "chown requires root";
    }

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_TRUE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
    ::unlink(path.c_str());
}

TEST(OpenVerifiedInputTest, SymlinkIsRefused)
{
    const std::string target = "/tmp/ipsec_test_target.mof";
    const std::string link = "/tmp/ipsec_test_link.mof";
    CreateFile(target, 0600);
    ::symlink(target.c_str(), link.c_str());

    const auto result = OpenVerifiedInput(link, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
    ::unlink(link.c_str());
    ::unlink(target.c_str());
}

TEST(OpenVerifiedInputTest, GroupWritableFileIsRefused)
{
    const std::string path = "/tmp/ipsec_test_grpwrite.mof";
    CreateFile(path, 0600);
    ::chmod(path.c_str(), 0660); // group-writable; set after creation to bypass umask
    if (::chown(path.c_str(), 0, 0) != 0)
    {
        ::unlink(path.c_str());
        GTEST_SKIP() << "chown requires root";
    }

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
    ::unlink(path.c_str());
}

TEST(OpenVerifiedInputTest, WorldWritableFileIsRefused)
{
    const std::string path = "/tmp/ipsec_test_worldwrite.mof";
    CreateFile(path, 0600);
    ::chmod(path.c_str(), 0666); // world-writable; set after creation to bypass umask
    if (::chown(path.c_str(), 0, 0) != 0)
    {
        ::unlink(path.c_str());
        GTEST_SKIP() << "chown requires root";
    }

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
    ::unlink(path.c_str());
}

TEST(OpenVerifiedInputTest, NonRootOwnedFileIsRefused)
{
    const std::string path = "/tmp/ipsec_test_nonroot.mof";
    CreateFile(path, 0600);
    // Leave ownership as the current user (non-root when not root)
    if (::geteuid() == 0)
    {
        ::chown(path.c_str(), 1000, 1000);
    }
    // When running as non-root, the file is already owned by the current user.

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
    ::unlink(path.c_str());
}

TEST(OpenVerifiedInputTest, FifoIsRefused)
{
    const std::string path = "/tmp/ipsec_test_fifo.mof";
    ::unlink(path.c_str());
    if (::mkfifo(path.c_str(), 0600) != 0)
    {
        GTEST_SKIP() << "mkfifo failed";
    }
    if (::chown(path.c_str(), 0, 0) != 0)
    {
        ::unlink(path.c_str());
        GTEST_SKIP() << "chown requires root";
    }

    // A root-owned 0600 FIFO passes the ownership and permission checks but
    // must still be refused because it is not a regular file (it could block
    // the read forever or stream unbounded data).
    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
    ::unlink(path.c_str());
}

// ---------------------------------------------------------------------------
// RefuseUnsafeLogFile
// ---------------------------------------------------------------------------

TEST(RefuseUnsafeLogFileTest, NonExistentPathInSafeParentIsAccepted)
{
    // "/" is root-owned and not world-writable; a not-yet-existing log file
    // there is acceptable (it will be created in the validated directory).
    EXPECT_FALSE(RefuseUnsafeLogFile("/ipsec_test_new_log.log", nullptr));
}

TEST(RefuseUnsafeLogFileTest, PathTraversalIsRefused)
{
    EXPECT_TRUE(RefuseUnsafeLogFile("/var/log/../../etc/passwd", nullptr));
}

TEST(RefuseUnsafeLogFileTest, SymlinkIsRefused)
{
    const std::string dir = "/tmp/ipsec_test_log_dir";
    ::mkdir(dir.c_str(), 0755);
    if (::chown(dir.c_str(), 0, 0) != 0)
    {
        ::rmdir(dir.c_str());
        GTEST_SKIP() << "chown requires root";
    }
    ::chmod(dir.c_str(), 0755);
    const std::string target = dir + "/target";
    const std::string link = dir + "/link.log";
    CreateFile(target, 0600);
    ::chown(target.c_str(), 0, 0);
    ::symlink(target.c_str(), link.c_str());

    EXPECT_TRUE(RefuseUnsafeLogFile(link, nullptr));

    ::unlink(link.c_str());
    ::unlink(target.c_str());
    ::rmdir(dir.c_str());
}

TEST(RefuseUnsafeLogFileTest, WritableParentIsRefused)
{
    const std::string dir = "/tmp/ipsec_test_log_writable_dir";
    ::mkdir(dir.c_str(), 0755);
    if (::chown(dir.c_str(), 0, 0) != 0)
    {
        ::rmdir(dir.c_str());
        GTEST_SKIP() << "chown requires root";
    }
    ::chmod(dir.c_str(), 0777); // world-writable; set after creation to bypass umask

    EXPECT_TRUE(RefuseUnsafeLogFile(dir + "/new.log", nullptr));

    ::rmdir(dir.c_str());
}

TEST(RefuseUnsafeLogFileTest, RootOwnedRegularFileIsAccepted)
{
    const std::string dir = "/tmp/ipsec_test_log_safe_dir";
    ::mkdir(dir.c_str(), 0755);
    if (::chown(dir.c_str(), 0, 0) != 0)
    {
        ::rmdir(dir.c_str());
        GTEST_SKIP() << "chown requires root";
    }
    ::chmod(dir.c_str(), 0755);
    const std::string file = dir + "/safe.log";
    CreateFile(file, 0600);
    ::chown(file.c_str(), 0, 0);

    EXPECT_FALSE(RefuseUnsafeLogFile(file, nullptr));

    ::unlink(file.c_str());
    ::rmdir(dir.c_str());
}
