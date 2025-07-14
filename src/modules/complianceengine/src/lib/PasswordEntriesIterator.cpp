// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Logging.h>
#include <PasswordEntriesIterator.h>
#include <cerrno>
#include <stdexcept>

namespace ComplianceEngine
{
using std::string;

PasswordEntryIterator::PasswordEntryIterator(const PasswordEntryRange* range)
    : mRange(range),
      mFgetspentBuffer(1024)
{
}

PasswordEntryIterator::PasswordEntryIterator(PasswordEntryIterator&& other) noexcept
    : mRange(other.mRange),
      mFgetspentEntry(other.mFgetspentEntry),
      mFgetspentBuffer(std::move(other.mFgetspentBuffer))
{
    other.mRange = nullptr; // Transfer ownership
}

PasswordEntryIterator& PasswordEntryIterator::operator=(PasswordEntryIterator&& other) noexcept
{
    if (this != &other)
    {
        mRange = other.mRange;
        mFgetspentEntry = other.mFgetspentEntry;
        mFgetspentBuffer = std::move(other.mFgetspentBuffer);
        other.mRange = nullptr; // Transfer ownership
    }
    return *this;
}

PasswordEntryIterator::reference PasswordEntryIterator::operator*()
{
    if (nullptr == mRange)
    {
        throw std::runtime_error("Attempted to dereference end iterator");
    }
    return mFgetspentEntry;
}

PasswordEntryIterator::pointer PasswordEntryIterator::operator->()
{
    if (nullptr == mRange)
    {
        throw std::runtime_error("Attempted to dereference end iterator");
    }
    return &mFgetspentEntry;
}

PasswordEntryIterator& PasswordEntryIterator::operator++()
{
    next();
    return *this;
}

PasswordEntryIterator PasswordEntryIterator::operator++(int)
{
    PasswordEntryIterator tmp = *this;
    next();
    return tmp;
}

bool PasswordEntryIterator::operator==(const PasswordEntryIterator& other) const
{
    return mRange == other.mRange;
}

bool PasswordEntryIterator::operator!=(const PasswordEntryIterator& other) const
{
    return !(*this == other);
}

void PasswordEntryIterator::next() // NOLINT(*-identifier-naming)
{
    if (nullptr == mRange)
    {
        throw std::runtime_error("Attempted to move past end iterator");
    }

    spwd* entry = nullptr;
    // fgetspent_r return 0 on success, -1 and sets errno on failure
    auto status = fgetspent_r(mRange->GetStream(), &mFgetspentEntry, mFgetspentBuffer.data(), mFgetspentBuffer.size(), &entry);
    if ((0 != status) || (nullptr == entry))
    {
        status = errno;
        if (ERANGE == status)
        {
            OsConfigLogInfo(mRange->GetLogHandle(), "Buffer size too small for /etc/shadow entry, resizing to %zu bytes", mFgetspentBuffer.size() * 2);
            mFgetspentBuffer.resize(mFgetspentBuffer.size() * 2);
            return next(); // Retry with a larger buffer
        }

        if (ENOENT == status)
        {
            OsConfigLogDebug(mRange->GetLogHandle(), "End of /etc/shadow file reached.");
            mRange = nullptr; // Mark as end iterator
            return;
        }

        OsConfigLogInfo(mRange->GetLogHandle(), "Failed to read /etc/shadow entry: %s (%d)", strerror(status), status);
        throw std::runtime_error("Failed to read /etc/shadow entry: " + string(strerror(status)) + ", errno: " + std::to_string(status));
    }
}

PasswordEntryRange::PasswordEntryRange(FILE* stream, OsConfigLogHandle log)
    : mLog(log),
      mStream(stream)
{
}

PasswordEntryRange::~PasswordEntryRange()
{
    if (nullptr != mStream)
    {
        fclose(mStream);
    }
}

PasswordEntryRange::PasswordEntryRange(PasswordEntryRange&& other) noexcept
    : mLog(other.mLog),
      mStream(other.mStream)
{
    other.mStream = nullptr; // Transfer ownership
}

PasswordEntryRange& PasswordEntryRange::operator=(PasswordEntryRange&& other) noexcept
{
    if (this != &other)
    {
        mLog = other.mLog;
        mStream = other.mStream;
        other.mStream = nullptr; // Transfer ownership
    }
    return *this;
}

Result<PasswordEntryRange> PasswordEntryRange::Create(OsConfigLogHandle log)
{
    return Create("/etc/shadow", log);
}

Result<PasswordEntryRange> PasswordEntryRange::Create(const std::string& path, OsConfigLogHandle log)
{
    OsConfigLogDebug(log, "Creating PasswordEntryRange for path: %s", path.c_str());
    auto stream = fopen(path.c_str(), "r");
    if (nullptr == stream)
    {
        return Error("Failed to open /etc/shadow file: " + string(strerror(errno)), errno);
    }

    return PasswordEntryRange(stream, log);
}

FILE* PasswordEntryRange::GetStream() const
{
    return mStream;
}

OsConfigLogHandle PasswordEntryRange::GetLogHandle() const
{
    return mLog;
}

PasswordEntryIterator PasswordEntryRange::begin() // NOLINT(*-identifier-naming)
{
    auto result = PasswordEntryIterator(this);
    result.next();
    return result;
}

PasswordEntryIterator PasswordEntryRange::end() // NOLINT(*-identifier-naming)
{
    return PasswordEntryIterator(nullptr);
}

} // namespace ComplianceEngine
