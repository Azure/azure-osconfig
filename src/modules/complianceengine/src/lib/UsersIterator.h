// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_USERS_ITERATOR_H
#define COMPLIANCE_USERS_ITERATOR_H

#include <Logging.h>
#include <MmiResults.h>
#include <ReentrantIterator.h>
#include <Result.h>
#include <pwd.h>
#include <vector>

namespace ComplianceEngine
{
class UsersRange;
class UsersIterator : public ReentrantIterator<struct passwd, UsersRange>
{
public:
    explicit UsersIterator(const UsersRange* range)
        : ReentrantIterator(range, fgetpwent_r)
    {
    }
};

class UsersRange : public ReentrantIteratorRange<UsersIterator, UsersRange>
{
    UsersRange() = delete;
    UsersRange(FILE* stream, OsConfigLogHandle logHandle) noexcept;

public:
    static Result<UsersRange> Make(OsConfigLogHandle logHandle);
    static Result<UsersRange> Make(std::string path, OsConfigLogHandle logHandle);
};
} // namespace ComplianceEngine

#endif // COMPLIANCE_USERS_ITERATOR_H
