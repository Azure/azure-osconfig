// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_TYPE_TRAITS_H
#define COMPLIANCE_TYPE_TRAITS_H

#include <type_traits>

namespace compliance
{
    template <typename T>
    constexpr bool noexcept_copyable() noexcept
    {
        return noexcept(std::is_copy_constructible<T>::value);
    }

    template <typename T>
    constexpr bool noexcept_movable() noexcept
    {
        return noexcept(std::is_move_constructible<T>::value);
    }
} // namespace compliance

#endif // COMPLIANCE_TYPE_TRAITS_H
