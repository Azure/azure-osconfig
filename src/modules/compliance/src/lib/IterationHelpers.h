// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ITERATION_HELPERS_H
#define COMPLIANCE_ITERATION_HELPERS_H

namespace compliance
{
// Enumeration to control the behavior of various iteration functions
// when a non-compliant user is encountered.
// If set to True, the iteration will stop immediately upon finding a non-compliant user.
// If set to False, the iteration will continue to check all users, but the overall result
// will be non-compliant if any user is found to be non-compliant.
enum class BreakOnNonCompliant
{
    True,
    False
};
} // namespace compliance

#endif // COMPLIANCE_ITERATION_HELPERS_H
