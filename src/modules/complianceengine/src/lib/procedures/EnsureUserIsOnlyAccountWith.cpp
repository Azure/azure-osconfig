#include <CommonUtils.h>
#include <EnsureUserIsOnlyAccountWith.h>
#include <UsersIterator.h>
#include <shadow.h>
#include <vector>

using std::map;
using std::string;
using std::vector;

namespace ComplianceEngine
{
Result<Status> AuditEnsureUserIsOnlyAccountWith(const EnsureUserIsOnlyAccountWithParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.test_etcPasswdPath.HasValue());
    bool hasUid = false;
    bool hasGid = false;

    auto users = UsersRange::Make(params.test_etcPasswdPath.Value(), context.GetLogHandle());
    if (!users.HasValue())
    {
        return users.Error();
    }

    for (const auto& item : users.Value())
    {
        if (params.uid.HasValue() && item.pw_uid == static_cast<decltype(item.pw_uid)>(params.uid.Value()))
        {
            if (item.pw_name != params.username)
            {
                OsConfigLogDebug(context.GetLogHandle(), "User '%s' has UID %d, but expected '%s'.", item.pw_name, item.pw_uid, params.username.c_str());
                return indicators.NonCompliant("A user other than '" + params.username + "' has UID " + std::to_string(item.pw_uid));
            }

            hasUid = true;
        }

        if (params.gid.HasValue() && item.pw_gid == static_cast<decltype(item.pw_uid)>(params.gid.Value()))
        {
            if (item.pw_name != params.username)
            {
                OsConfigLogDebug(context.GetLogHandle(), "User '%s' has GID %d, but expected '%s'.", item.pw_name, item.pw_gid, params.username.c_str());
                return indicators.NonCompliant("A user other than '" + params.username + "' has GID " + std::to_string(item.pw_gid));
            }

            hasGid = true;
        }
    }

    if (params.uid.HasValue() && !hasUid)
    {
        OsConfigLogDebug(context.GetLogHandle(), "No user with UID %d found.", params.uid.Value());
        return indicators.NonCompliant("No user with UID " + std::to_string(params.uid.Value()) + " found");
    }

    if (params.gid.HasValue() && !hasGid)
    {
        OsConfigLogDebug(context.GetLogHandle(), "No user with GID %d found.", params.gid.Value());
        return indicators.NonCompliant("No user with GID " + std::to_string(params.gid.Value()) + " found");
    }

    return indicators.Compliant("All criteria has been met for user '" + params.username + "'");
}
} // namespace ComplianceEngine
