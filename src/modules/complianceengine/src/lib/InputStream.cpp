// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <InputStream.h>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>

namespace ComplianceEngine
{
using std::ifstream;
using std::string;

InputStream::InputStream(string fileName, ContextInterface& context)
    : mContext(context),
      mFileName(std::move(fileName))
{
    assert(!mStream.is_open());
}

InputStream::InputStream(InputStream&& other) noexcept
    : mContext(other.mContext),
      mFileName(std::move(other.mFileName)),
      mStream(std::move(other.mStream))
{
    assert(mStream.is_open());
}

InputStream& InputStream::operator=(InputStream&& other) noexcept
{
    if (this == &other)
        return *this;

    // The context reference must be constant for moves to work and it's global anyway
    assert(&mContext == &other.mContext);
    mFileName = std::move(other.mFileName);
    mStream = std::move(other.mStream);
    assert(mStream.is_open());
    return *this;
}

Result<InputStream> InputStream::Open(const string& fileName, ContextInterface& context)
{
    // access lets us determine readability and obtain error codes
    if (0 != ::access(fileName.c_str(), R_OK))
    {
        const auto status = errno;
        OsConfigLogInfo(context.GetLogHandle(), "Failed to access '%s': %s (%d)", fileName.c_str(), strerror(status), status);
        return Error(string("failed to access '") + fileName + "': " + strerror(status), status);
    }

    InputStream result(fileName, context);
    result.mStream.open(context.GetSpecialFilePath(fileName));
    if (!result.mStream.is_open())
    {
        OsConfigLogInfo(context.GetLogHandle(), "Failed to open '%s'", result.mFileName.c_str());
        return Error(string("failed to open '") + result.mFileName + "'");
    }

    assert(result.mStream.is_open());
    return result;
}

Result<string> InputStream::ReadLine()
{
    assert(mStream.is_open());
    if (mBytesRead >= cMaxReadSize)
    {
        OsConfigLogError(mContext.GetLogHandle(), "Maximum file '%s' read size reached", mFileName.c_str());
        return Error("maximum file '" + mFileName + "' read size reached", E2BIG);
    }

    // We want to always check with Good() before reading
    if (mStream.eof())
    {
        OsConfigLogError(mContext.GetLogHandle(), "Attempted to read file '%s' after EOF", mFileName.c_str());
        return Error(string("attempted to read file '") + mFileName + "' after EOF", EBADFD);
    }

    // We won't return empty strings in case an error happened previously
    if (mStream.fail() || mStream.bad())
    {
        OsConfigLogError(mContext.GetLogHandle(), "Attempted to read file '%s' after failure", mFileName.c_str());
        return Error(string("attempted to read file '") + mFileName + "' after failure", EBADFD);
    }
    // eof(), fail(), bad() and limits are already checked
    assert(Good());

    // Read the data
    string line;
    std::getline(mStream, line);

    // Include the line size without the newline character
    mBytesRead += line.size();
    if (!mStream.eof())
    {
        // Here we know a newline has been parsed.
        ++mBytesRead;
    }

    if (mStream.bad())
    {
        // fail() may return true here in case there's no trailing newline character, so we stick to bad().
        OsConfigLogError(mContext.GetLogHandle(), "Failed to read line from '%s': %d", mFileName.c_str(), static_cast<int>(mStream.rdstate()));
        return Error("failed to read line from '" + mFileName + "'", EBADFD);
    }

    return Result<string>(std::move(line));
}

bool InputStream::Good() const
{
    assert(mStream.is_open());
    return mBytesRead < cMaxReadSize && mStream.good();
}

bool InputStream::AtEnd() const
{
    assert(mStream.is_open());
    return mStream.eof();
}

const string& InputStream::GetFileName() const
{
    assert(mStream.is_open());
    return mFileName;
}

InputStreamIterators::LinesRange InputStream::Lines() &
{
    return InputStreamIterators::LinesRange(*this);
}

size_t InputStream::BytesRead() const
{
    return mBytesRead;
}

namespace InputStreamIterators
{
LinesIterator::LinesIterator(InputStream& stream)
    : mStream(stream)
{
}

LinesIterator::LinesIterator(const LinesIterator& other)
    : mStream(other.mStream),
      mValue(other.mValue)
{
}

LinesIterator::LinesIterator(LinesIterator&& other) noexcept
    : mStream(other.mStream),
      mValue(std::move(other.mValue))
{
}

LinesIterator& LinesIterator::operator=(const LinesIterator& other)
{
    if (this == &other)
        return *this;

    assert(&mStream == &other.mStream);
    mValue = other.mValue;
    return *this;
}

LinesIterator& LinesIterator::operator=(LinesIterator&& other) noexcept
{
    if (this == &other)
        return *this;

    assert(&mStream == &other.mStream);
    mValue = std::move(other.mValue);
    return *this;
}

LinesIterator& LinesIterator::operator++()
{
    if (mStream.Good())
        mValue = mStream.ReadLine();
    else
        mValue.Reset();
    return *this;
}

LinesIterator LinesIterator::operator++(int)
{
    LinesIterator tmp = *this;
    ++*this;
    return tmp;
}

Result<std::string> LinesIterator::operator*() const& noexcept(false)
{
    CheckValue();
    return mValue.Value();
}

Result<std::string> LinesIterator::operator*() && noexcept(false)
{
    CheckValue();
    return std::move(mValue.Value());
}

const Result<std::string>* LinesIterator::operator->() const noexcept(false)
{
    CheckValue();
    return &mValue.Value();
}

bool LinesIterator::IsEnd() const
{
    return mValue.HasValue();
}

void LinesIterator::CheckValue() const noexcept(false)
{
    if (!IsEnd())
    {
        throw std::logic_error("LinesIterator: unchecked access to Value");
    }
}

// The comparison makes only sense to compare with the end() iterator
bool LinesIterator::operator==(const LinesIterator& other) const
{
    return mValue.HasValue() == other.mValue.HasValue();
}

bool LinesIterator::operator!=(const LinesIterator& other) const
{
    return !(*this == other);
}

LinesRange::LinesRange(InputStream& stream)
    : mStream(stream)
{
}

LinesRange::LinesRange(const LinesRange& other)
    : mStream(other.mStream)
{
}

LinesRange::LinesRange(LinesRange&& other) noexcept
    : mStream(other.mStream)
{
}

LinesRange& LinesRange::operator=(const LinesRange& other)
{
    assert(&mStream == &other.mStream);
    return *this;
}

LinesRange& LinesRange::operator=(LinesRange&& other) noexcept
{
    assert(&mStream == &other.mStream);
    return *this;
}

LinesIterator LinesRange::begin() const
{
    return ++LinesIterator(mStream);
}

LinesIterator LinesRange::end() const
{
    return LinesIterator(mStream);
}
} // namespace InputStreamIterators
} // namespace ComplianceEngine
