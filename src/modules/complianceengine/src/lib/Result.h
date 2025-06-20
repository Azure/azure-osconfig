// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_RESULT_H
#define COMPLIANCEENGINE_RESULT_H

#include "TypeTraits.h"

#include <stdexcept>
#include <string>

namespace ComplianceEngine
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
std::ostream& operator<<(std::ostream& os, const ComplianceEngine::Error& error);

template <typename T>
class Result
{
    union Pointer
    {
        T* value;
        ComplianceEngine::Error* error;
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

    Result(ComplianceEngine::Error error)
        : mTag(Tag::Error)
    {
        mPointer.error = new ComplianceEngine::Error(std::move(error));
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
            mPointer.error = new ComplianceEngine::Error(*other.mPointer.error);
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

    bool HasValue() const noexcept
    {
        return mTag == Tag::Value;
    }

    operator bool() const noexcept
    {
        return HasValue();
    }

    T ValueOr(T default_value) const& noexcept(NoexceptCopyable<T>())
    {
        if (mTag == Tag::Error)
        {
            return std::move(default_value);
        }

        return *mPointer.value;
    }

    T ValueOr(T default_value) && noexcept(NoexceptCopyable<T>())
    {
        if (mTag == Tag::Error)
        {
            return std::move(default_value);
        }

        return std::move(*mPointer.value);
    }

    const T& Value() const&
    {
        CheckValue();
        return *mPointer.value;
    }

    T Value() &&
    {
        CheckValue();
        return std::move(*mPointer.value);
    }

    T& Value() &
    {
        CheckValue();
        return *mPointer.value;
    }

    const T* operator->() const&
    {
        CheckValue();
        return mPointer.value;
    }

    T* operator->() &
    {
        CheckValue();
        return mPointer.value;
    }

    const ComplianceEngine::Error& Error() const&
    {
        CheckError();
        return *mPointer.error;
    }

    ComplianceEngine::Error Error() &&
    {
        CheckError();
        return std::move(*mPointer.error);
    }

    ComplianceEngine::Error& Error() &
    {
        CheckError();
        return *mPointer.error;
    }

private:
    void CheckError() const
    {
        if (mTag != Tag::Error)
        {
            throw std::logic_error("Result: unchecked access to Error");
        }
    }
    void CheckValue() const
    {
        if (mTag != Tag::Value)
        {
            throw std::logic_error("Result: unchecked access to Value");
        }
    }
};
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_RESULT_H
