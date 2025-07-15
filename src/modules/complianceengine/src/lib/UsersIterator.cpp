// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <UsersIterator.h>
#include <cerrno>
#include <pwd.h>

namespace ComplianceEngine
{
UsersIterator::UsersIterator(const UsersRange* range)
    : mRange(range),
      mBuffer(1024)
{
    memset(&mStorage, 0, sizeof(mStorage));
}

UsersIterator::reference UsersIterator::operator*()
{
    if (nullptr == mRange)
    {
        throw std::logic_error("Dereferencing end iterator");
    }
    return mStorage;
}
UsersIterator::pointer UsersIterator::operator->()
{
    if (nullptr == mRange)
    {
        throw std::logic_error("Dereferencing end iterator");
    }
    return &mStorage;
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
    return mRange == other.mRange;
}

bool UsersIterator::operator!=(const UsersIterator& other) const
{
    return !(*this == other);
}

void UsersIterator::next() // NOLINT(*-identifier-naming)
{
    if (nullptr == mRange)
    {
        throw std::logic_error("Dereferencing end iterator");
    }

    passwd* entry = nullptr;
    if (0 != fgetpwent_r(mRange->GetStream(), &mStorage, mBuffer.data(), mBuffer.size(), &entry))
    {
        auto status = errno;
        if (ENOENT == status)
        {
            OsConfigLogDebug(mRange->GetLogHandle(), "Reached end of user entries in stream");
            mRange = nullptr; // Reached the end of the stream
            return;
        }

        if (ERANGE == status)
        {
            OsConfigLogDebug(mRange->GetLogHandle(), "Buffer too small, resizing to %zu bytes", mBuffer.size() * 2);
            mBuffer.resize(mBuffer.size() * 2); // Resize buffer if it was too small
            return next();                      // Retry with the new buffer size
        }

        OsConfigLogError(mRange->GetLogHandle(), "Failed to read next user entry: %s", strerror(status));
        throw std::runtime_error("Failed to read next user entry: " + std::string(strerror(status)));
    }
}

UsersRange::UsersRange(FILE* stream, OsConfigLogHandle logHandle) noexcept
    : mStream(stream),
      mLog(logHandle)
{
}

Result<UsersRange> UsersRange::Make(OsConfigLogHandle logHandle)
{
    return Make("/etc/passwd", logHandle);
}

Result<UsersRange> UsersRange::Make(std::string path, OsConfigLogHandle logHandle)
{
    auto stream = fopen(path.c_str(), "r");
    if (nullptr == stream)
    {
        OsConfigLogError(logHandle, "Failed to open file '%s': %s", path.c_str(), strerror(errno));
        return Error("Failed to create UsersRange: " + std::string(strerror(errno)), errno);
    }

    return UsersRange(stream, logHandle);
}

UsersRange::UsersRange(UsersRange&& other) noexcept
    : mStream(other.mStream),
      mLog(other.mLog)
{
    other.mStream = nullptr; // Transfer ownership
}

UsersRange& UsersRange::operator=(UsersRange&& other) noexcept
{
    if (this != &other)
    {
        mStream = other.mStream;
        mLog = other.mLog;
        other.mStream = nullptr; // Transfer ownership
    }
    return *this;
}

UsersRange::~UsersRange()
{
    if (mStream != nullptr)
    {
        fclose(mStream);
    }
}

UsersIterator UsersRange::begin() // NOLINT(*-identifier-naming)
{
    auto result = UsersIterator(this);
    result.next(); // Initialize the iterator to the first user
    return result;
}

UsersIterator UsersRange::end() // NOLINT(*-identifier-naming)
{
    return UsersIterator(nullptr);
}

} // namespace ComplianceEngine
