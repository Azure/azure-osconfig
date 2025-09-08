// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_BINDING_PARSERS_H
#define COMPLIANCEENGINE_BINDING_PARSERS_H

#include <Pattern.h>
#include <Result.h>
#include <string>
#include <sys/types.h>

namespace ComplianceEngine
{
namespace BindingParsers
{
// Generic parsing function, must be specialized for each supported type
template <typename T>
Result<T> Parse(const std::string& input);

// Parse a string parameter
template <>
Result<std::string> Parse<std::string>(const std::string& input);

// Parse an integer parameter
template <>
Result<int> Parse<int>(const std::string& input);

// Parse a regex parameter
template <>
Result<regex> Parse<regex>(const std::string& input);

// Parse a regex parameter
template <>
Result<Pattern> Parse<Pattern>(const std::string& input);

// Parse a boolean parameter
template <>
Result<bool> Parse<bool>(const std::string& input);

// Parse an octal integer parameter
template <>
Result<mode_t> Parse<mode_t>(const std::string& input);
} // namespace BindingParsers
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_BINDING_PARSERS_H
