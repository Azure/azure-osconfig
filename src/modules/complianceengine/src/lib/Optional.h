// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_OPTIONAL_H
#define COMPLIANCEENGINE_OPTIONAL_H

#include "TypeTraits.h"

#include <memory>
#include <stdexcept>

namespace ComplianceEngine
{
template <typename T>
class Optional
{
    alignas(T) char mBuffer[sizeof(T)];
    T* mValue = nullptr;

public:
    Optional() = default;
    Optional(T value)
        : mValue(new (mBuffer) T(std::move(value)))
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

    Optional(Optional&& other) noexcept(NoexceptMovable<T>())
    {
        if (nullptr != other.mValue)
        {
            mValue = new (mBuffer) T(std::move(*other.mValue));
        }
        else
        {
            mValue = nullptr;
        }
        other.Reset();
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

    Optional& operator=(Optional&& other) noexcept(NoexceptMovable<T>())
    {
        if (this == &other)
        {
            return *this;
        }

        Reset();
        if (nullptr != other.mValue)
        {
            mValue = new (mBuffer) T(std::move(*other.mValue));
        }
        else
        {
            mValue = nullptr;
        }
        other.Reset();
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
        CheckValue();
        return *mValue;
    }

    T Value() && noexcept(NoexceptMovable<T>())
    {
        CheckValue();
        return std::move(*mValue);
    }

    T& Value() &
    {
        CheckValue();
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

    void Reset() noexcept(NoexceptDestructible<T>())
    {
        if (nullptr != mValue)
        {
            mValue->~T();
        }
        mValue = nullptr;
    }

private:
    void CheckValue() const
    {
        if (nullptr == mValue)
        {
            throw std::logic_error("Optional: unchecked access to Value");
        }
    }
};
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_OPTIONAL_H
