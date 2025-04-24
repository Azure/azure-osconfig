// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_REGEX_FALLBACK_H
#define COMPLIANCE_REGEX_FALLBACK_H

#if USE_REGEX_FALLBACK == 0
#error "USE_REGEX_FALLBACK should be set to 1 here"
#endif

#include <cassert>
#include <memory>
#include <regex.h>
#include <regex> // for std::regex_constants
#include <string>

namespace RegexLibcWrapper
{
class Regex
{
    int ConvertFlags(std::regex_constants::syntax_option_type options)
    {
        int flags = 0;
        if (options & std::regex_constants::icase)
        {
            flags |= REG_ICASE;
        }
        if (options & std::regex_constants::extended)
        {
            flags |= REG_EXTENDED;
        }
        return flags;
    }

public:
    Regex() = default;
    Regex(const Regex&) = delete;
    Regex(Regex&&) noexcept = default;
    Regex& operator=(Regex&&) noexcept = default;
    Regex(const std::string& r, std::regex_constants::syntax_option_type options = std::regex_constants::extended)
    {
        int v = regcomp(&preg, r.c_str(), ConvertFlags(options));
        if (0 != v)
        {
            throw std::runtime_error("Invalid regex");
        }
    }
    ~Regex()
    {
        regfree(&preg);
    }
    regex_t preg;
};

class SubMatch
{
public:
    SubMatch() = delete;
    SubMatch(const SubMatch&) = default;
    SubMatch& operator=(const SubMatch&) = default;
    SubMatch(SubMatch&&) = default;
    SubMatch& operator=(SubMatch&&) = default;

    const bool matched = false;
    std::string str() const
    {
        return std::string(mTarget->c_str() + mPmatch->rm_so, mPmatch->rm_eo - mPmatch->rm_so);
    }

    operator std::string() const
    {
        return str();
    }

    std::size_t length() const
    {
        return mPmatch->rm_eo - mPmatch->rm_so;
    }

private:
    friend class MatchResults;
    friend class SubMatchIterator;
    SubMatch(const std::string* target, const regmatch_t* pmatch)
        : matched(pmatch->rm_so != -1),
          mTarget(target),
          mPmatch(pmatch)
    {
    }

    const std::string* mTarget;
    const regmatch_t* mPmatch;
};

class SubMatchIterator
{
public:
    SubMatchIterator(const SubMatchIterator&) = default;
    SubMatchIterator& operator=(const SubMatchIterator&) = default;
    SubMatchIterator(SubMatchIterator&&) = default;
    SubMatchIterator& operator=(SubMatchIterator&&) = default;

    SubMatch operator*() const
    {
        return SubMatch(mTarget, &mPmatch[mIndex]);
    }

    SubMatchIterator& operator++()
    {
        ++mIndex;
        return *this;
    }

    SubMatchIterator operator++(int)
    {
        SubMatchIterator tmp = *this;
        ++mIndex;
        return tmp;
    }

    bool operator!=(const SubMatchIterator& other) const
    {
        return mIndex != other.mIndex;
    }
    bool operator==(const SubMatchIterator& other) const
    {
        return mIndex == other.mIndex;
    }

private:
    friend class MatchResults;
    SubMatchIterator() = default;
    SubMatchIterator(const std::string* target, const regmatch_t* pmatch, std::size_t index)
        : mTarget(target),
          mPmatch(pmatch),
          mIndex(index)
    {
    }

    const std::string* mTarget = nullptr;
    const regmatch_t* mPmatch = nullptr;
    std::size_t mIndex = 0;
};

class MatchResults
{
public:
    MatchResults() = default;
    MatchResults(const MatchResults&) = delete;
    MatchResults(MatchResults&&) = default;
    MatchResults& operator=(MatchResults&&) = default;

    std::size_t size() const
    {
        return mSize;
    }

    bool ready() const
    {
        return nullptr != mPmatch;
    }

    SubMatch operator[](std::size_t i) const
    {
        assert(ready());
        assert(i < mSize);
        return SubMatch(&mTarget, &mPmatch[i]);
    }

    SubMatchIterator begin() const
    {
        return SubMatchIterator(&mTarget, mPmatch.get(), 0);
    }

    SubMatchIterator end() const
    {
        return SubMatchIterator(&mTarget, mPmatch.get(), mSize);
    }

    std::string prefix() const
    {
        assert(ready());
        assert(size() > 0);
        return std::string(mTarget.c_str(), mPmatch[0].rm_so);
    }

    std::string suffix() const
    {
        assert(ready());
        assert(size() > 0);
        return std::string(mTarget.c_str() + mPmatch[mSize - 1].rm_eo, mTarget.length() - mPmatch[mSize - 1].rm_eo);
    }

private:
    friend bool regexSearch(const std::string& s, MatchResults& m, const Regex& r);
    MatchResults(std::string target, std::unique_ptr<regmatch_t[]> matches, std::size_t size)
        : mTarget(std::move(target)),
          mPmatch(std::move(matches))
    {
        mSize = 0;
        for (mSize = 0; mSize < size; ++mSize)
        {
            if (mPmatch[mSize].rm_so == -1)
            {
                break;
            }
        }
    }

    std::string mTarget;
    std::size_t mSize = 0;
    std::unique_ptr<regmatch_t[]> mPmatch;
};

inline bool regexSearch(const std::string& s, MatchResults& m, const Regex& r)
{
    const auto size = r.preg.re_nsub + 1;
    auto matches = std::unique_ptr<regmatch_t[]>(new regmatch_t[size]);
    auto result = (0 == regexec(&r.preg, s.c_str(), size, matches.get(), 0));
    m = MatchResults(s, std::move(matches), result ? size : 0);
    return result;
}

inline bool regexSearch(const std::string& s, const Regex& r)
{
    return (0 == regexec(&r.preg, s.c_str(), 0, NULL, 0));
}

} // namespace RegexLibcWrapper

#define regex_search RegexLibcWrapper::regexSearch
using regex = RegexLibcWrapper::Regex;
using smatch = RegexLibcWrapper::MatchResults;
using sub_match = RegexLibcWrapper::SubMatch;
#endif // COMPLIANCE_REGEX_FALLBACK_H
