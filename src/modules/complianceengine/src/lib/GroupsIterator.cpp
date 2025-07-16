// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <GroupsIterator.h>
#include <cerrno>
#include <pwd.h>

namespace ComplianceEngine
{

GroupsRange::GroupsRange(FILE* stream, OsConfigLogHandle logHandle) noexcept
    : ReentrantIteratorRange<GroupsIterator, GroupsRange>(stream, logHandle)
{
}

Result<GroupsRange> GroupsRange::Make(OsConfigLogHandle logHandle)
{
    return Make("/etc/group", logHandle);
}

Result<GroupsRange> GroupsRange::Make(std::string path, OsConfigLogHandle logHandle)
{
    auto stream = fopen(path.c_str(), "r");
    if (nullptr == stream)
    {
        OsConfigLogError(logHandle, "Failed to open file '%s': %s", path.c_str(), strerror(errno));
        return Error("Failed to create GroupsRange: " + std::string(strerror(errno)), errno);
    }

    return GroupsRange(stream, logHandle);
}
} // namespace ComplianceEngine
