#include <CommonUtils.h>
#include <EnsureGroupIsOnlyGroupWith.h>
#include <Evaluator.h>
#include <GroupsIterator.h>
#include <StringTools.h>
#include <vector>

using std::map;
using std::string;
using std::vector;

namespace ComplianceEngine
{
Result<Status> AuditEnsureGroupIsOnlyGroupWith(const EnsureGroupIsOnlyGroupWithParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.test_etcGroupPath.HasValue());
    bool hasGid = false;

    auto groups = GroupsRange::Make(params.test_etcGroupPath.Value(), context.GetLogHandle());
    if (!groups.HasValue())
    {
        return groups.Error();
    }

    for (const auto& item : groups.Value())
    {
        if (params.gid.HasValue() && item.gr_gid == static_cast<decltype(item.gr_gid)>(params.gid.Value()))
        {
            if (item.gr_name != params.group)
            {
                OsConfigLogDebug(context.GetLogHandle(), "Group '%s' has GID %d, but expected '%s'.", item.gr_name, item.gr_gid, params.group.c_str());
                return indicators.NonCompliant("A group other than '" + params.group + "' has GID " + std::to_string(item.gr_gid));
            }

            hasGid = true;
        }
    }

    if (params.gid.HasValue() && !hasGid)
    {
        OsConfigLogDebug(context.GetLogHandle(), "No group with GID %d found.", params.gid.Value());
        return indicators.NonCompliant("No group with GID " + std::to_string(params.gid.Value()) + " found");
    }

    return indicators.Compliant("All criteria has been met for group '" + params.group + "'");
}
} // namespace ComplianceEngine
