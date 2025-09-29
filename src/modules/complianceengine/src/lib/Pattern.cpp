// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Pattern.h>

namespace ComplianceEngine
{
Pattern::Pattern(const std::string& pattern) noexcept(false)
    : mPattern(pattern),
      mRegex(pattern)
{
}
} // namespace ComplianceEngine
