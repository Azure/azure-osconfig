// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <FilePermissionsHelpers.h>
#include <Internal.h>
#include <Regex.h>
#include <dirent.h>
#include <errno.h>
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

Result<Status> AuditEnsureFilePermissionsHelper(const std::string& filename, const std::map<std::string, std::string>& args, IndicatorsTree& indicators,
    ContextInterface& context)
{
    auto log = context.GetLogHandle();
    struct stat statbuf;
    if (0 != stat(filename.c_str(), &statbuf))
    {
        const int status = errno;
        if (ENOENT == status)
        {
            OsConfigLogDebug(log, "File '%s' does not exist", filename.c_str());
            return indicators.Compliant("File '" + filename + "' does not exist");
        }

        OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "stat", status);
        OsConfigLogError(log, "Stat error %s (%d)", strerror(status), status);
        return Error("Stat error '" + std::string(strerror(status)) + "'", status);
    }

    auto it = args.find("owner");
    if (it != args.end())
    {
        auto owner = std::move(it->second);
        const struct passwd* pwd = getpwuid(statbuf.st_uid);
        if (nullptr == pwd)
        {
            OsConfigLogDebug(log, "No user with UID %d", statbuf.st_uid);
            return indicators.NonCompliant("No user with uid " + std::to_string(statbuf.st_uid));
        }
        std::istringstream iss(owner);
        std::string ownerName;
        bool ownerOk = false;
        while (std::getline(iss, ownerName, '|'))
        {
            if (ownerName == pwd->pw_name)
            {
                OsConfigLogDebug(log, "Matched owner '%s' to '%s'", ownerName.c_str(), pwd->pw_name);
                ownerOk = true;
                break;
            }
        }
        if (!ownerOk)
        {
            OsConfigLogDebug(log, "Invalid '%s' owner - is '%s' should be '%s'", filename.c_str(), pwd->pw_name, owner.c_str());
            return indicators.NonCompliant("Invalid owner on '" + filename + "' - is '" + std::string(pwd->pw_name) + "' should be '" + owner + "'");
        }
        else
        {
            OsConfigLogDebug(log, "Matched owner '%s' to '%s'", owner.c_str(), pwd->pw_name);
        }

        indicators.Compliant(filename + " owner matches expected value '" + owner + "'");
    }

    it = args.find("group");
    if (it != args.end())
    {
        auto groupName = std::move(it->second);
        struct group* grp = getgrgid(statbuf.st_gid);
        if (nullptr == grp)
        {
            OsConfigLogDebug(log, "No group with GID %d", statbuf.st_gid);
            return indicators.NonCompliant("No group with gid " + std::to_string(statbuf.st_gid));
        }
        std::istringstream iss(groupName);
        std::string group;
        bool groupOk = false;
        while (std::getline(iss, group, '|'))
        {
            if (group == grp->gr_name)
            {
                OsConfigLogDebug(log, "Matched group '%s' to '%s'", group.c_str(), grp->gr_name);
                groupOk = true;
                break;
            }
        }
        if (!groupOk)
        {
            OsConfigLogDebug(log, "Invalid group on '%s' - is '%s' should be '%s'", filename.c_str(), grp->gr_name, groupName.c_str());
            return indicators.NonCompliant("Invalid group on '" + filename + "' - is '" + std::string(grp->gr_name) + "' should be '" + groupName + "'");
        }
        else
        {
            OsConfigLogDebug(log, "Matched group '%s' to '%s'", groupName.c_str(), grp->gr_name);
        }

        indicators.Compliant(filename + " group matches expected value '" + groupName + "'");
    }

    bool has_permissions = false;
    bool has_mask = false;
    unsigned short perms = 0;
    unsigned short mask = 0;
    it = args.find("permissions");
    if (it != args.end())
    {
        auto permissions = std::move(it->second);
        char* endptr = nullptr;
        perms = strtol(permissions.c_str(), &endptr, 8);
        if ((nullptr == endptr) || ('\0' != *endptr))
        {
            OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "strtol", EINVAL);
            OsConfigLogError(log, "Invalid permissions: %s", permissions.c_str());
            return Error("Invalid permissions argument: " + permissions, EINVAL);
        }
        has_permissions = true;
    }

    it = args.find("mask");
    if (it != args.end())
    {
        auto maskArg = std::move(it->second);
        char* endptr = nullptr;
        mask = strtol(maskArg.c_str(), &endptr, 8);
        if ((nullptr == endptr) || ('\0' != *endptr))
        {
            OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "strtol", EINVAL);
            OsConfigLogError(log, "Invalid mask argument: %s", maskArg.c_str());
            return Error("Invalid mask argument: " + maskArg, EINVAL);
        }
        has_mask = true;
    }
    if ((has_permissions && has_mask) && (0 != (perms & mask)))
    {
        OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "permissions/mask", EINVAL);
        OsConfigLogError(log, "Invalid permissions and mask - same bits set in both");
        return Error("Invalid permissions and mask - same bits set in both");
    }
    if (has_permissions)
    {
        if (perms != (statbuf.st_mode & perms))
        {
            std::ostringstream oss;
            oss << "Invalid permissions on '" << filename << "' - are " << std::oct << (statbuf.st_mode & displayMask) << " should be at least "
                << std::oct << perms;
            return indicators.NonCompliant(oss.str());
        }

        OsConfigLogDebug(log, "%s permissions are correct", filename.c_str());
        std::ostringstream oss;
        oss << filename << " matches expected permissions " << std::oct << perms;
        indicators.Compliant(oss.str());
    }
    if (has_mask)
    {
        if (0 != (statbuf.st_mode & mask))
        {
            std::ostringstream oss;
            oss << "Invalid permissions on '" << filename << "' - are " << std::oct << (statbuf.st_mode & displayMask) << " should be set to "
                << std::oct << std::setw(3) << std::setfill('0') << (statbuf.st_mode & ~mask & displayMask) << " or a more restrictive value";
            return indicators.NonCompliant(oss.str());
        }

        OsConfigLogDebug(log, "%s mask is correct", filename.c_str());
        std::ostringstream oss;
        oss << filename << " mask matches expected mask " << std::oct << mask;
        indicators.Compliant(oss.str());
    }

    OsConfigLogDebug(log, "File '%s' has correct permissions", filename.c_str());
    return indicators.Compliant("File '" + filename + "' has correct permissions and ownership");
}

Result<Status> RemediateEnsureFilePermissionsHelper(const std::string& filename, const std::map<std::string, std::string>& args,
    IndicatorsTree& indicators, ContextInterface& context)
{
    auto log = context.GetLogHandle();
    struct stat statbuf;
    std::string owner, groupName;
    if (stat(filename.c_str(), &statbuf) < 0)
    {
        const int status = errno;
        if (ENOENT == status)
        {
            OsConfigLogDebug(log, "File '%s' does not exist", filename.c_str());
            return indicators.NonCompliant("File '" + filename + "' does not exist");
        }

        OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "stat", status);
        OsConfigLogError(log, "Stat error %s (%d)", strerror(status), status);
        return Error("Stat error '" + std::string(strerror(status)) + "'", status);
    }

    uid_t uid = statbuf.st_uid;
    gid_t gid = statbuf.st_gid;
    bool owner_changed = false;
    auto it = args.find("owner");
    if (it != args.end())
    {
        owner = std::move(it->second);
        std::istringstream iss(owner);
        std::string ownerName;
        std::string firstOwner;
        bool ownerOk = false;
        const struct passwd* pwd = getpwuid(statbuf.st_uid);
        while (std::getline(iss, ownerName, '|'))
        {
            if (firstOwner.empty())
            {
                firstOwner = ownerName;
            }
            if ((nullptr != pwd) && (ownerName == pwd->pw_name))
            {
                OsConfigLogDebug(log, "Matched owner '%s' to '%s'", ownerName.c_str(), pwd->pw_name);
                ownerOk = true;
                break;
            }
        }
        if (!ownerOk)
        {
            pwd = getpwnam(firstOwner.c_str());
            if (pwd == nullptr)
            {
                OsConfigLogDebug(log, "No user with name %s", firstOwner.c_str());
                return indicators.NonCompliant("No user with name " + firstOwner);
            }
            uid = pwd->pw_uid;
            if (uid != statbuf.st_uid)
            {
                owner_changed = true;
            }
            else
            {
                OsConfigLogDebug(log, "Matched owner '%s' to '%s'", owner.c_str(), pwd->pw_name);
            }
        }
    }

    it = args.find("group");
    if (it != args.end())
    {
        groupName = std::move(it->second);
        const struct group* grp = getgrgid(statbuf.st_gid);
        std::istringstream iss(groupName);
        std::string group;
        std::string firstGroup;
        bool groupOk = false;
        while (std::getline(iss, group, '|'))
        {
            if (firstGroup.empty())
            {
                firstGroup = group;
            }
            if ((nullptr != grp) && (group == grp->gr_name))
            {
                OsConfigLogDebug(log, "Matched group '%s' to '%s'", group.c_str(), grp->gr_name);
                groupOk = true;
                break;
            }
        }
        if (!groupOk)
        {
            grp = getgrnam(firstGroup.c_str());
            if (grp == nullptr)
            {
                OsConfigLogDebug(log, "No group with GID %d", statbuf.st_gid);
                return indicators.NonCompliant("No group with gid " + std::to_string(statbuf.st_gid));
            }
            gid = grp->gr_gid;
            if (gid != statbuf.st_gid)
            {
                owner_changed = true;
            }
            else
            {
                OsConfigLogDebug(log, "Matched group '%s' to '%s'", groupName.c_str(), grp->gr_name);
            }
        }
    }
    if (owner_changed)
    {
        OsConfigLogInfo(log, "Changing owner of '%s' from %d:%d to %d:%d", filename.c_str(), statbuf.st_uid, statbuf.st_gid, uid, gid);
        if (0 != chown(filename.c_str(), uid, gid))
        {
            int status = errno;
            OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "chown", status);
            OsConfigLogError(log, "Chown error %s (%d)", strerror(status), status);
            return Error(std::string("Chown error: ") + strerror(status), status);
        }

        indicators.Compliant(filename + " owner changed to " + owner + ":" + groupName);
    }

    bool has_permissions = false;
    bool has_mask = false;
    unsigned short perms = 0;
    unsigned short mask = 0;
    unsigned short new_perms = statbuf.st_mode;
    it = args.find("permissions");
    if (it != args.end())
    {
        auto permissions = std::move(it->second);
        char* endptr = nullptr;
        perms = strtol(permissions.c_str(), &endptr, 8);
        if ((nullptr == endptr) || ('\0' != *endptr))
        {
            OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "strtol", EINVAL);
            OsConfigLogError(log, "Invalid permissions argument: %s", permissions.c_str());
            return Error("Invalid permissions argument: " + permissions);
        }
        new_perms |= perms;
        has_permissions = true;
    }

    it = args.find("mask");
    if (it != args.end())
    {
        auto maskArg = std::move(it->second);
        char* endptr = nullptr;
        mask = strtol(maskArg.c_str(), &endptr, 8);
        if ((nullptr == endptr) || ('\0' != *endptr))
        {
            OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "strtol", EINVAL);
            OsConfigLogError(log, "Invalid mask argument: %s", maskArg.c_str());
            return Error("Invalid mask argument: " + maskArg, EINVAL);
        }
        new_perms &= ~mask;
        has_mask = true;
    }
    // Sanity check - we can't have bits set in the mask and permissions at the same time.
    if ((has_permissions && has_mask) && (0 != (perms & mask)))
    {
        OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "permissions/mask", EINVAL);
        OsConfigLogError(log, "Invalid permissions and mask - same bits set in both");
        return Error("Invalid permissions and mask - same bits set in both", EINVAL);
    }
    if (new_perms != statbuf.st_mode)
    {
        OsConfigLogInfo(log, "Changing permissions of '%s' from %o to %o", filename.c_str(), statbuf.st_mode, new_perms);
        if (chmod(filename.c_str(), new_perms) < 0)
        {
            int status = errno;
            OSConfigTelemetryStatusTrace(context.GetTelemetryHandle(), "chmod", status);
            OsConfigLogError(log, "Chmod error %s (%d)", strerror(status), status);
            return Error(std::string("Chmod error: ") + strerror(status), status);
        }

        indicators.Compliant(filename + " permissions changed to " + std::to_string(new_perms));
    }

    OsConfigLogDebug(log, "File '%s' remediation succeeded", filename.c_str());
    return Status::Compliant;
}
} // namespace ComplianceEngine
