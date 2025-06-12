// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ITERATE_USERS_H
#define COMPLIANCE_ITERATE_USERS_H

#include <ContextInterface.h>
#include <IterationHelpers.h>
#include <MmiResults.h>
#include <Result.h>
#include <functional>
#include <pwd.h>

namespace ComplianceEngine
{
class UsersIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = struct passwd*;
    using difference_type = std::ptrdiff_t;
    using pointer = struct passwd**;
    using reference = struct passwd*&;
    UsersIterator();
    UsersIterator(bool);
    ~UsersIterator();
    reference operator*();
    pointer operator->();

    UsersIterator& operator++();

    UsersIterator operator++(int);

    bool operator==(const UsersIterator& other) const;

    bool operator!=(const UsersIterator& other) const;

private:
    void Next();
    struct passwd* pwent;
};
class UsersRange
{
public:
    UsersIterator Begin();
    UsersIterator End();
};
} // namespace ComplianceEngine

#endif // COMPLIANCE_ITERATE_USERS_H
