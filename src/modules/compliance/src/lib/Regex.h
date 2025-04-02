// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// GCC <= 4.8 has broken std::regex support. This is a very
// ugly hack that uses libc regexes to emulate std::regex
// for the functionality that we currently need in compliance.
// It has to be removed and forgotten after we abandon support
// for older GCC versions.

#if __cplusplus >= 201103L &&                                                                                                                          \
    (!defined(__GLIBCXX__) || (__cplusplus >= 201402L) ||                                                                                              \
        (defined(_GLIBCXX_REGEX_DFS_QUANTIFIERS_LIMIT) || defined(_GLIBCXX_REGEX_STATE_LIMIT) || (defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE > 4)))

#include <regex>
#define regex_search std::regex_search
using regex = std::regex;

#else

#include <memory>
#include <regex.h>
#include <regex>
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
        this->preg = std::unique_ptr<regex_t>(new (regex_t));
        int v = regcomp(this->preg.get(), r.c_str(), ConvertFlags(options));
        if (0 != v)
        {
            throw std::runtime_error("Invalid regex");
        }
    }
    std::unique_ptr<regex_t> preg;
};

inline bool regexSearch(const std::string& s, const Regex& r)
{
    return (0 == regexec(r.preg.get(), s.c_str(), 0, NULL, 0));
}
} // namespace RegexLibcWrapper

#define regex_search RegexLibcWrapper::regexSearch
using regex = RegexLibcWrapper::Regex;

#endif
