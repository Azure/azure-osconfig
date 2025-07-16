// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_GROUPS_ITERATOR_H
#define COMPLIANCE_GROUPS_ITERATOR_H

#include <Logging.h>
#include <MmiResults.h>
#include <ReentrantIterator.h>
#include <Result.h>
#include <grp.h>
#include <vector>

namespace ComplianceEngine
{
class GroupsRange;
class GroupsIterator : public ReentrantIterator<struct group, GroupsRange>
{
public:
    explicit GroupsIterator(const GroupsRange* range)
        : ReentrantIterator(range, fgetgrent_r)
    {
    }
};
class GroupsRange : public ReentrantIteratorRange<GroupsIterator, GroupsRange>
{
    GroupsRange() = delete;
    GroupsRange(FILE* stream, OsConfigLogHandle logHandle) noexcept;

public:
    static Result<GroupsRange> Make(OsConfigLogHandle logHandle);
    static Result<GroupsRange> Make(std::string path, OsConfigLogHandle logHandle);
};
} // namespace ComplianceEngine

#endif // COMPLIANCE_GROUPS_ITERATOR_H
