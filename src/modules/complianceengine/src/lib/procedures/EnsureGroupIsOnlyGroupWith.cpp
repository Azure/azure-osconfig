#include <CommonUtils.h>
#include <Evaluator.h>
#include <GroupsIterator.h>
#include <StringTools.h>
#include <vector>

using std::map;
using std::string;
using std::vector;

namespace ComplianceEngine
{
AUDIT_FN(EnsureGroupIsOnlyGroupWith, "group:A pattern or value to match group names against", "gid:A value to match the GID against::d+",
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
        auto parsedGID = TryStringToInt(std::move(it->second));
        if (!parsedGID.HasValue())
        {
            return parsedGID.Error();
        }
        gid = parsedGID.Value();
    }

    auto groups = GroupsRange::Make(groupPath, context.GetLogHandle());
    if (!groups.HasValue())
    {
        return groups.Error();
    }

    for (const auto& item : groups.Value())
    {
        if (gid.HasValue() && item.gr_gid == gid.Value())
        {
            if (item.gr_name != groupName)
            {
                OsConfigLogDebug(context.GetLogHandle(), "Group '%s' has GID %d, but expected '%s'.", item.gr_name, item.gr_gid, groupName.c_str());
                return indicators.NonCompliant("A group other than '" + groupName + "' has GID " + std::to_string(item.gr_gid));
            }

            hasGid = true;
        }
    }

    if (gid.HasValue() && !hasGid)
    {
        OsConfigLogDebug(context.GetLogHandle(), "No group with GID %d found.", gid.Value());
        return indicators.NonCompliant("No group with GID " + std::to_string(gid.Value()) + " found");
    }

    return indicators.Compliant("All criteria has been met for group '" + groupName + "'");
}
} // namespace ComplianceEngine
