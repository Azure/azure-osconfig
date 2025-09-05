// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_PATTERN_H
#define COMPLIANCEENGINE_PATTERN_H

#include <Regex.h>
#include <Result.h>

namespace ComplianceEngine
{
// Binds together a pattern and associated regular expression
class Pattern
{
    // The pattern string
    std::string mPattern;

    // The compiled regular expression
    regex mRegex;

    // Constructs the pattern, may frow on incorrect patterns.
    // Use Make(pattern) to get a Result<Pattern> object instead.
    Pattern(const std::string& pattern) noexcept(false);

public:
    Pattern() = default;
    Pattern(const Pattern&) = default;
    Pattern(Pattern&&) = default;
    Pattern& operator=(const Pattern& other)
    {
        if (&other == this)
            return *this;
        mPattern = other.mPattern;
        mRegex = other.mRegex;
        return *this;
    }

    Pattern& operator=(Pattern&& other) noexcept
    {
        if (&other == this)
            return *this;
        mPattern = std::move(other.mPattern);
        mRegex = std::move(other.mRegex);
        return *this;
    }

    ~Pattern() = default;

    // Factory method to create a Pattern object, returns an error if the pattern is invalid
    static Result<Pattern> Make(const std::string& pattern)
    {
        try
        {
            return Pattern(pattern);
        }
        catch (const std::exception& e)
        {
            return Error("Regular expression '" + pattern + "' compilation failed: " + e.what(), EINVAL);
        }
    }

    // Get the original pattern string
    const std::string& GetPattern() const
    {
        return mPattern;
    }

    // Get the compiled regex object
    const regex& GetRegex() const&
    {
        return mRegex;
    }

    // Get the compiled regex object
    regex& GetRegex() &
    {
        return mRegex;
    }
};
} // namespace ComplianceEngine

namespace std
{
inline string ToString(const ComplianceEngine::Pattern& pattern)
{
    return pattern.GetPattern();
}
} // namespace std
#endif // COMPLIANCEENGINE_PATTERN_H
