// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define PCRE2_CODE_UNIT_WIDTH 8

#include "Regex.h"

#include <iostream>
#include <pcre2.h>
#include <stdexcept>
#include <string>

namespace compliance
{
void Regex::PcreDeleter::operator()(pcre* value) const
{
    pcre2_code_free(value);
}

Result<Regex> Regex::Compile(const std::string& pattern) noexcept
{
    int errorcode = 0;
    PCRE2_SIZE erroroffset = 0;
    auto* regex = pcre2_compile((PCRE2_SPTR)pattern.c_str(), PCRE2_ZERO_TERMINATED, 0, &errorcode, &erroroffset, nullptr);
    if (nullptr == regex)
    {
        char buffer[256];
        pcre2_get_error_message(errorcode, (PCRE2_UCHAR*)buffer, sizeof(buffer));
        return Error("PCRE compilation failed at offset " + std::to_string(erroroffset) + ": " + std::string(buffer));
    }

    return Regex(regex);
}

Regex::Regex(pcre* regex) noexcept
    : mRegex(regex)
{
}

bool Regex::Match(const std::string& subject) const noexcept
{
    auto matchData = pcre2_match_data_create_from_pattern(mRegex.get(), nullptr);
    if (nullptr == matchData)
    {
        return false;
    }

    auto rc = pcre2_match(mRegex.get(), (PCRE2_SPTR)subject.c_str(), subject.length(), 0, 0, matchData, nullptr);
    if (rc < 0)
    {
        pcre2_match_data_free(matchData);
        return false;
    }

    pcre2_match_data_free(matchData);
    return true;
}
} // namespace compliance
