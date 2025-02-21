// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_RESULT_H
#define COMPLIANCE_RESULT_H

#include "TypeTraits.h"

#include <string>

namespace compliance
{
struct Error
{
    int code = -1;
    std::string message;

    Error(std::string message, int code)
        : code(code),
          message(std::move(message))
    {
    }
    Error(std::string message)
        : message(std::move(message))
    {
    }
    Error(const Error& other)
        : code(other.code),
          message(other.message)
    {
    }
    Error(Error&& other) noexcept
        : code(other.code),
          message(std::move(other.message))
    {
    }
    Error& operator=(const Error& other)
    {
        if (this == &other)
        {
            return *this;
        }

        code = other.code;
        message = other.message;
        return *this;
    }

    Error& operator=(Error&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        code = other.code;
        message = std::move(other.message);
        return *this;
    }
    ~Error() = default;
};

template <typename T>
class Result
{
    union Pointer
    {
        T* value;
        Error* error;
    };

    enum class Tag
    {
        Value,
        Error
    };

    Tag mTag;
    Pointer mPointer;

public:
    Result(T value)
        : mTag(Tag::Value)
    {
        mPointer.value = new T(std::move(value));
    }

    Result(Error error)
        : mTag(Tag::Error)
    {
        mPointer.error = new Error(std::move(error));
    }

    Result(const Result& other)
        : mTag(other.mTag)
    {
        if (mTag == Tag::Value)
        {
            mPointer.value = new T(*other.mPointer.value);
        }
        else
        {
            mPointer.error = new Error(*other.mPointer.error);
        }
    }

    Result(Result&& other) noexcept
        : mTag(other.mTag)
    {
        if (mTag == Tag::Value)
        {
            mPointer.value = other.mPointer.value;
        }
        else
        {
            mPointer.error = other.mPointer.error;
        }

        other.mPointer.error = nullptr;
        other.mTag = Tag::Error;
    }

    ~Result()
    {
        if (mTag == Tag::Value)
        {
            delete mPointer.value;
        }
        else
        {
            delete mPointer.error;
        }
    }

    Result& operator=(const Result& other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (mTag == Tag::Value)
        {
            delete mPointer.value;
        }
        else
        {
            delete mPointer.error;
        }

        mTag = other.mTag;
        other.mTag = Tag::Error;

        if (mTag == Tag::Value)
        {
            mPointer.value = other.mPointer.value;
        }
        else
        {
            mPointer.error = other.mPointer.error;
        }

        other.mPointer.error = nullptr;
        return *this;
    }

    Result& operator=(Result&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        if (mTag == Tag::Value)
        {
            delete mPointer.value;
        }
        else
        {
            delete mPointer.error;
        }

        mTag = other.mTag;
        if (mTag == Tag::Value)
        {
            mPointer.value = other.mPointer.value;
            other.mPointer.value = nullptr;
        }
        else
        {
            mPointer.error = other.mPointer.error;
            other.mPointer.error = nullptr;
        }

        return *this;
    }

    bool has_value() const noexcept
    {
        return mTag == Tag::Value;
    }

    operator bool() const noexcept
    {
        return has_value();
    }

    T value_or(T default_value) const& noexcept(noexcept_copyable<T>())
    {
        if (mTag == Tag::Error)
        {
            return std::move(default_value);
        }

        return *mPointer.value;
    }

    T value_or(T default_value) && noexcept(noexcept_copyable<T>())
    {
        if (mTag == Tag::Error)
        {
            return std::move(default_value);
        }

        return std::move(*mPointer.value);
    }

    const T& value() const& noexcept(noexcept_copyable<T>())
    {
        return *mPointer.value;
    }

    T value() && noexcept(noexcept_movable<T>())
    {
        return std::move(*mPointer.value);
    }

    T& value() &
    {
        return *mPointer.value;
    }

    const T* operator->() const&
    {
        return mPointer.value;
    }

    T* operator->() &
    {
        return mPointer.value;
    }

    const Error& error() const&
    {
        return *mPointer.error;
    }

    Error error() &&
    {
        return std::move(*mPointer.error);
    }

    Error& error() &
    {
        return *mPointer.error;
    }
};
} // namespace compliance

#endif // COMPLIANCE_RESULT_H
