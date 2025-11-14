// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_FILE_STREAM_H
#define COMPLIANCEENGINE_FILE_STREAM_H

#include <ContextInterface.h>
#include <Result.h>
#include <cassert>
#include <fstream>

namespace ComplianceEngine
{
namespace InputStreamIterators
{
class LinesRange;
} // namespace InputStreamIterators

// This class wraps the C++ std::ifstream with
// CE-specific error handling and a read size limit.
//
// 1. It uses factory method instead of constructors, to provide error handling with Result.
// 2. It guarantees an instance is always assosciated with an open file.
// 3. Any previous errors cause subsequent reads to fail
// 4. Context is used to provide mocking mechanism - filenames can be overridden for testing.
// 5. Maximum number of bytes to read is limited to cMaxReadSize. This is a soft limit as we allow
//    to read last full line using std::getline.
class InputStream
{
public:
    // Maximum number of bytes read from an input stream
    static constexpr std::size_t cMaxReadSize = 1024 * 1024 * 128;

    InputStream(const InputStream&) = delete;
    InputStream(InputStream&&) noexcept;
    InputStream& operator=(const InputStream&) = delete;
    InputStream& operator=(InputStream&&) noexcept;
    ~InputStream() = default;

    // Opens a file for reading.
    static Result<InputStream> Open(const std::string& fileName, ContextInterface& context);

    // Reads a single line.
    Result<std::string> ReadLine();

    // Returns true in case more bytes can be read from the file.
    // This means there was no error returned so far,
    // we have not reached end of the file, and we have not reached
    // the read size limit.
    bool Good() const;

    // Returns true in case we have reached end of the file.
    bool AtEnd() const;

    // Returns the file name passed to Open.
    // Note: In case of mocking, the underlying filename is not stored.
    const std::string& GetFileName() const;

    // Obtains a range for line-by-line range-based iteration.
    InputStreamIterators::LinesRange Lines() &;

    // Obtains the number of bytes read so far.
    std::size_t BytesRead() const;

private:
    InputStream() = delete;

    explicit InputStream(std::string fileName, ContextInterface& context);

    // Holds logger handle and allows mocking.
    ContextInterface& mContext;

    // The file name passed to Open.
    std::string mFileName;

    // The underlying stream.
    std::ifstream mStream;

    // The number of bytes read so far.
    std::size_t mBytesRead = 0;
};

namespace InputStreamIterators
{
// This class allows to iterate a file line by line.
class LinesIterator
{
public:
    LinesIterator(const LinesIterator& other);
    LinesIterator(LinesIterator&& other) noexcept;
    LinesIterator& operator=(const LinesIterator& other);
    LinesIterator& operator=(LinesIterator&& other) noexcept;
    ~LinesIterator() = default;

    LinesIterator& operator++();
    LinesIterator operator++(int);
    Result<std::string> operator*() const& noexcept(false);
    Result<std::string> operator*() && noexcept(false);
    const Result<std::string>* operator->() const noexcept(false);
    bool operator==(const LinesIterator& other) const;
    bool operator!=(const LinesIterator& other) const;

    // Returns true in case this is the end iterator.
    bool IsEnd() const;

private:
    friend class LinesRange;

    InputStream& mStream;
    // End iterator has nullopt assigned
    Optional<Result<std::string>> mValue = Optional<Result<std::string>>();

    // Only range class(es) are able to construct new iterators
    explicit LinesIterator(InputStream& stream);

    // Throws in case of end iterator dereference.
    void CheckValue() const noexcept(false);
};

// This class provides an interface for the range-based for loops.
class LinesRange
{
public:
    explicit LinesRange(InputStream& stream);
    LinesRange(const LinesRange& other);
    LinesRange(LinesRange&& other) noexcept;
    LinesRange& operator=(const LinesRange& other);
    LinesRange& operator=(LinesRange&& other) noexcept;
    ~LinesRange() = default;

    LinesIterator begin() const; // NOLINT(*-identifier-naming)
    LinesIterator end() const;   // NOLINT(*-identifier-naming)

private:
    InputStream& mStream;
};
} // namespace InputStreamIterators
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_FILE_STREAM_H
