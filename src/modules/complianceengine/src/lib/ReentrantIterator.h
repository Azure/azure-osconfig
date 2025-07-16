// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_REENTRANT_ITERATOR_H
#define COMPLIANCE_REENTRANT_ITERATOR_H

#include <Logging.h>
#include <MmiResults.h>
#include <Result.h>
#include <vector>

namespace ComplianceEngine
{
template <typename EntryType, typename RangeType>
class ReentrantIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = EntryType;
    using pointer = EntryType*;
    using reference = EntryType&;

    ReentrantIterator(const RangeType* range, int (*getter)(FILE*, EntryType*, char*, size_t, EntryType**))
        : mRange(range),
          mBuffer(1024),
          mGetter(getter)
    {
        memset(&mStorage, 0, sizeof(mStorage));
    }
    ~ReentrantIterator() = default;
    ReentrantIterator() = delete;
    ReentrantIterator(const ReentrantIterator&) = default;
    ReentrantIterator(ReentrantIterator&&) noexcept = default;
    ReentrantIterator& operator=(const ReentrantIterator&) = default;
    ReentrantIterator& operator=(ReentrantIterator&&) noexcept = default;

    reference operator*()
    {
        if (nullptr == mRange)
        {
            throw std::logic_error("Dereferencing end iterator");
        }
        return mStorage;
    }

    pointer operator->()
    {
        if (nullptr == mRange)
        {
            throw std::logic_error("Dereferencing end iterator");
        }
        return &mStorage;
    }

    ReentrantIterator& operator++()
    {
        next();
        return *this;
    }

    ReentrantIterator operator++(int)
    {
        ReentrantIterator tmp = *this;
        next();
        return tmp;
    }

    bool operator==(const ReentrantIterator& other) const
    {
        return mRange == other.mRange;
    }

    bool operator!=(const ReentrantIterator& other) const
    {
        return !(*this == other);
    }

    void next() // NOLINT(*-identifier-naming)
    {
        if (nullptr == mRange)
        {
            throw std::logic_error("Dereferencing end iterator");
        }

        EntryType* entry = nullptr;
        if (0 != mGetter(mRange->GetStream(), &mStorage, mBuffer.data(), mBuffer.size(), &entry))
        {
            auto status = errno;
            if (ENOENT == status)
            {
                OsConfigLogDebug(mRange->GetLogHandle(), "Reached end of entries in the input stream");
                mRange = nullptr;                       // Reached the end of the stream
                memset(&mStorage, 0, sizeof(mStorage)); // Clear storage
                return;
            }

            if (ERANGE == status)
            {
                OsConfigLogDebug(mRange->GetLogHandle(), "Buffer too small, resizing to %zu bytes", mBuffer.size() * 2);
                mBuffer.resize(mBuffer.size() * 2); // Resize buffer if it was too small
                return next();                      // Retry with the new buffer size
            }

            OsConfigLogError(mRange->GetLogHandle(), "Failed to read next entry: %s", strerror(status));
            throw std::runtime_error("Failed to read next entry: " + std::string(strerror(status)));
        }
    }

private:
    EntryType mStorage;
    const RangeType* mRange;
    std::vector<char> mBuffer;
    int (*mGetter)(FILE*, EntryType*, char*, size_t, EntryType**);
};

template <typename IteratorType, typename DerivedType>
class ReentrantIteratorRange
{
    FILE* mStream;
    OsConfigLogHandle mLog;
    friend DerivedType;

private:
    ReentrantIteratorRange() = delete;
    ReentrantIteratorRange(FILE* stream, OsConfigLogHandle logHandle) noexcept
        : mStream(stream),
          mLog(logHandle)
    {
    }

    ReentrantIteratorRange(const ReentrantIteratorRange&) = delete;
    ReentrantIteratorRange(ReentrantIteratorRange&& other) noexcept
        : mStream(other.mStream),
          mLog(other.mLog)
    {
        other.mStream = nullptr;
        other.mLog = nullptr;
    }

    ReentrantIteratorRange& operator=(const ReentrantIteratorRange&) = delete;
    ReentrantIteratorRange& operator=(ReentrantIteratorRange&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        mStream = other.mStream;
        mLog = other.mLog;
        other.mStream = nullptr;
        other.mLog = nullptr;
        return *this;
    }

public:
    ~ReentrantIteratorRange()
    {
        if (mStream)
        {
            fclose(mStream);
        }
    }

    FILE* GetStream() const noexcept
    {
        return mStream;
    }

    OsConfigLogHandle GetLogHandle() const noexcept
    {
        return mLog;
    }

    IteratorType begin() // NOLINT(*-identifier-naming)
    {
        auto result = IteratorType(static_cast<const DerivedType*>(this));
        result.next(); // Initialize the iterator
        return result;
    }

    IteratorType end() // NOLINT(*-identifier-naming)
    {
        return IteratorType(nullptr); // Return an end iterator
    }
};
} // namespace ComplianceEngine

#endif // COMPLIANCE_REENTRANT_ITERATOR_H
