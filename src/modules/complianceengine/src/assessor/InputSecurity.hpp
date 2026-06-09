// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ENGINE_ASSESSOR_INPUT_SECURITY_HPP
#define COMPLIANCE_ENGINE_ASSESSOR_INPUT_SECURITY_HPP

#include <Logging.h>
#include <Result.h>
#include <string>

namespace ComplianceEngine
{
namespace Assessor
{

// Returns true (and logs an error) if the parent directory of `path` is not
// owned by root or is writable by group/others. A writable parent directory
// enables a rename-swap attack: an attacker can unlink the file after it has
// been validated and replace it with a hostile one before it is read.
// Uses stat() (follows symlinks) so intermediate directory symlinks do not
// defeat the check.
bool RefuseWritableParentDir(const std::string& path, OsConfigLogHandle logHandle);

// Opens `path` safely for reading, refusing symlinks and verifying ownership
// and permissions via fstat() on the resulting fd.
//
// Security properties:
//   - O_NOFOLLOW causes the kernel to refuse a symlink in the final path
//     component atomically (returns ELOOP), eliminating the lstat-then-open
//     TOCTOU window.
//   - O_NONBLOCK prevents the open() call from blocking on a FIFO (without
//     it, O_RDONLY on a FIFO stalls until a writer appears). For regular
//     files O_NONBLOCK has no effect on Linux reads.
//   - fstat() checks the inode we actually hold, not a potentially-swapped
//     path entry. The file must be a regular file, root-owned, and not
//     group/world-writable. Non-regular files (FIFOs, devices) are refused so
//     they cannot block the read or stream unbounded data; streaming inputs
//     must be supplied via stdin instead.
//   - O_CLOEXEC prevents accidental fd inheritance into child processes.
//
// Returns the open fd on success (caller owns it and must ::close() it),
// or an Error on any failure (error already logged).
ComplianceEngine::Result<int> OpenVerifiedInput(const std::string& path, OsConfigLogHandle logHandle);

// Rejects path traversal sequences ("/../", "/.." suffix, "../" prefix, or
// a bare ".."). These sequences can be used to escape an expected directory
// even when all other permission checks pass.
// Returns true (and logs an error) if the path contains traversal components.
bool RefusePathTraversal(const std::string& path, OsConfigLogHandle logHandle);

// Returns true (and logs an error) if the log-file `path` is unsafe to open
// while running as root. The shared logging code opens the log with a
// symlink-following append and then chmod's it, so an attacker-controlled
// symlink or a writable parent directory could redirect root's writes onto a
// sensitive file. This applies the same posture as OpenVerifiedInput: rejects
// path traversal, requires a root-owned non-writable parent directory, and (if
// the path already exists) rejects symlinks, non-regular files, non-root
// ownership, and group/world-writable modes. A non-existent path is allowed
// because it is created inside the validated parent directory.
//
// Known limitation (TOCTOU): unlike OpenVerifiedInput, this cannot fstat() a
// held fd, because the shared OpenLog() API is path-only and TrimLog()
// re-opens the path on every rotation. The check is therefore an lstat() of
// the path shortly before OpenLog() (and each later rotation) re-resolves it
// with symlink-following fopen(), leaving a small check-to-use window. The
// required root-owned, non-writable parent directory closes that window in
// practice by preventing any swap of the entry. See the threat-model comment
// in Main.cpp for details.
bool RefuseUnsafeLogFile(const std::string& path, OsConfigLogHandle logHandle);

} // namespace Assessor
} // namespace ComplianceEngine

#endif // COMPLIANCE_ENGINE_ASSESSOR_INPUT_SECURITY_HPP
