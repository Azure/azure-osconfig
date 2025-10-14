// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_OPTIONAL_H
#define COMPLIANCEENGINE_OPTIONAL_H

#include "TypeTraits.h"

#include <memory>

namespace ComplianceEngine
{
template <typename T>
class Optional
{
    std::unique_ptr<T> mValue;

public:
    Optional() = default;
    Optional(T value)
    {
        mValue.reset(new T(std::move(value)));
    }
    ~Optional() = default;

    Optional(const Optional& other)
    {
        if (other.mValue != nullptr)
        {
            mValue.reset(new T(*other.mValue));
        }
        else
        {
            mValue.reset();
        }
    }

    Optional(Optional&& other) noexcept
        : mValue(std::move(other.mValue))
    {
    }

    Optional& operator=(const Optional& other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (other.mValue != nullptr)
        {
            mValue.reset(new T(*other.mValue));
        }
        else
        {
            mValue.reset();
        }

        return *this;
    }

    Optional& operator=(T value)
    {
        if (mValue != nullptr)
        {
            *mValue = std::move(value);
        }
        else
        {
            mValue.reset(new T(std::move(value)));
        }
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        mValue = std::move(other.mValue);
        return *this;
    }

    bool HasValue() const
    {
        return mValue != nullptr;
    }

    T ValueOr(T default_value) const noexcept(NoexceptCopyable<T>())
    {
        if (mValue == nullptr)
        {
            return default_value;
        }

        return *mValue;
    }

    const T& Value() const&
    {
        return *mValue;
    }

    T Value() && noexcept(NoexceptMovable<T>())
    {
        return std::move(*mValue);
    }

    T& Value() &
    {
        return *mValue;
    }

    const T* operator->() const&
    {
        return mValue.get();
    }

    T* operator->() &
    {
        return mValue.get();
    }

    operator bool() const noexcept
    {
        return HasValue();
    }

    void Reset()
    {
        mValue.reset();
    }
};
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_OPTIONAL_H
