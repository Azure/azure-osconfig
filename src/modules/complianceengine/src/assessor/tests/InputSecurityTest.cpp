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
        ::mkdir(path.c_str(), mode);
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

// ---------------------------------------------------------------------------
// RefuseWritableParentDir
// ---------------------------------------------------------------------------

TEST_F(RefuseWritableParentDirTest, ParentRootOwnedNotWritableIsAccepted)
{
    const std::string dir = MakeSubdir("safe_parent", 0755);
    if (::chown(dir.c_str(), 0, 0) != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string file = dir + "/test.mof";
    CreateFile(file, 0600);
    ::chown(file.c_str(), 0, 0);

    EXPECT_FALSE(RefuseWritableParentDir(file, nullptr));
}

TEST_F(RefuseWritableParentDirTest, WorldWritableParentIsRefused)
{
    const std::string dir = MakeSubdir("writable_parent", 0755);
    if (::chown(dir.c_str(), 0, 0) != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    ::chmod(dir.c_str(), 0777); // world-writable; set after creation to bypass umask
    const std::string file = dir + "/test.mof";
    CreateFile(file, 0600);
    ::chown(file.c_str(), 0, 0);

    EXPECT_TRUE(RefuseWritableParentDir(file, nullptr));
}

TEST_F(RefuseWritableParentDirTest, NonRootOwnedParentIsRefused)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "cannot chown to arbitrary uid as non-root";
    }
    const std::string dir = MakeSubdir("nonroot_parent", 0755);
    ::chown(dir.c_str(), 1000, 1000); // non-root owner
    const std::string file = dir + "/test.mof";
    CreateFile(file, 0600);

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
    const std::string path = TempPath("safe_file.mof");
    CreateFile(path, 0600);
    if (::chown(path.c_str(), 0, 0) != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_TRUE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

TEST_F(OpenVerifiedInputTest, SymlinkIsRefused)
{
    const std::string target = TempPath("target.mof");
    const std::string link = TempPath("link.mof");
    CreateFile(target, 0600);
    ::symlink(target.c_str(), link.c_str());

    const auto result = OpenVerifiedInput(link, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

TEST_F(OpenVerifiedInputTest, GroupWritableFileIsRefused)
{
    const std::string path = TempPath("grpwrite.mof");
    CreateFile(path, 0600);
    ::chmod(path.c_str(), 0660); // group-writable; set after creation to bypass umask
    if (::chown(path.c_str(), 0, 0) != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

TEST_F(OpenVerifiedInputTest, WorldWritableFileIsRefused)
{
    const std::string path = TempPath("worldwrite.mof");
    CreateFile(path, 0600);
    ::chmod(path.c_str(), 0666); // world-writable; set after creation to bypass umask
    if (::chown(path.c_str(), 0, 0) != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

TEST_F(OpenVerifiedInputTest, NonRootOwnedFileIsRefused)
{
    const std::string path = TempPath("nonroot.mof");
    CreateFile(path, 0600);
    // When running as root, explicitly chown to a non-root uid.
    // When running as non-root, the file is already owned by the current user.
    if (::geteuid() == 0)
    {
        ::chown(path.c_str(), 1000, 1000);
    }

    const auto result = OpenVerifiedInput(path, nullptr);
    EXPECT_FALSE(result.HasValue());
    if (result.HasValue())
    {
        ::close(result.Value());
    }
}

TEST_F(OpenVerifiedInputTest, FifoIsRefused)
{
    const std::string path = TempPath("test.fifo");
    if (::mkfifo(path.c_str(), 0600) != 0)
    {
        GTEST_SKIP() << "mkfifo failed";
    }
    if (::chown(path.c_str(), 0, 0) != 0)
    {
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
    const std::string dir = MakeSubdir("log_dir", 0755);
    if (::chown(dir.c_str(), 0, 0) != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string target = dir + "/target";
    const std::string link = dir + "/link.log";
    CreateFile(target, 0600);
    ::chown(target.c_str(), 0, 0);
    ::symlink(target.c_str(), link.c_str());

    EXPECT_TRUE(RefuseUnsafeLogFile(link, nullptr));
}

TEST_F(RefuseUnsafeLogFileTest, WritableParentIsRefused)
{
    const std::string dir = MakeSubdir("log_writable_dir", 0755);
    if (::chown(dir.c_str(), 0, 0) != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    ::chmod(dir.c_str(), 0777); // world-writable; set after creation to bypass umask

    EXPECT_TRUE(RefuseUnsafeLogFile(dir + "/new.log", nullptr));
}

TEST_F(RefuseUnsafeLogFileTest, RootOwnedRegularFileIsAccepted)
{
    const std::string dir = MakeSubdir("log_safe_dir", 0755);
    if (::chown(dir.c_str(), 0, 0) != 0)
    {
        GTEST_SKIP() << "chown requires root";
    }
    const std::string file = dir + "/safe.log";
    CreateFile(file, 0600);
    ::chown(file.c_str(), 0, 0);

    EXPECT_FALSE(RefuseUnsafeLogFile(file, nullptr));
}
