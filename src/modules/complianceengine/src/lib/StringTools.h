// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_STRING_TOOLS_H
#define COMPLIANCEENGINE_STRING_TOOLS_H

#include <Result.h>

namespace ComplianceEngine
{
std::string EscapeForShell(const std::string& str);
std::string TrimWhiteSpaces(const std::string& str);
Result<int> TryStringToInt(const std::string& str, int base = 10);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_STRING_TOOLS_H
