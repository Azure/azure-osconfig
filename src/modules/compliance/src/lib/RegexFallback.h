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
#include <stdexcept>
#include <string>

namespace RegexLibcWrapper
{
class RegexException : public std::runtime_error
{
    std::regex_constants::error_type mCode;

public:
    RegexException(const std::string& message, int code)
        : std::runtime_error(message),
          mCode(std::regex_constants::error_space)
    {
        switch (code)
        {
            case REG_BADBR:
                mCode = std::regex_constants::error_backref;
                break;
            case REG_BADPAT:
                mCode = std::regex_constants::error_paren;
                break;
            case REG_BADRPT:
                mCode = std::regex_constants::error_badrepeat;
                break;
            case REG_EBRACE:
                mCode = std::regex_constants::error_brace;
                break;
            case REG_EBRACK:
                mCode = std::regex_constants::error_brack;
                break;
            case REG_ECOLLATE:
                mCode = std::regex_constants::error_collate;
                break;
            case REG_ECTYPE:
                mCode = std::regex_constants::error_ctype;
                break;
            case REG_EESCAPE:
                mCode = std::regex_constants::error_escape;
                break;
            case REG_EPAREN:
                mCode = std::regex_constants::error_paren;
                break;
            case REG_ERANGE:
                mCode = std::regex_constants::error_range;
                break;
            case REG_ESUBREG:
                mCode = std::regex_constants::error_backref;
                break;
            default:
                mCode = std::regex_constants::error_space;
                break;
        }
    }

    RegexException(const RegexException&) = default;
    RegexException(RegexException&&) = default;
    RegexException& operator=(RegexException&&) = default;
    RegexException& operator=(const RegexException&) = default;

    std::regex_constants::error_type code() const
    {
        return mCode;
    }
};

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
        preg = std::unique_ptr<regex_t>(new regex_t);
        int v = regcomp(preg.get(), r.c_str(), ConvertFlags(options));
        if (0 != v)
        {
            char errbuf[256];
            regerror(v, preg.get(), errbuf, sizeof(errbuf));
            throw RegexException(errbuf, v);
        }
    }
    ~Regex()
    {
        if (preg)
        {
            regfree(preg.get());
        }
    }
    std::unique_ptr<regex_t> preg;
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
        if (!matched)
        {
            return std::string();
        }
        return std::string(mTarget->c_str() + mPmatch->rm_so, mPmatch->rm_eo - mPmatch->rm_so);
    }

    operator std::string() const
    {
        return str();
    }

    std::size_t length() const
    {
        if (!matched)
        {
            return 0;
        }
        return mPmatch->rm_eo - mPmatch->rm_so;
    }

private:
    friend class MatchResults;
    friend class SubMatchIterator;
    SubMatch(const std::string* target, const regmatch_t* pmatch)
        : matched((nullptr != pmatch) && (pmatch->rm_so != -1)),
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
        if (i < mSize)
        {
            return SubMatch(&mTarget, &mPmatch[i]);
        }

        return SubMatch(&mTarget, nullptr);
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
    friend bool regexMatch(const std::string& s, MatchResults& m, const Regex& r);
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
    const auto size = r.preg->re_nsub + 1;
    auto matches = std::unique_ptr<regmatch_t[]>(new regmatch_t[size]);
    auto result = (0 == regexec(r.preg.get(), s.c_str(), size, matches.get(), 0));
    m = MatchResults(s, std::move(matches), result ? size : 0);
    return result;
}

inline bool regexMatch(const std::string& s, const Regex& r)
{
    return (0 == regexec(r.preg.get(), s.c_str(), 0, NULL, 0));
}

inline bool regexMatch(const std::string& s, MatchResults& m, const Regex& r)
{
    const auto size = r.preg->re_nsub + 1;
    auto matches = std::unique_ptr<regmatch_t[]>(new regmatch_t[size]);
    auto result = (0 == regexec(r.preg.get(), s.c_str(), size, matches.get(), 0));
    if (result)
    {
        if (matches[0].rm_so != 0)
        {
            return false;
        }

        if (matches[0].rm_eo != static_cast<regoff_t>(s.length()))
        {
            return false;
        }
    }
    m = MatchResults(s, std::move(matches), result ? size : 0);
    return result;
}

inline bool regexSearch(const std::string& s, const Regex& r)
{
    return (0 == regexec(r.preg.get(), s.c_str(), 0, NULL, 0));
}
} // namespace RegexLibcWrapper

namespace std
{
inline std::ostream& operator<<(std::ostream& os, const RegexLibcWrapper::SubMatch& m)
{
    os << m.str();
    return os;
}
} // namespace std

#define regex_search RegexLibcWrapper::regexSearch
#define regex_match RegexLibcWrapper::regexMatch
using regex = RegexLibcWrapper::Regex;
using smatch = RegexLibcWrapper::MatchResults;
using regex_error = RegexLibcWrapper::RegexException;
#endif // COMPLIANCE_REGEX_FALLBACK_H
