// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_TYPE_TRAITS_H
#define COMPLIANCE_TYPE_TRAITS_H

#include <type_traits>

namespace compliance
{
template <typename T>
constexpr bool NoexceptCopyable() noexcept
{
    return std::is_nothrow_copy_constructible<T>::value;
}

template <typename T>
constexpr bool NoexceptMovable() noexcept
{
    return std::is_nothrow_move_constructible<T>::value;
}
} // namespace compliance

#endif // COMPLIANCE_TYPE_TRAITS_H
