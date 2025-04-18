// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ITERATE_USERS_H
#define COMPLIANCE_ITERATE_USERS_H

#include <CommonUtils.h>
#include <Result.h>
#include <cerrno>
#include <memory>
#include <pwd.h>

namespace compliance
{
enum class BreakOnFalse
{
    True,
    False
};

template <typename Callable>
Result<bool> IterateUsers(Callable callable, BreakOnFalse breakOnFalse, OsConfigLogHandle log)
{
    bool result = true;
    struct passwd* pwd = nullptr;
    setpwent();
    for (errno = 0, pwd = getpwent(); nullptr != pwd; errno = 0, pwd = getpwent())
    {
        auto callbackResult = callable(pwd);
        if (!callbackResult.HasValue())
        {
            OsConfigLogDebug(log, "Iteration failed");
            return callbackResult.Error();
        }

        if (!callbackResult.Value())
        {
            result = false;
            if (breakOnFalse == BreakOnFalse::True)
            {
                OsConfigLogDebug(log, "Iteration stopped");
                break;
            }
            else
            {
                OsConfigLogDebug(log, "Callback returned false, but continuing");
            }
        }
    }

    int status = errno;
    endpwent();
    if (0 != status)
    {
        return Error(std::string("getpwent failed: ") + strerror(status), status);
    }

    return result;
}
} // namespace compliance

#endif // COMPLIANCE_ITERATE_USERS_H
