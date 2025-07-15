// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_GROUPS_ITERATOR_H
#define COMPLIANCE_GROUPS_ITERATOR_H

#include <Logging.h>
#include <MmiResults.h>
#include <Result.h>
#include <grp.h>
#include <vector>

namespace ComplianceEngine
{
class GroupsRange;
class GroupsIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = struct group;
    using pointer = struct group*;
    using reference = struct group&;
    GroupsIterator(const GroupsRange* range);
    ~GroupsIterator() = default;
    reference operator*();
    pointer operator->();

    GroupsIterator& operator++();
    GroupsIterator operator++(int);
    bool operator==(const GroupsIterator& other) const;
    bool operator!=(const GroupsIterator& other) const;

    void next(); // NOLINT(*-identifier-naming)

private:
    group mStorage;
    const GroupsRange* mRange;
    std::vector<char> mBuffer;
};
class GroupsRange
{
    FILE* mStream;
    OsConfigLogHandle mLog;

    GroupsRange() = delete;
    GroupsRange(FILE* stream, OsConfigLogHandle logHandle) noexcept;

public:
    static Result<GroupsRange> Make(OsConfigLogHandle logHandle);
    static Result<GroupsRange> Make(std::string path, OsConfigLogHandle logHandle);
    GroupsRange(const GroupsRange&) = delete;
    GroupsRange(GroupsRange&&) noexcept;
    GroupsRange& operator=(const GroupsRange&) = delete;
    GroupsRange& operator=(GroupsRange&&) noexcept;
    ~GroupsRange();

    FILE* GetStream() const noexcept
    {
        return mStream;
    }

    OsConfigLogHandle GetLogHandle() const noexcept
    {
        return mLog;
    }

    GroupsIterator begin(); // NOLINT(*-identifier-naming)
    GroupsIterator end();   // NOLINT(*-identifier-naming)
};
} // namespace ComplianceEngine

#endif // COMPLIANCE_GROUPS_ITERATOR_H
