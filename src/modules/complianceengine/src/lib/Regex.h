// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_REGEX_H
#define COMPLIANCEENGINE_REGEX_H

#include <bits/c++config.h>

// GCC <= 4.8 has broken std::regex support. This is a very
// ugly hack that uses libc regexes to emulate std::regex
// for the functionality that we currently need in ComplianceEngine.
// It has to be removed and forgotten after we abandon support
// for older GCC versions.

#if __cplusplus >= 201103L && ((__cplusplus >= 201402L) || (defined(_GLIBCXX_REGEX_DFS_QUANTIFIERS_LIMIT) || defined(_GLIBCXX_REGEX_STATE_LIMIT) ||    \
                                                               (defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE > 4)))
#define USE_REGEX_FALLBACK 0
#else
#define USE_REGEX_FALLBACK 1
#endif

#if USE_REGEX_FALLBACK == 0
#include <regex>
#define regex_search std::regex_search
#define regex_match std::regex_match
using regex = std::regex;
using smatch = std::smatch;
using regex_error = std::regex_error;
#else // #if USE_REGEX_FALLBACK == 0
#include <RegexFallback.h>
#endif // #if USE_REGEX_FALLBACK == 0

#endif // COMPLIANCEENGINE_REGEX_H
