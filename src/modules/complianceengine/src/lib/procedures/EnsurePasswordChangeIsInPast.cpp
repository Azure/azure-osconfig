#include <CommonUtils.h>
#include <Evaluator.h>
#include <PasswordEntriesIterator.h>
#include <Regex.h>
#include <ScopeGuard.h>
#include <pwd.h>
#include <shadow.h>
#include <vector>

using std::map;
using std::string;
using std::vector;

namespace ComplianceEngine
{
AUDIT_FN(EnsurePasswordChangeIsInPast, "test_etcShadowPath:Path to the /etc/shadow file to test against::/etc/shadow")
{
    auto etcShadowPath = string("/etc/shadow");
    auto it = args.find("test_etcShadowPath");
    if (it != args.end())
    {
        etcShadowPath = std::move(it->second);
    }

    auto range = PasswordEntryRange::Make(etcShadowPath, context.GetLogHandle());
    if (!range.HasValue())
    {
        return range.Error();
    }

    const std::size_t maxInvalidUsers = 5;
    std::size_t invalidUsersCount = 0;
    const auto today = time(nullptr) / (24 * 3600);
    for (const auto& item : range.Value())
    {
        OsConfigLogDebug(context.GetLogHandle(), "Processing user: %s, %ld %ld", item.sp_namp, today, item.sp_lstchg);
        if (today >= item.sp_lstchg)
        {
            continue;
        }

        OsConfigLogDebug(context.GetLogHandle(), "User %s has a password change date in the future: %ld", item.sp_namp, item.sp_lstchg);
        if (invalidUsersCount < maxInvalidUsers)
        {
            const auto* pwd = getpwnam(item.sp_namp);
            if (pwd)
            {
                indicators.NonCompliant("User " + std::to_string(pwd->pw_uid) + " has a password change date in the future");
            }
            else
            {
                indicators.NonCompliant("Some user has a password change date in the future and is not present in password database");
            }
        }
        invalidUsersCount++;
        if (invalidUsersCount >= maxInvalidUsers)
        {
            OsConfigLogInfo(context.GetLogHandle(), "Too many invalid users found, stopping further checks.");
            break;
        }
    }

    if (0 == invalidUsersCount)
    {
        OsConfigLogDebug(context.GetLogHandle(), "All users have password change dates in the past.");
        return indicators.Compliant("All users have password change dates in the past");
    }

    return indicators.NonCompliant("At least " + std::to_string(invalidUsersCount) + " users have password change dates in the future");
}
} // namespace ComplianceEngine
