// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_USERS_ITERATOR_H
#define COMPLIANCE_USERS_ITERATOR_H

#include <Logging.h>
#include <MmiResults.h>
#include <Result.h>
#include <pwd.h>
#include <vector>

namespace ComplianceEngine
{
class UsersRange;
class UsersIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = struct passwd;
    using pointer = struct passwd*;
    using reference = struct passwd&;
    UsersIterator(const UsersRange* range);
    ~UsersIterator() = default;
    reference operator*();
    pointer operator->();

    UsersIterator& operator++();
    UsersIterator operator++(int);
    bool operator==(const UsersIterator& other) const;
    bool operator!=(const UsersIterator& other) const;

    void next(); // NOLINT(*-identifier-naming)

private:
    passwd mStorage;
    const UsersRange* mRange;
    std::vector<char> mBuffer;
};
class UsersRange
{
    FILE* mStream;
    OsConfigLogHandle mLog;

    UsersRange() = delete;
    UsersRange(FILE* stream, OsConfigLogHandle logHandle) noexcept;

public:
    static Result<UsersRange> Make(OsConfigLogHandle logHandle);
    static Result<UsersRange> Make(std::string path, OsConfigLogHandle logHandle);
    UsersRange(const UsersRange&) = delete;
    UsersRange(UsersRange&&) noexcept;
    UsersRange& operator=(const UsersRange&) = delete;
    UsersRange& operator=(UsersRange&&) noexcept;
    ~UsersRange();

    FILE* GetStream() const noexcept
    {
        return mStream;
    }

    OsConfigLogHandle GetLogHandle() const noexcept
    {
        return mLog;
    }

    UsersIterator begin(); // NOLINT(*-identifier-naming)
    UsersIterator end();   // NOLINT(*-identifier-naming)
};
} // namespace ComplianceEngine

#endif // COMPLIANCE_USERS_ITERATOR_H
