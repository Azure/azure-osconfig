// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <IterateUsers.h>
#include <cerrno>

namespace ComplianceEngine
{
UsersIterator::UsersIterator()
    : pwent(nullptr)
{
}
UsersIterator::UsersIterator(bool)
{
    next();
}
UsersIterator::~UsersIterator()
{
    if (pwent != nullptr)
    {
        endpwent();
    }
}

UsersIterator::reference UsersIterator::operator*()
{
    return pwent;
}
UsersIterator::pointer UsersIterator::operator->()
{
    return &pwent;
}

UsersIterator& UsersIterator::operator++()
{
    next();
    return *this;
}

UsersIterator UsersIterator::operator++(int)
{
    UsersIterator tmp = *this;
    next();
    return tmp;
}

bool UsersIterator::operator==(const UsersIterator& other) const
{
    return pwent == other.pwent;
}

bool UsersIterator::operator!=(const UsersIterator& other) const
{
    return !(*this == other);
}

void UsersIterator::next()
{
    pwent = getpwent();
    if (pwent == nullptr)
    {
        endpwent();
    }
}

UsersIterator UsersRange::begin()
{
    return UsersIterator(true);
}
UsersIterator UsersRange::end()
{
    return UsersIterator();
}

} // namespace ComplianceEngine
