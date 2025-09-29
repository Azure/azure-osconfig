// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_REGEX_FALLBACK_H
#define COMPLIANCEENGINE_REGEX_FALLBACK_H

// We don't want clang-tidy to touch this file as we're keeping compatibility
// with the original regex, which means e.g. different function names.

// NOLINTBEGIN

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
        if ((options & std::regex_constants::extended) || (options & std::regex_constants::ECMAScript))
        {
            flags |= REG_EXTENDED;
        }
        return flags;
    }

public:
    Regex() = default;
    Regex(const Regex& other)
        : Regex(other.pattern, other.options)
    {
    }
    Regex(Regex&& other) noexcept
        : pattern(std::move(other.pattern)),
          options(other.options),
          preg(std::move(other.preg))
    {
    }
    Regex& operator=(const Regex& other)
    {
        *this = Regex(other.pattern, other.options);
        return *this;
    }
    Regex& operator=(Regex&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        pattern = std::move(other.pattern);
        options = other.options;
        preg = std::move(other.preg);
        return *this;
    }

    Regex(const std::string& r, std::regex_constants::syntax_option_type options = std::regex_constants::extended)
        : pattern(r),
          options(options)
    {
        preg = std::unique_ptr<regex_t>(new regex_t);
        std::string newR;
        newR.reserve(pattern.size() + 1);
        bool escapeNext = false;
        for (const char& c : pattern)
        {
            if (escapeNext)
            {
                switch (c)
                {
                    case 'n':
                        newR += '\n';
                        break;
                    case 't':
                        newR += '\t';
                        break;
                    case 'r':
                        newR += '\r';
                        break;
                    case 'h':
                        newR += "[ \t]";
                        break;
                    default:
                        // keep characters as they are
                        newR += '\\';
                        newR += c;
                }
                escapeNext = false;
            }
            else if (c == '\\')
            {
                escapeNext = true;
            }
            else
            {
                newR += c;
            }
        }
        if (escapeNext)
        {
            newR += '\\'; // add last backslash if it was not escaped
        }
        int v = regcomp(preg.get(), newR.c_str(), ConvertFlags(options));
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
    std::string pattern;
    std::regex_constants::syntax_option_type options;
    std::unique_ptr<regex_t> preg;
};

class SubMatch
{
public:
    SubMatch() = delete;
    SubMatch(const SubMatch&) = default;
    SubMatch& operator=(const SubMatch& other)
    {
        if (this == &other)
        {
            return *this;
        }
        mTarget = other.mTarget;
        mPmatch = other.mPmatch;
        return *this;
    }
    SubMatch(SubMatch&&) = default;
    SubMatch& operator=(SubMatch&& other)
    {
        if (this == &other)
        {
            return *this;
        }

        mTarget = other.mTarget;
        mPmatch = other.mPmatch;
        return *this;
    }

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

    bool empty() const
    {
        return mSize == 0;
    }

    std::size_t position(std::size_t i) const
    {
        assert(ready());
        if (i < mSize)
        {
            return mPmatch[i].rm_so;
        }
        return std::string::npos;
    }

    std::size_t length(std::size_t i) const
    {
        assert(ready());
        if (i < mSize)
        {
            return mPmatch[i].rm_eo - mPmatch[i].rm_so;
        }
        return 0;
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
          mSize(size),
          mPmatch(std::move(matches))
    {
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
    regmatch_t matches[1];
    auto result = (0 == regexec(r.preg.get(), s.c_str(), 1, matches, 0));
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
    return result;
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
            m = MatchResults(s, std::move(matches), 0);
            return false;
        }

        if (matches[0].rm_eo != static_cast<regoff_t>(s.length()))
        {
            m = MatchResults(s, std::move(matches), 0);
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

// NOLINTEND

#endif // COMPLIANCEENGINE_REGEX_FALLBACK_H
