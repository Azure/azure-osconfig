// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_STRING_TOOLS_H
#define COMPLIANCEENGINE_STRING_TOOLS_H

#include <Result.h>

namespace ComplianceEngine
{

// ============================================================================
// Security-related string utilities
// ============================================================================

/**
 * @brief Escapes a string for safe use inside double-quoted shell arguments.
 *
 * This function escapes characters that have special meaning inside double quotes:
 * - Backslash (\) - escape character
 * - Double quote (") - ends the quoted string
 * - Backtick (`) - command substitution
 * - Dollar sign ($) - variable expansion
 *
 * USAGE: The escaped string MUST be placed inside double quotes in the command.
 *
 * Example:
 *   std::string cmd = "gsettings get \"" + EscapeForShell(schema) + "\" \"" + EscapeForShell(key) + "\"";
 *
 * WARNING: This function does NOT protect against:
 * - Command chaining (;, &&, ||) when used outside quotes
 * - Pipe redirection (|, >, <) when used outside quotes
 * Always wrap the escaped value in double quotes!
 *
 * @param str The string to escape
 * @return The escaped string, safe for use inside double quotes
 */
std::string EscapeForShell(const std::string& str);

// ============================================================================
// General string utilities
// ============================================================================

/**
 * @brief Removes leading and trailing whitespace from a string.
 *
 * @param str The string to trim
 * @return The trimmed string
 */
std::string TrimWhiteSpaces(const std::string& str);

/**
 * @brief Safely converts a string to an integer.
 *
 * @param str The string to convert
 * @param base The numeric base (default: 10)
 * @return Result containing the integer value or an error
 */
Result<int> TryStringToInt(const std::string& str, int base = 10);

/**
 * @brief Safely converts a string to an unsigned integer.
 *
 * @param str The string to convert
 * @param base The numeric base (default: 10)
 * @return Result containing the unsigned integer value or an error
 */
Result<unsigned int> TryStringToUint(const std::string& str, int base = 10);

} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_STRING_TOOLS_H
