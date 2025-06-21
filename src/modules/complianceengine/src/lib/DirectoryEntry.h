// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_DIRECTORYENTRY_H
#define COMPLIANCEENGINE_DIRECTORYENTRY_H

#include <fts.h>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace ComplianceEngine
{

enum class DirectoryEntryType
{
    RegularFile,
    Directory,
    SymbolicLink,
    Other
};

struct DirectoryEntry
{
    std::string path;
    DirectoryEntryType type;

    DirectoryEntry(const std::string& p, DirectoryEntryType t)
        : path(p),
          type(t)
    {
    }
};

// Forward declarations
class DirectoryEntries;

// Interface for directory iteration - enables mocking
class DirectoryIteratorInterface
{
public:
    virtual ~DirectoryIteratorInterface() = default;
    virtual DirectoryEntries GetEntries(const std::string& directoryPath, bool recursive) const = 0;
};

// Streaming iterator that advances FTS structure lazily
class DirectoryIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = DirectoryEntry;
    using difference_type = std::ptrdiff_t;
    using pointer = const DirectoryEntry*;
    using reference = const DirectoryEntry&;

    // Constructors for different backends
    DirectoryIterator();                                               // End iterator
    DirectoryIterator(FTS* fts, bool recursive);                       // FTS-based streaming iterator
    DirectoryIterator(std::vector<DirectoryEntry>::const_iterator it); // Vector-based iterator for testing

    ~DirectoryIterator();

    // Copy constructor and assignment
    DirectoryIterator(const DirectoryIterator& other);
    DirectoryIterator& operator=(const DirectoryIterator& other);

    // Iterator operations
    DirectoryIterator& operator++();
    DirectoryIterator operator++(int);
    const DirectoryEntry& operator*() const;
    const DirectoryEntry* operator->() const;
    bool operator==(const DirectoryIterator& other) const;
    bool operator!=(const DirectoryIterator& other) const;

private:
    enum BackendType
    {
        FTS_BACKEND,
        VECTOR_BACKEND,
        END_ITERATOR
    };

    BackendType mBackendType;

    // FTS backend data
    FTS* mFts;
    FTSENT* mCurrentEntry;
    bool mRecursive;
    DirectoryEntry mCurrentDirectoryEntry;

    // Vector backend data
    std::vector<DirectoryEntry>::const_iterator mVectorIt;

    void AdvanceFts();
    bool ShouldSkipEntry(FTSENT* entry) const;
    DirectoryEntryType GetEntryType(int fts_info) const;
    void UpdateCurrentEntryFromFts();
};

// Container class that provides begin/end functionality for range-based loops
class DirectoryEntries
{
public:
    using iterator = DirectoryIterator;
    using const_iterator = DirectoryIterator;

    // Constructor for FTS-based streaming iteration (production)
    DirectoryEntries(FTS* fts, bool recursive);

    // Constructor for vector-based iteration (testing)
    explicit DirectoryEntries(std::vector<DirectoryEntry> entries);

    ~DirectoryEntries();

    // Non-copyable to avoid FTS pointer issues, but movable
    DirectoryEntries(const DirectoryEntries&) = delete;
    DirectoryEntries& operator=(const DirectoryEntries&) = delete;

    // Move constructor and assignment
    DirectoryEntries(DirectoryEntries&& other) noexcept;
    DirectoryEntries& operator=(DirectoryEntries&& other) noexcept;

    iterator Begin();
    iterator End();
    size_t Size() const;
    bool Empty() const;

private:
    enum BackendType
    {
        FTS_BACKEND,
        VECTOR_BACKEND
    };

    BackendType mBackendType;

    // FTS backend data
    FTS* mFts;
    bool mRecursive;
    bool mOwnsHandle;

    // Vector backend data
    std::vector<DirectoryEntry> mEntries;
};

// Concrete implementation using fts
class FtsDirectoryIterator : public DirectoryIteratorInterface
{
public:
    DirectoryEntries GetEntries(const std::string& directoryPath, bool recursive) const override;

private:
    DirectoryEntryType GetEntryType(int fts_info) const;
};

} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_DIRECTORYENTRY_H
