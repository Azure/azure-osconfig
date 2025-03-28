// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_REGEX_H
#define COMPLIANCE_REGEX_H

// #define PCRE2_CODE_UNIT_WIDTH 8

#include <Result.h>
#include <memory>
#include <string>
// #include <pcre2.h>

struct pcre2_real_code_8;
namespace compliance
{
class Regex
{
private:
    using pcre = pcre2_real_code_8;
    struct PcreDeleter
    {
        void operator()(pcre* value) const;
    };
    std::unique_ptr<pcre, PcreDeleter> mRegex = nullptr;

    explicit Regex(pcre* regex) noexcept;

public:
    Regex(const Regex&) = delete;
    Regex& operator=(const Regex&) = delete;
    Regex(Regex&&) noexcept = default;
    Regex& operator=(Regex&&) noexcept = default;
    Regex() = delete;
    ~Regex() = default;

    static Result<Regex> Compile(const std::string& pattern) noexcept;
    bool Match(const std::string& subject) const noexcept;
};
} // namespace compliance

#endif // COMPLIANCE_REGEX_H
