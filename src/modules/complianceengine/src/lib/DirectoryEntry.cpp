// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DirectoryEntry.h"

#include <cerrno>
#include <cstring>
#include <fts.h>

namespace ComplianceEngine
{

// DirectoryIterator implementation
DirectoryIterator::DirectoryIterator()
    : mBackendType(END_ITERATOR),
      mFts(nullptr),
      mCurrentEntry(nullptr),
      mRecursive(false),
      mCurrentDirectoryEntry("", DirectoryEntryType::Other)
{
}

DirectoryIterator::DirectoryIterator(FTS* fts, bool recursive)
    : mBackendType(FTS_BACKEND),
      mFts(fts),
      mCurrentEntry(nullptr),
      mRecursive(recursive),
      mCurrentDirectoryEntry("", DirectoryEntryType::Other)
{
    if (mFts)
    {
        advanceFts(); // Get the first valid entry
    }
    else
    {
        mBackendType = END_ITERATOR;
    }
}

DirectoryIterator::DirectoryIterator(std::vector<DirectoryEntry>::const_iterator it)
    : mBackendType(VECTOR_BACKEND),
      mFts(nullptr),
      mCurrentEntry(nullptr),
      mRecursive(false),
      mCurrentDirectoryEntry("", DirectoryEntryType::Other),
      mVectorIt(it)
{
}

DirectoryIterator::~DirectoryIterator()
{
    // Don't close FTS here - DirectoryEntries owns it
}

DirectoryIterator::DirectoryIterator(const DirectoryIterator& other)
    : mBackendType(other.mBackendType),
      mFts(other.mFts),
      mCurrentEntry(other.mCurrentEntry),
      mRecursive(other.mRecursive),
      mCurrentDirectoryEntry(other.mCurrentDirectoryEntry),
      mVectorIt(other.mVectorIt)
{
}

DirectoryIterator& DirectoryIterator::operator=(const DirectoryIterator& other)
{
    if (this != &other)
    {
        mBackendType = other.mBackendType;
        mFts = other.mFts;
        mCurrentEntry = other.mCurrentEntry;
        mRecursive = other.mRecursive;
        mCurrentDirectoryEntry = other.mCurrentDirectoryEntry;
        mVectorIt = other.mVectorIt;
    }
    return *this;
}

DirectoryIterator& DirectoryIterator::operator++()
{
    switch (mBackendType)
    {
        case FTS_BACKEND:
            advanceFts();
            break;
        case VECTOR_BACKEND:
            ++mVectorIt;
            break;
        case END_ITERATOR:
            // No-op
            break;
    }
    return *this;
}

DirectoryIterator DirectoryIterator::operator++(int)
{
    DirectoryIterator temp(*this);
    operator++();
    return temp;
}

const DirectoryEntry& DirectoryIterator::operator*() const
{
    switch (mBackendType)
    {
        case FTS_BACKEND:
            return mCurrentDirectoryEntry;
        case VECTOR_BACKEND:
            return *mVectorIt;
        case END_ITERATOR:
        default:
            return mCurrentDirectoryEntry; // Should not happen, but avoid crash
    }
}

const DirectoryEntry* DirectoryIterator::operator->() const
{
    return &(operator*());
}

bool DirectoryIterator::operator==(const DirectoryIterator& other) const
{
    // Different backend types are not equal unless both are end iterators
    if (mBackendType != other.mBackendType)
    {
        return mBackendType == END_ITERATOR && other.mBackendType == END_ITERATOR;
    }

    switch (mBackendType)
    {
        case FTS_BACKEND:
            return mFts == other.mFts && mCurrentEntry == other.mCurrentEntry;
        case VECTOR_BACKEND:
            return mVectorIt == other.mVectorIt;
        case END_ITERATOR:
            return true;
        default:
            return false;
    }
}

bool DirectoryIterator::operator!=(const DirectoryIterator& other) const
{
    return !(*this == other);
}

void DirectoryIterator::advanceFts()
{
    if (!mFts || mBackendType != FTS_BACKEND)
    {
        mBackendType = END_ITERATOR;
        return;
    }

    // Stream through FTS entries one at a time
    while ((mCurrentEntry = fts_read(mFts)) != nullptr)
    {
        // Skip entries we don't want to include
        if (shouldSkipEntry(mCurrentEntry))
        {
            continue;
        }

        // Found a valid entry - update our current entry and return
        updateCurrentEntryFromFts();
        return;
    }

    // No more entries - we've reached the end
    mBackendType = END_ITERATOR;
    mCurrentEntry = nullptr;
}

bool DirectoryIterator::shouldSkipEntry(FTSENT* entry) const
{
    if (!entry)
        return true;

    // Skip the root directory itself (level 0)
    if (entry->fts_level == 0)
    {
        return true;
    }

    // For non-recursive mode, only include immediate children (level 1)
    if (!mRecursive && entry->fts_level > 1)
    {
        // Tell FTS to skip this entire subtree for efficiency
        fts_set(mFts, entry, FTS_SKIP);
        return true;
    }

    // Handle directory entries in non-recursive mode
    if (!mRecursive && entry->fts_info == FTS_D && entry->fts_level == 1)
    {
        // We want to include the directory entry itself, but not descend into it
        fts_set(mFts, entry, FTS_SKIP);
        return false; // Don't skip - include the directory entry
    }

    // Skip post-order directory visits (FTS_DP) - we only want pre-order
    if (entry->fts_info == FTS_DP)
    {
        return true;
    }

    // Skip error entries and unreadable entries
    if (entry->fts_info == FTS_ERR || entry->fts_info == FTS_NS || entry->fts_info == FTS_DNR)
    {
        return true;
    }

    return false;
}

DirectoryEntryType DirectoryIterator::getEntryType(int fts_info) const
{
    switch (fts_info)
    {
        case FTS_F:
            return DirectoryEntryType::RegularFile;
        case FTS_D:
            return DirectoryEntryType::Directory;
        case FTS_SL:
        case FTS_SLNONE:
            return DirectoryEntryType::SymbolicLink;
        default:
            return DirectoryEntryType::Other;
    }
}

void DirectoryIterator::updateCurrentEntryFromFts()
{
    if (mCurrentEntry)
    {
        DirectoryEntryType type = getEntryType(mCurrentEntry->fts_info);
        mCurrentDirectoryEntry = DirectoryEntry(mCurrentEntry->fts_path, type);
    }
}

// DirectoryEntries implementation
DirectoryEntries::DirectoryEntries(FTS* fts, bool recursive)
    : mBackendType(FTS_BACKEND),
      mFts(fts),
      mRecursive(recursive),
      mOwnsHandle(true)
{
    // If FTS is null, we'll return empty iterators
    if (!mFts)
    {
        mOwnsHandle = false;
    }
}

DirectoryEntries::DirectoryEntries(std::vector<DirectoryEntry> entries)
    : mBackendType(VECTOR_BACKEND),
      mFts(nullptr),
      mRecursive(false),
      mOwnsHandle(false),
      mEntries(std::move(entries))
{
}

DirectoryEntries::~DirectoryEntries()
{
    if (mOwnsHandle && mFts)
    {
        fts_close(mFts);
        mFts = nullptr;
    }
}

DirectoryEntries::DirectoryEntries(DirectoryEntries&& other)
    : mBackendType(other.mBackendType),
      mFts(other.mFts),
      mRecursive(other.mRecursive),
      mOwnsHandle(other.mOwnsHandle),
      mEntries(std::move(other.mEntries))
{
    other.mFts = nullptr;
    other.mOwnsHandle = false;
}

DirectoryEntries& DirectoryEntries::operator=(DirectoryEntries&& other)
{
    if (this != &other)
    {
        if (mOwnsHandle && mFts)
        {
            fts_close(mFts);
        }

        mBackendType = other.mBackendType;
        mFts = other.mFts;
        mRecursive = other.mRecursive;
        mOwnsHandle = other.mOwnsHandle;
        mEntries = std::move(other.mEntries);

        other.mFts = nullptr;
        other.mOwnsHandle = false;
    }
    return *this;
}

DirectoryIterator DirectoryEntries::begin()
{
    switch (mBackendType)
    {
        case FTS_BACKEND:
            if (!mFts)
            {
                return DirectoryIterator(); // End iterator
            }
            return DirectoryIterator(mFts, mRecursive);
        case VECTOR_BACKEND:
            return DirectoryIterator(mEntries.begin());
        default:
            return DirectoryIterator(); // End iterator
    }
}

DirectoryIterator DirectoryEntries::end()
{
    switch (mBackendType)
    {
        case FTS_BACKEND:
            return DirectoryIterator(); // End iterator
        case VECTOR_BACKEND:
            return DirectoryIterator(mEntries.end());
        default:
            return DirectoryIterator(); // End iterator
    }
}

size_t DirectoryEntries::size() const
{
    switch (mBackendType)
    {
        case FTS_BACKEND:
            // For streaming FTS backend, size is not known until full iteration
            // This is a fundamental limitation of lazy streaming
            // To get size, caller would need to iterate through all entries
            return 0; // Indicates unknown size
        case VECTOR_BACKEND:
            return mEntries.size();
        default:
            return 0;
    }
}

bool DirectoryEntries::empty() const
{
    switch (mBackendType)
    {
        case FTS_BACKEND:
            // For FTS backend, check if we have a valid FTS handle
            // This is a best-effort check - true emptiness requires iteration
            return !mFts;
        case VECTOR_BACKEND:
            return mEntries.empty();
        default:
            return true;
    }
}

// FtsDirectoryIterator implementation
DirectoryEntries FtsDirectoryIterator::GetEntries(const std::string& directoryPath, bool recursive) const
{
    // Prepare paths for fts_open
    char* paths[] = {const_cast<char*>(directoryPath.c_str()), nullptr};

    // Configure FTS options for optimal streaming performance
    int options = FTS_PHYSICAL | FTS_NOCHDIR;

    // Use physical traversal for better performance and to avoid symbolic link loops
    // FTS_NOCHDIR prevents fts from changing the working directory

    FTS* fts = fts_open(paths, options, nullptr);
    if (!fts)
    {
        // Return empty container if fts_open fails
        return DirectoryEntries(static_cast<FTS*>(nullptr), recursive);
    }

    // DirectoryEntries takes ownership of the FTS handle and will close it in destructor
    return DirectoryEntries(fts, recursive);
}

DirectoryEntryType FtsDirectoryIterator::GetEntryType(int fts_info) const
{
    switch (fts_info)
    {
        case FTS_F:
            return DirectoryEntryType::RegularFile;
        case FTS_D:
            return DirectoryEntryType::Directory;
        case FTS_SL:
        case FTS_SLNONE:
            return DirectoryEntryType::SymbolicLink;
        default:
            return DirectoryEntryType::Other;
    }
}

} // namespace ComplianceEngine
