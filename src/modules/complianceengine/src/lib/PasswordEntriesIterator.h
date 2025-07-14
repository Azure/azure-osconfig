// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_PASSWORD_ENTRIES_ITERATOR_H
#define COMPLIANCE_PASSWORD_ENTRIES_ITERATOR_H

#include <ContextInterface.h>
#include <Logging.h>
#include <shadow.h>
#include <vector>

namespace ComplianceEngine
{
class PasswordEntryRange;
// Iterator for iterating over password entries for system users (/etc/shadow).
class PasswordEntryIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = spwd;
    using pointer = spwd*;
    using reference = spwd&;
    explicit PasswordEntryIterator(const PasswordEntryRange* range);
    ~PasswordEntryIterator() = default;
    PasswordEntryIterator(const PasswordEntryIterator&) = default;
    PasswordEntryIterator(PasswordEntryIterator&&) noexcept noexcept;
    PasswordEntryIterator& operator=(const PasswordEntryIterator&) = default;
    PasswordEntryIterator& operator=(PasswordEntryIterator&&) noexcept noexcept;
    reference operator*();
    pointer operator->();

    PasswordEntryIterator& operator++();
    PasswordEntryIterator operator++(int);
    bool operator==(const PasswordEntryIterator& other) const;
    bool operator!=(const PasswordEntryIterator& other) const;

    void next(); // NOLINT(*-identifier-naming)

private:
    const PasswordEntryRange* mRange;
    spwd mFgetspentEntry;
    std::vector<char> mFgetspentBuffer;
};

class PasswordEntryRange
{
    OsConfigLogHandle mLog;
    FILE* mStream;

    PasswordEntryRange(FILE* stream, OsConfigLogHandle log);

public:
    ~PasswordEntryRange();
    PasswordEntryRange(const PasswordEntryRange&) = delete;
    PasswordEntryRange& operator=(const PasswordEntryRange&) = delete;
    PasswordEntryRange(PasswordEntryRange&&) noexcept noexcept;
    PasswordEntryRange& operator=(PasswordEntryRange&&) noexcept noexcept;

    static Result<PasswordEntryRange> Create(OsConfigLogHandle log = nullptr);
    static Result<PasswordEntryRange> Create(const std::string& path, OsConfigLogHandle log = nullptr);
    FILE* GetStream() const;
    OsConfigLogHandle GetLogHandle() const;
    PasswordEntryIterator begin(); // NOLINT(*-identifier-naming)
    PasswordEntryIterator end();   // NOLINT(*-identifier-naming)
};
} // namespace ComplianceEngine

#endif // COMPLIANCE_PASSWORD_ENTRIES_ITERATOR_H
