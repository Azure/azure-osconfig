#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <ScopeGuard.h>
#include <StringTools.h>
#include <UsersIterator.h>
#include <shadow.h>
#include <vector>

using std::map;
using std::string;
using std::vector;

namespace ComplianceEngine
{
AUDIT_FN(EnsureUserIsOnlyAccountWith, "username:A pattern or value to match usernames against", "uid:A value to match the UID against::d+",
    "gid:A value to match the GID against::d+", "test_etcPasswdPath:Alternative path to the /etc/passwd file to test against")
{
    auto it = args.find("username");
    if (it == args.end())
    {
        return Error("Missing 'username' parameter", EINVAL);
    }
    const auto username = std::move(it->second);

    string passwdPath = "/etc/passwd";
    it = args.find("test_etcPasswdPath");
    if (it != args.end())
    {
        passwdPath = std::move(it->second);
    }

    Optional<decltype(passwd::pw_uid)> uid;
    bool hasUid = false;
    it = args.find("uid");
    if (it != args.end())
    {
        auto parsedUID = TryStringToInt(std::move(it->second));
        if (!parsedUID.HasValue())
        {
            return parsedUID.Error();
        }
        uid = parsedUID.Value();
    }

    Optional<decltype(passwd::pw_gid)> gid;
    bool hasGid = false;
    it = args.find("gid");
    if (it != args.end())
    {
        auto parsedGID = TryStringToInt(std::move(it->second));
        if (!parsedGID.HasValue())
        {
            return parsedGID.Error();
        }
        gid = parsedGID.Value();
    }

    auto users = UsersRange::Make(passwdPath, context.GetLogHandle());
    if (!users.HasValue())
    {
        return users.Error();
    }

    for (const auto& item : users.Value())
    {
        if (uid.HasValue() && item.pw_uid == uid.Value())
        {
            if (item.pw_name != username)
            {
                OsConfigLogDebug(context.GetLogHandle(), "User '%s' has UID %d, but expected '%s'.", item.pw_name, item.pw_uid, username.c_str());
                return indicators.NonCompliant("A user other than '" + username + "' has UID " + std::to_string(item.pw_uid));
            }

            hasUid = true;
        }

        if (gid.HasValue() && item.pw_gid == gid.Value())
        {
            if (item.pw_name != username)
            {
                OsConfigLogDebug(context.GetLogHandle(), "User '%s' has GID %d, but expected '%s'.", item.pw_name, item.pw_gid, username.c_str());
                return indicators.NonCompliant("A user other than '" + username + "' has GID " + std::to_string(item.pw_gid));
            }

            hasGid = true;
        }
    }

    if (uid.HasValue() && !hasUid)
    {
        OsConfigLogDebug(context.GetLogHandle(), "No user with UID %d found.", uid.Value());
        return indicators.NonCompliant("No user with UID " + std::to_string(uid.Value()) + " found");
    }

    if (gid.HasValue() && !hasGid)
    {
        OsConfigLogDebug(context.GetLogHandle(), "No user with GID %d found.", gid.Value());
        return indicators.NonCompliant("No user with GID " + std::to_string(gid.Value()) + " found");
    }

    return indicators.Compliant("All criteria has been met for user '" + username + "'");
}
} // namespace ComplianceEngine
