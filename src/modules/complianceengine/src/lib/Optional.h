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
    char mBuffer[sizeof(T)];
    T* mValue = nullptr;

public:
    Optional() = default;
    Optional(T mValue)
        : mValue(new (mBuffer) T(std::move(mValue)))
    {
    }

    ~Optional()
    {
        Reset();
    }

    Optional(const Optional& other)
    {
        Reset();
        if (nullptr != other.mValue)
        {
            mValue = new (mBuffer) T(*other.mValue);
        }
        else
        {
            mValue = nullptr;
        }
    }

    Optional(Optional&& other) noexcept
        : mValue(other.mValue)
    {
        other.mValue = nullptr;
    }

    Optional& operator=(const Optional& other)
    {
        if (this == &other)
        {
            return *this;
        }

        Reset();
        if (nullptr != other.mValue)
        {
            mValue = new (mBuffer) T(*other.mValue);
        }
        else
        {
            mValue = nullptr;
        }

        return *this;
    }

    Optional& operator=(T value)
    {
        Reset();
        mValue = new (mBuffer) T(std::move(value));
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Reset();
        mValue = other.mValue;
        other.mValue = nullptr;
        return *this;
    }

    bool HasValue() const
    {
        return mValue != nullptr;
    }

    T ValueOr(T default_value) const noexcept(NoexceptCopyable<T>())
    {
        if (nullptr == mValue)
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
        return mValue;
    }

    T* operator->() &
    {
        return mValue;
    }

    operator bool() const noexcept
    {
        return HasValue();
    }

    void Reset()
    {
        if (nullptr != mValue)
        {
            mValue->~T();
        }
        mValue = nullptr;
    }
};
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_OPTIONAL_H
