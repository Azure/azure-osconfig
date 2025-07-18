// // Copyright (c) Microsoft Corporation. All rights reserved.
// // Licensed under the MIT License.

#include <UsersIterator.h>

namespace ComplianceEngine
{
UsersRange::UsersRange(FILE* stream, OsConfigLogHandle logHandle) noexcept
    : ReentrantIteratorRange<UsersIterator, UsersRange>(stream, logHandle)
{
}

Result<UsersRange> UsersRange::Make(OsConfigLogHandle logHandle)
{
    return Make("/etc/passwd", logHandle);
}

Result<UsersRange> UsersRange::Make(std::string path, OsConfigLogHandle logHandle)
{
    auto stream = fopen(path.c_str(), "r");
    if (nullptr == stream)
    {
        OsConfigLogError(logHandle, "Failed to open file '%s': %s", path.c_str(), strerror(errno));
        return Error("Failed to create UsersRange: " + std::string(strerror(errno)), errno);
    }

    return UsersRange(stream, logHandle);
}
} // namespace ComplianceEngine
