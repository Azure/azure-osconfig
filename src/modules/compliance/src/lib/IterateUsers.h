// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ITERATE_USERS_H
#define COMPLIANCE_ITERATE_USERS_H

#include <ContextInterface.h>
#include <IterationHelpers.h>
#include <MmiResults.h>
#include <Result.h>
#include <functional>
#include <pwd.h>

namespace compliance
{
using UserIterationCallback = std::function<Result<Status>(const struct passwd*)>;
// Iterate over all users in the system and apply the provided callback function to each user.
// The callback function should return a Result<Status> indicating the compliance status of the user.
// If the callback returns a non-compliant status and breakOnNonCompliant is set to true, the iteration will stop.
// The function returns a Result<Status> indicating the overall compliance status of all users.
// If the iteration encounters an error, it will return an Error object with the error message and code.
Result<Status> IterateUsers(UserIterationCallback callback, BreakOnNonCompliant breakOnNonCompliant, ContextInterface& context);
} // namespace compliance

#endif // COMPLIANCE_ITERATE_USERS_H
