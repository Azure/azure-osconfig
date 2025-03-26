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

#    include <regex>
#    include <string>
#    define regex_search std::regex_search
using regex = std::regex;

#else

#    include <regex.h>
namespace RegexLibcWrapper
{
class Regex
{
public:
    Regex(const std::string& r)
    {
        this->preg = new (regex_t);
        int v = regcomp(this->preg, r.c_str(), REG_EXTENDED);
        if (0 != v)
        {
            throw std::runtime_error("Invalid regex");
        }
    }
    std::unique_ptr<regex_t> preg;
};
bool regexSearch(const std::string& s, const Regex& r)
{
    return (regexec(r.preg, s.c_str(), 0, NULL, 0) == 0);
}
} // namespace RegexLibcWrapper

#    define regex_search                                                                                                                               \
    RegexLibcWrapper:                                                                                                                                  \
        regexSearch
using regex = RegexLibcWrapper::Regex;

#endif
