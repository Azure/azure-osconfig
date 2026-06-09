// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <InputSecurity.hpp>
#include <Logging.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using ComplianceEngine::Error;
using ComplianceEngine::Result;

namespace ComplianceEngine
{
namespace Assessor
{

bool RefusePathTraversal(const std::string& path, OsConfigLogHandle logHandle)
{
    // Reject any path component equal to "..". We check for the four forms in
    // which ".." can appear: as a prefix ("../"), embedded ("/../"), as a
    // suffix ("/.." or the path is literally ".."), or the whole string.
    if (path == ".." || path.compare(0, 3, "../") == 0 || path.find("/../") != std::string::npos ||
        (path.size() >= 3 && path.compare(path.size() - 3, 3, "/..") == 0))
    {
        OsConfigLogError(logHandle, "Refusing input path '%s': contains path traversal sequence '..'.", path.c_str());
        return true;
    }
    return false;
}

bool RefuseWritableParentDir(const std::string& path, OsConfigLogHandle logHandle)
{
    const size_t slash = path.rfind('/');
    std::string dir;
    if (slash == std::string::npos)
    {
        dir = ".";
    }
    else if (slash == 0)
    {
        dir = "/";
    }
    else
    {
        dir = path.substr(0, slash);
    }

struct stat st;
if (::stat(dir.c_str(), &st) != 0)
{
    const std::string msg = std::strerror(errno);
    OsConfigLogError(logHandle, "Refusing to use path '%s': failed to stat parent directory '%s': %s", path.c_str(), dir.c_str(), msg.c_str());
    return true;
}
    if (st.st_uid != 0)
    {
        OsConfigLogError(logHandle, "Refusing to read input file '%s': parent directory '%s' not owned by root (uid %u).", path.c_str(), dir.c_str(),
            static_cast<unsigned>(st.st_uid));
        return true;
    }
    if (st.st_mode & (S_IWGRP | S_IWOTH))
    {
        OsConfigLogError(logHandle, "Refusing to read input file '%s': parent directory '%s' writable by group or others (mode %04o).", path.c_str(),
            dir.c_str(), static_cast<unsigned>(st.st_mode & 07777));
        return true;
    }
    return false;
}

Result<int> OpenVerifiedInput(const std::string& path, OsConfigLogHandle logHandle)
{
    const int fd = ::open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0)
    {
        if (errno == ELOOP)
        {
            OsConfigLogError(logHandle, "Refusing to open input file '%s': path is a symlink.", path.c_str());
            return Error("path is a symlink");
        }
        const std::string msg = std::strerror(errno);
        OsConfigLogError(logHandle, "Failed to open input file '%s': %s", path.c_str(), msg.c_str());
        return Error(msg);
    }

    struct stat st;
    if (::fstat(fd, &st) != 0)
    {
        const std::string msg = std::strerror(errno);
        OsConfigLogError(logHandle, "Failed to stat input file '%s': %s", path.c_str(), msg.c_str());
        ::close(fd);
        return Error(msg);
    }
    if (!S_ISREG(st.st_mode))
    {
        // Refuse FIFOs, devices, sockets, and directories. A FIFO would let an
        // attacker block the read indefinitely or stream unbounded data; a
        // character device such as /dev/zero would do the same. Streaming
        // inputs (pipes, process substitution) must be supplied via stdin,
        // which deliberately bypasses these on-disk integrity checks.
        OsConfigLogError(logHandle, "Refusing to read input file '%s': not a regular file.", path.c_str());
        ::close(fd);
        return Error("not a regular file");
    }
    if (st.st_uid != 0)
    {
        OsConfigLogError(logHandle, "Refusing to read input file '%s': not owned by root (uid %u).", path.c_str(), static_cast<unsigned>(st.st_uid));
        ::close(fd);
        return Error("not owned by root");
    }
    if (st.st_mode & (S_IWGRP | S_IWOTH))
    {
        OsConfigLogError(logHandle, "Refusing to read input file '%s': writable by group or others (mode %04o).", path.c_str(),
            static_cast<unsigned>(st.st_mode & 07777));
        ::close(fd);
        return Error("writable by group or others");
    }
    return fd;
}

bool RefuseUnsafeLogFile(const std::string& path, OsConfigLogHandle logHandle)
{
    // The log file is opened (and chmod'd) by the shared logging code while we
    // run as root, and that open follows symlinks. Apply the same posture as
    // --input so an attacker cannot redirect root's writes:
    //   - reject path traversal,
    //   - require a root-owned, non-group/world-writable parent directory
    //     (prevents a rename-swap onto a hostile target),
    //   - if the path already exists, reject symlinks, non-root ownership, or
    //     group/world-writable modes. A non-existent path is fine because it
    //     will be created inside the parent directory we just validated.
    if (RefusePathTraversal(path, logHandle))
    {
        return true;
    }
    if (RefuseWritableParentDir(path, logHandle))
    {
        return true;
    }

struct stat st;
if (::lstat(path.c_str(), &st) != 0)
{
    if (errno == ENOENT)
    {
        // Does not exist yet; it will be created in the parent directory already verified to be root-owned and safe.
        return false;
    }
    const std::string msg = std::strerror(errno);
    OsConfigLogError(logHandle, "Refusing to use log file '%s': failed to inspect path: %s", path.c_str(), msg.c_str());
    return true;
}
    if (S_ISLNK(st.st_mode))
    {
        OsConfigLogError(logHandle, "Refusing to use log file '%s': path is a symlink.", path.c_str());
        return true;
    }
    if (!S_ISREG(st.st_mode))
    {
        OsConfigLogError(logHandle, "Refusing to use log file '%s': not a regular file.", path.c_str());
        return true;
    }
    if (st.st_uid != 0)
    {
        OsConfigLogError(logHandle, "Refusing to use log file '%s': not owned by root (uid %u).", path.c_str(), static_cast<unsigned>(st.st_uid));
        return true;
    }
    if (st.st_mode & (S_IWGRP | S_IWOTH))
    {
        OsConfigLogError(logHandle, "Refusing to use log file '%s': writable by group or others (mode %04o).", path.c_str(),
            static_cast<unsigned>(st.st_mode & 07777));
        return true;
    }
    return false;
}

} // namespace Assessor
} // namespace ComplianceEngine
