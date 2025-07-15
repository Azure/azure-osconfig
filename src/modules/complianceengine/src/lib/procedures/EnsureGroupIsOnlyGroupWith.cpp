#include <CommonUtils.h>
#include <Evaluator.h>
#include <GroupsIterator.h>
#include <vector>

using std::map;
using std::string;
using std::vector;

namespace ComplianceEngine
{
AUDIT_FN(EnsureGroupIsOnlyGroupWith, "group:A pattern or value to match usernames against", "gid:A value to match the UID against::d+",
    "test_etcGroupPath:Alternative path to the /etc/group file to test against")
{
    auto it = args.find("group");
    if (it == args.end())
    {
        return Error("Missing 'group' parameter", EINVAL);
    }
    const auto groupName = std::move(it->second);

    string groupPath = "/etc/group";
    it = args.find("test_etcGroupPath");
    if (it != args.end())
    {
        groupPath = std::move(it->second);
    }

    Optional<decltype(group::gr_gid)> gid;
    bool hasGid = false;
    it = args.find("gid");
    if (it != args.end())
    {
        try
        {
            gid = std::stoi(it->second);
        }
        catch (const std::invalid_argument&)
        {
            return Error("Invalid UID value: " + it->second, EINVAL);
        }
        catch (const std::out_of_range&)
        {
            return Error("UID value out of range: " + it->second, ERANGE);
        }
    }

    auto users = GroupsRange::Make(groupPath, context.GetLogHandle());
    if (!users.HasValue())
    {
        return users.Error();
    }

    for (const auto& item : users.Value())
    {
        if (gid.HasValue() && item.gr_gid == gid.Value())
        {
            if (hasGid && item.gr_name != groupName)
            {
                OsConfigLogDebug(context.GetLogHandle(), "User '%s' has UID %d, but expected '%s'.", item.gr_name, item.gr_gid, groupName.c_str());
                return indicators.NonCompliant("User '" + string(item.gr_name) + "' has UID " + std::to_string(item.gr_gid));
            }

            hasGid = true;
        }
    }

    if (gid.HasValue() && !hasGid)
    {
        OsConfigLogDebug(context.GetLogHandle(), "No user with UID %d found.", gid.Value());
        return indicators.NonCompliant("No user with UID " + std::to_string(gid.Value()) + " found");
    }

    return indicators.Compliant("All criteria has been met for user '" + groupName + "'");
}
} // namespace ComplianceEngine
