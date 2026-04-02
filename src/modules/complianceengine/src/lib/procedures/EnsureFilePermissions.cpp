// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <Bindings.h>
#include <CommonUtils.h>
#include <EnsureFilePermissions.h>
#include <Evaluator.h>
#include <Telemetry.h>
#include <fnmatch.h>
#include <fts.h>
#include <grp.h>
#include <iomanip>
#include <iostream>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace ComplianceEngine
{
// Mask to display permissions
const unsigned short displayMask = 0xFFF;

Result<Status> EnsureFilePermissionsCollectionHelper(const EnsureFilePermissionsCollectionParams& params, IndicatorsTree& indicators,
    ContextInterface& context, bool isRemediation)
{

    assert(params.behavior.HasValue());
    const auto behavior = params.behavior.Value();
    auto log = context.GetLogHandle();
    auto directory = params.directory;
    // Respect explicit false; default behavior when unset is true
    bool recurse = params.recurse.ValueOr(true);
    char* const paths[] = {&directory[0], nullptr};
    FTS* ftsp = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR, nullptr);

    if (!ftsp)
    {
        if (Behavior::NoneExist == params.behavior.Value() || Behavior::CheckIfExists == params.behavior.Value())
        {
            OsConfigLogDebug(log, "Directory '%s' does not exist as expected ", directory.c_str());
            return indicators.Compliant("Directory '" + directory + "' does not exist as expected");
        }
        OsConfigLogInfo(log, "Directory '%s' does not exist, but it should", directory.c_str());
        return indicators.NonCompliant("Directory '" + directory + "' does not exist, but it should");
    }
    auto ftspDeleter = std::unique_ptr<FTS, int (*)(FTS*)>(ftsp, fts_close);

    FTSENT* entry = nullptr;
    int numberOfComplinatFiles = 0;
    int numberOfnonCompliantFiles = 0;
    while (nullptr != (entry = fts_read(ftsp)))
    {
        if (FTS_F == entry->fts_info && (recurse || entry->fts_level == 1))
        {
            if (0 == fnmatch(params.ext.c_str(), entry->fts_name, 0))
            {
                const char* fileName = entry->fts_path;

                EnsureFilePermissionsParams subParams;
                subParams.filename = fileName;
                subParams.owner = params.owner;
                subParams.group = params.group;
                subParams.permissions = params.permissions;
                subParams.mask = params.mask;
                subParams.behavior = params.behavior;
                Result<Status> result = isRemediation ? RemediateEnsureFilePermissions(subParams, indicators, context) :
                                                        AuditEnsureFilePermissions(subParams, indicators, context);
                if (!result.HasValue())
                {
                    OsConfigLogError(log, "Error processing permissions for '%s'", fileName);
                    OSConfigTelemetryStatusTrace(isRemediation ? "RemediateEnsureFilePermissions" : "AuditEnsureFilePermissions", result.Error().code);
                    return result;
                }

                if (Status::NonCompliant == result.Value())
                {
                    numberOfnonCompliantFiles++;
                    OsConfigLogError(log, "File '%s' does not match required permissions", fileName);
                }
                else
                {
                    numberOfComplinatFiles++;
                    OsConfigLogDebug(log, "File '%s' matches required permissions", fileName);
                }
            }
        }
        if (FTS_D == entry->fts_info && !recurse && (strcmp(entry->fts_path, directory.c_str()) != 0))
        {
            fts_set(ftsp, entry, FTS_SKIP);
        }
    }

    if (Behavior::CheckIfExists == behavior)
    {
        if (numberOfnonCompliantFiles == 0)
        {
            OsConfigLogDebug(log, "All matching files in '%s' match expected permissions", directory.c_str());
            return indicators.Compliant("All matching files in '" + directory + "' match expected permissions");
        }
        OsConfigLogDebug(log, "At least one file in '%s' did not match expected permissions", directory.c_str());
        return indicators.NonCompliant("At least one file in '" + directory + "' did not match expected permissions");
    }
    else if (Behavior::NoneExist == behavior)
    {
        if ((numberOfComplinatFiles == 0) && (numberOfnonCompliantFiles == 0))
        {
            OsConfigLogDebug(log, "No files in '%s' match the pattern, as expected", directory.c_str());
            return indicators.Compliant("No files in '" + directory + "' match the pattern as expected");
        }

        OsConfigLogDebug(log, "No files in '%s' match the pattern but they shold", directory.c_str());
        return indicators.NonCompliant("No matching files found in '" + directory + "' but they should");
    }
    else if (Behavior::AnyExist == behavior || Behavior::AtLeastOneExists == behavior)
    {
        if ((numberOfComplinatFiles > 0) && (numberOfnonCompliantFiles == 0))
        {
            OsConfigLogDebug(log, "At least one file in '%s' matched required permissions as expected", directory.c_str());
            return indicators.Compliant("At least one file in '" + directory + "' matched required permissions as expected");
        }
        OsConfigLogDebug(log, "At least one file in '%s' did not match required permissions but it should", directory.c_str());
        return indicators.NonCompliant("At least one file in '" + directory + "' did not match required permissions but it should");
    }
    else if (Behavior::OnlyOneExists == behavior)
    {
        if ((numberOfComplinatFiles == 1) && (numberOfnonCompliantFiles == 0))
        {
            OsConfigLogDebug(log, "Exactly one file in '%s' matched required permissions as expected", directory.c_str());
            return indicators.Compliant("Exactly one file in '" + directory + "' matched required permissions as expected");
        }
        if (numberOfComplinatFiles > 1)
        {
            OsConfigLogDebug(log, "Expected exactly one file in '%s' but more matched required permissions", directory.c_str());
            return indicators.NonCompliant("Expected exactly one file in '" + directory + "' but more matched required permissions");
        }
        OsConfigLogDebug(log, "Expected exactly one file in '%s' but more matched required permissions", directory.c_str());
        return indicators.NonCompliant("Expected exactly one file in '" + directory + "' but more matched required permissions");
    }
    else if (Behavior::AllExist == behavior)
    {
        if (numberOfnonCompliantFiles == 0 && (numberOfComplinatFiles > 0))
        {
            OsConfigLogDebug(log, "All files in '%s' matched required permissions as expected", directory.c_str());
            return indicators.Compliant("All files in '" + directory + "' matched required permissions as expected");
        }
        OsConfigLogDebug(log, "At least one file in '%s' did not match required permissions but they should", directory.c_str());
        return indicators.NonCompliant("At least one file in '" + directory + "' did not match required permissions but they should");
    }
    else
    {
        return Error("Unknown behavior: " + std::to_string(params.behavior.Value()), EINVAL);
    }
}

Result<Status> AuditEnsureFilePermissions(const EnsureFilePermissionsParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.behavior.HasValue());
    auto log = context.GetLogHandle();
    struct stat statbuf;
    if (0 != stat(params.filename.c_str(), &statbuf))
    {
        const int status = errno;
        if (ENOENT == status)
        {
            if (Behavior::NoneExist == params.behavior.Value() || Behavior::CheckIfExists == params.behavior.Value())
            {
                OsConfigLogDebug(log, "File '%s' does not exist as it should", params.filename.c_str());
                return indicators.Compliant("File '" + params.filename + "' does not exist as it should");
            }
            OsConfigLogDebug(log, "File '%s' does not exist but it should", params.filename.c_str());
            return indicators.NonCompliant("File '" + params.filename + "' does not exist but it should");
        }

        OsConfigLogError(log, "Stat error %s (%d)", strerror(status), status);
        OSConfigTelemetryStatusTrace("stat", status);
        return Error("Stat error '" + std::string(strerror(status)) + "'", status);
    }

    if (Behavior::NoneExist == params.behavior.Value())
    {
        OsConfigLogDebug(log, "File '%s' exist but it should not", params.filename.c_str());
        return indicators.NonCompliant("File '" + params.filename + "' exist but it should not");
    }
    // Behavior::CheckIfExists: file exists, proceed to check permissions

    if (params.owner.HasValue())
    {
        const passwd* pwd = getpwuid(statbuf.st_uid);
        if (nullptr == pwd)
        {
            OsConfigLogDebug(log, "No user with UID %d", statbuf.st_uid);
            return indicators.NonCompliant("No user with uid " + std::to_string(statbuf.st_uid));
        }
        bool ownerOk = false;
        for (const auto& owner : params.owner->items)
        {
            if (owner.GetPattern() == pwd->pw_name)
            {
                OsConfigLogDebug(log, "Matched owner '%s' to '%s'", owner.GetPattern().c_str(), pwd->pw_name);
                ownerOk = true;
                break;
            }
        }
        if (!ownerOk)
        {
            OsConfigLogDebug(log, "Invalid '%s' owner - is '%s' should be '%s'", params.filename.c_str(), pwd->pw_name, params.owner->ToString().c_str());
            return indicators.NonCompliant("Invalid owner on '" + params.filename + "' - is '" + std::string(pwd->pw_name) + "' should be '" +
                                           params.owner->ToString() + "'");
        }
        else
        {
            OsConfigLogDebug(log, "Matched owner '%s' to '%s'", params.owner->ToString().c_str(), pwd->pw_name);
        }

        indicators.Compliant(params.filename + " owner matches expected value '" + params.owner->ToString() + "'");
    }

    if (params.group.HasValue())
    {
        group* grp = getgrgid(statbuf.st_gid);
        if (nullptr == grp)
        {
            OsConfigLogDebug(log, "No group with GID %d", statbuf.st_gid);
            return indicators.NonCompliant("No group with gid " + std::to_string(statbuf.st_gid));
        }
        bool groupOk = false;
        for (const auto& group : params.group->items)
        {
            if (group.GetPattern() == grp->gr_name)
            {
                OsConfigLogDebug(log, "Matched group '%s' to '%s'", group.GetPattern().c_str(), grp->gr_name);
                groupOk = true;
                break;
            }
        }
        if (!groupOk)
        {
            OsConfigLogDebug(log, "Invalid group on '%s' - is '%s' should be '%s'", params.filename.c_str(), grp->gr_name, params.group->ToString().c_str());
            return indicators.NonCompliant("Invalid group on '" + params.filename + "' - is '" + std::string(grp->gr_name) + "' should be '" +
                                           params.group->ToString() + "'");
        }
        else
        {
            OsConfigLogDebug(log, "Matched group '%s' to '%s'", params.group->ToString().c_str(), grp->gr_name);
        }

        indicators.Compliant(params.filename + " group matches expected value '" + params.group->ToString() + "'");
    }

    if ((params.permissions.HasValue() && params.mask.HasValue()) && (0 != (params.permissions.Value() & params.mask.Value())))
    {
        OsConfigLogError(log, "Invalid permissions and mask - same bits set in both");
        OSConfigTelemetryStatusTrace("permissions", EINVAL);
        return Error("Invalid permissions and mask - same bits set in both");
    }
    if (params.permissions.HasValue())
    {
        if (params.permissions.Value() != (statbuf.st_mode & params.permissions.Value()))
        {
            std::ostringstream oss;
            oss << "Invalid permissions on '" << params.filename << "' - are " << std::oct << (statbuf.st_mode & displayMask) << " should be at least "
                << std::oct << params.permissions.Value();
            return indicators.NonCompliant(oss.str());
        }

        OsConfigLogDebug(log, "%s permissions are correct", params.filename.c_str());
        std::ostringstream oss;
        oss << params.filename << " matches expected permissions " << std::oct << params.permissions.Value();
        indicators.Compliant(oss.str());
    }
    if (params.mask.HasValue())
    {
        if (0 != (statbuf.st_mode & params.mask.Value()))
        {
            std::ostringstream oss;
            oss << "Invalid permissions on '" << params.filename << "' - are " << std::oct << (statbuf.st_mode & displayMask) << " should be set to "
                << std::oct << std::setw(3) << std::setfill('0') << (statbuf.st_mode & ~params.mask.Value() & displayMask) << " or a more restrictive value";
            return indicators.NonCompliant(oss.str());
        }

        OsConfigLogDebug(log, "%s mask is correct", params.filename.c_str());
        std::ostringstream oss;
        oss << params.filename << " mask matches expected mask " << std::oct << params.mask.Value();
        indicators.Compliant(oss.str());
    }

    OsConfigLogDebug(log, "File '%s' has correct permissions and ownership, as expected", params.filename.c_str());
    return indicators.Compliant("File '" + params.filename + "' has correct permissions and ownership as expected");
}

Result<Status> RemediateEnsureFilePermissions(const EnsureFilePermissionsParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto log = context.GetLogHandle();
    struct stat statbuf;
    if (stat(params.filename.c_str(), &statbuf) < 0)
    {
        const int status = errno;
        if (ENOENT == status)
        {
            OsConfigLogDebug(log, "File '%s' does not exist", params.filename.c_str());
            return indicators.NonCompliant("File '" + params.filename + "' does not exist");
        }

        OsConfigLogError(log, "Stat error %s (%d)", strerror(status), status);
        OSConfigTelemetryStatusTrace("stat", status);
        return Error("Stat error '" + std::string(strerror(status)) + "'", status);
    }

    uid_t uid = statbuf.st_uid;
    gid_t gid = statbuf.st_gid;
    bool ownership_changed = false;
    if (params.owner.HasValue())
    {
        if (params.owner->items.empty())
        {
            return Error("Empty list of owners provided", EINVAL);
        }

        bool ownerOk = false;
        const struct passwd* pwd = getpwuid(statbuf.st_uid);
        for (const auto& owner : params.owner->items)
        {
            if ((nullptr != pwd) && (owner.GetPattern() == pwd->pw_name))
            {
                OsConfigLogDebug(log, "Matched owner '%s' to '%s'", params.owner->ToString().c_str(), pwd->pw_name);
                ownerOk = true;
                break;
            }
        }
        if (!ownerOk)
        {
            const auto& firstOwner = params.owner->items.front();
            pwd = getpwnam(firstOwner.GetPattern().c_str());
            if (pwd == nullptr)
            {
                OsConfigLogDebug(log, "No user with name %s", firstOwner.GetPattern().c_str());
                return indicators.NonCompliant("No user with name " + firstOwner.GetPattern());
            }
            uid = pwd->pw_uid;
            if (uid != statbuf.st_uid)
            {
                ownership_changed = true;
            }
            else
            {
                OsConfigLogDebug(log, "Matched owner '%s' to '%s'", params.owner->ToString().c_str(), pwd->pw_name);
            }
        }
    }

    if (params.group.HasValue())
    {
        if (params.group->items.empty())
        {
            return Error("Empty list of groups provided", EINVAL);
        }

        const struct group* grp = getgrgid(statbuf.st_gid);
        bool groupOk = false;
        for (const auto& group : params.group->items)
        {
            if ((nullptr != grp) && (group.GetPattern() == grp->gr_name))
            {
                OsConfigLogDebug(log, "Matched group '%s' to '%s'", group.GetPattern().c_str(), grp->gr_name);
                groupOk = true;
                break;
            }
        }
        if (!groupOk)
        {
            const auto& firstGroup = params.group->items.front();
            grp = getgrnam(firstGroup.GetPattern().c_str());
            if (grp == nullptr)
            {
                OsConfigLogDebug(log, "No group with GID %d", statbuf.st_gid);
                return indicators.NonCompliant("No group with gid " + std::to_string(statbuf.st_gid));
            }
            gid = grp->gr_gid;
            if (gid != statbuf.st_gid)
            {
                ownership_changed = true;
            }
            else
            {
                OsConfigLogDebug(log, "Matched group '%s' to '%s'", params.group->ToString().c_str(), grp->gr_name);
            }
        }
    }
    if (ownership_changed)
    {
        OsConfigLogInfo(log, "Changing owner of '%s' from %d:%d to %d:%d", params.filename.c_str(), statbuf.st_uid, statbuf.st_gid, uid, gid);
        if (0 != chown(params.filename.c_str(), uid, gid))
        {
            int status = errno;
            OsConfigLogError(log, "Chown error %s (%d)", strerror(status), status);
            OSConfigTelemetryStatusTrace("chown", status);
            return Error(std::string("Chown error: ") + strerror(status), status);
        }

        indicators.Compliant(params.filename + " owner changed to " + std::to_string(uid) + ":" + std::to_string(gid));
    }

    unsigned short new_perms = statbuf.st_mode;
    if (params.permissions.HasValue())
    {
        new_perms |= params.permissions.Value();
    }

    if (params.mask.HasValue())
    {
        new_perms &= ~params.mask.Value();
    }
    // Sanity check - we can't have bits set in the mask and permissions at the same time.
    if ((params.permissions.HasValue() && params.mask.HasValue()) && (0 != (params.permissions.Value() & params.mask.Value())))
    {
        OsConfigLogError(log, "Invalid permissions and mask - same bits set in both");
        OSConfigTelemetryStatusTrace("permissions", EINVAL);
        return Error("Invalid permissions and mask - same bits set in both", EINVAL);
    }
    if (new_perms != statbuf.st_mode)
    {
        OsConfigLogInfo(log, "Changing permissions of '%s' from %o to %o", params.filename.c_str(), statbuf.st_mode, new_perms);
        if (chmod(params.filename.c_str(), new_perms) < 0)
        {
            int status = errno;
            OsConfigLogError(log, "Chmod error %s (%d)", strerror(status), status);
            OSConfigTelemetryStatusTrace("chmod", status);
            return Error(std::string("Chmod error: ") + strerror(status), status);
        }

        indicators.Compliant(params.filename + " permissions changed to " + std::to_string(new_perms));
    }

    OsConfigLogDebug(log, "File '%s' remediation succeeded", params.filename.c_str());
    return Status::Compliant;
}

Result<Status> AuditEnsureFilePermissionsCollection(const EnsureFilePermissionsCollectionParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    return EnsureFilePermissionsCollectionHelper(params, indicators, context, false);
}

Result<Status> RemediateEnsureFilePermissionsCollection(const EnsureFilePermissionsCollectionParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    return EnsureFilePermissionsCollectionHelper(params, indicators, context, true);
}

} // namespace ComplianceEngine
