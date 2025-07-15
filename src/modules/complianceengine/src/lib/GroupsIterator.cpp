// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <GroupsIterator.h>
#include <cerrno>
#include <pwd.h>

namespace ComplianceEngine
{
GroupsIterator::GroupsIterator(const GroupsRange* range)
    : mRange(range),
      mBuffer(1024)
{
    memset(&mStorage, 0, sizeof(mStorage));
}

GroupsIterator::reference GroupsIterator::operator*()
{
    if (nullptr == mRange)
    {
        throw std::logic_error("Dereferencing end iterator");
    }
    return mStorage;
}
GroupsIterator::pointer GroupsIterator::operator->()
{
    if (nullptr == mRange)
    {
        throw std::logic_error("Dereferencing end iterator");
    }
    return &mStorage;
}

GroupsIterator& GroupsIterator::operator++()
{
    next();
    return *this;
}

GroupsIterator GroupsIterator::operator++(int)
{
    GroupsIterator tmp = *this;
    next();
    return tmp;
}

bool GroupsIterator::operator==(const GroupsIterator& other) const
{
    return mRange == other.mRange;
}

bool GroupsIterator::operator!=(const GroupsIterator& other) const
{
    return !(*this == other);
}

void GroupsIterator::next() // NOLINT(*-identifier-naming)
{
    if (nullptr == mRange)
    {
        throw std::logic_error("Dereferencing end iterator");
    }

    group* entry = nullptr;
    if (0 != fgetgrent_r(mRange->GetStream(), &mStorage, mBuffer.data(), mBuffer.size(), &entry))
    {
        auto status = errno;
        if (ENOENT == status)
        {
            OsConfigLogDebug(mRange->GetLogHandle(), "Reached end of group entries in stream");
            mRange = nullptr; // Reached the end of the stream
            return;
        }

        if (ERANGE == status)
        {
            OsConfigLogDebug(mRange->GetLogHandle(), "Buffer too small, resizing to %zu bytes", mBuffer.size() * 2);
            mBuffer.resize(mBuffer.size() * 2); // Resize buffer if it was too small
            return next();                      // Retry with the new buffer size
        }

        OsConfigLogError(mRange->GetLogHandle(), "Failed to read next group entry: %s", strerror(status));
        throw std::runtime_error("Failed to read next group entry: " + std::string(strerror(status)));
    }
}

GroupsRange::GroupsRange(FILE* stream, OsConfigLogHandle logHandle) noexcept
    : mStream(stream),
      mLog(logHandle)
{
}

Result<GroupsRange> GroupsRange::Make(OsConfigLogHandle logHandle)
{
    return Make("/etc/group", logHandle);
}

Result<GroupsRange> GroupsRange::Make(std::string path, OsConfigLogHandle logHandle)
{
    auto stream = fopen(path.c_str(), "r");
    if (nullptr == stream)
    {
        OsConfigLogError(logHandle, "Failed to open file '%s': %s", path.c_str(), strerror(errno));
        return Error("Failed to create GroupsRange: " + std::string(strerror(errno)), errno);
    }

    return GroupsRange(stream, logHandle);
}

GroupsRange::GroupsRange(GroupsRange&& other) noexcept
    : mStream(other.mStream),
      mLog(other.mLog)
{
    other.mStream = nullptr; // Transfer ownership
}

GroupsRange& GroupsRange::operator=(GroupsRange&& other) noexcept
{
    if (this != &other)
    {
        mStream = other.mStream;
        mLog = other.mLog;
        other.mStream = nullptr; // Transfer ownership
    }
    return *this;
}

GroupsRange::~GroupsRange()
{
    if (mStream != nullptr)
    {
        fclose(mStream);
    }
}

GroupsIterator GroupsRange::begin() // NOLINT(*-identifier-naming)
{
    auto result = GroupsIterator(this);
    result.next(); // Initialize the iterator to the first user
    return result;
}

GroupsIterator GroupsRange::end() // NOLINT(*-identifier-naming)
{
    return GroupsIterator(nullptr);
}

} // namespace ComplianceEngine
