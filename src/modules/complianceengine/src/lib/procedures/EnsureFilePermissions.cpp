// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <fts.h>
#include <grp.h>
#include <iostream>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace ComplianceEngine
{
namespace
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
            OsConfigLogDebug(log, "No user with UID %d", statbuf.st_gid);
            return indicators.NonCompliant("No user with uid " + std::to_string(statbuf.st_uid));
        }
        if (owner != pwd->pw_name)
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
        if ('\0' != *endptr)
        {
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
        if ('\0' != *endptr)
        {
            OsConfigLogError(log, "Invalid mask argument: %s", maskArg.c_str());
            return Error("Invalid mask argument: " + maskArg, EINVAL);
        }
        has_mask = true;
    }
    if ((has_permissions && has_mask) && (0 != (perms & mask)))
    {
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
        else
        {
            OsConfigLogDebug(log, "Permissions are correct");
        }

        std::ostringstream oss;
        oss << filename << " matches expected permissions " << std::oct << perms;
        indicators.Compliant(oss.str());
    }
    if (has_mask)
    {
        if (0 != (statbuf.st_mode & mask))
        {
            std::ostringstream oss;
            oss << "Invalid permissions on '" << filename << "' - are " << std::oct << (statbuf.st_mode & displayMask) << " while " << std::oct << mask
                << " should not be set";
            return indicators.NonCompliant(oss.str());
        }
        else
        {
            OsConfigLogDebug(log, "Mask is correct");
        }
    }

    OsConfigLogDebug(log, "File '%s' has correct permissions", filename.c_str());
    std::ostringstream oss;
    oss << filename << " mask matches expected mask " << std::oct << mask;
    return indicators.Compliant(oss.str());
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
        const struct passwd* pwd = getpwnam(owner.c_str());
        if (pwd == nullptr)
        {
            OsConfigLogDebug(log, "No user with UID %d", statbuf.st_gid);
            return indicators.NonCompliant("No user with name " + owner);
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

    it = args.find("group");
    if (it != args.end())
    {
        groupName = std::move(it->second);
        const struct group* grp = getgrgid(statbuf.st_gid);
        if (nullptr == grp)
        {
            OsConfigLogDebug(log, "No group with GID %d", statbuf.st_gid);
            return indicators.NonCompliant("No group with gid " + std::to_string(statbuf.st_gid));
        }
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
            if (group == grp->gr_name)
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
        if ('\0' != *endptr)
        {
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
        if ('\0' != *endptr)
        {
            OsConfigLogError(log, "Invalid mask argument: %s", maskArg.c_str());
            return Error("Invalid mask argument: " + maskArg, EINVAL);
        }
        new_perms &= ~mask;
        has_mask = true;
    }
    // Sanity check - we can't have bits set in the mask and permissions at the same time.
    if ((has_permissions && has_mask) && (0 != (perms & mask)))
    {
        OsConfigLogError(log, "Invalid permissions and mask - same bits set in both");
        return Error("Invalid permissions and mask - same bits set in both", EINVAL);
    }
    if (new_perms != statbuf.st_mode)
    {
        OsConfigLogInfo(log, "Changing permissions of '%s' from %o to %o", filename.c_str(), statbuf.st_mode, new_perms);
        if (chmod(filename.c_str(), new_perms) < 0)
        {
            int status = errno;
            OsConfigLogError(log, "Chmod error %s (%d)", strerror(status), status);
            return Error(std::string("Chmod error: ") + strerror(status), status);
        }

        std::ostringstream oss;
        oss << std::oct << new_perms;
        indicators.Compliant(filename + " permissions changed to " + oss.str());
    }

    OsConfigLogDebug(log, "File '%s' remediation succeeded", filename.c_str());
    return Status::Compliant;
}

Result<Status> EnsureFilePermissionsCollectionHelper(const std::map<std::string, std::string>& args, IndicatorsTree& indicators,
    ContextInterface& context, bool isRemediation)
{
    auto log = context.GetLogHandle();
    auto it = args.find("directory");
    if (it == args.end())
    {
        OsConfigLogError(log, "No directory provided");
        return Error("No directory provided", EINVAL);
    }
    auto directory = std::move(it->second);
    it = args.find("ext");
    if (it == args.end())
    {
        OsConfigLogError(log, "No file pattern provided");
        return Error("No file pattern provided", EINVAL);
    }
    auto ext = std::move(it->second);

    char* const paths[] = {&directory[0], nullptr};
    FTS* ftsp = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR, NULL);

    if (!ftsp)
    {
        OsConfigLogInfo(log, "Directory '%s' does not exist", directory.c_str());
        return indicators.Compliant("Directory '" + directory + "' does not exist");
    }
    auto ftspDeleter = std::unique_ptr<FTS, int (*)(FTS*)>(ftsp, fts_close);
    bool hasFiles = false;

    FTSENT* entry = nullptr;
    while (nullptr != (entry = fts_read(ftsp)))
    {
        if (FTS_F == entry->fts_info)
        {
            if (0 == fnmatch(ext.c_str(), entry->fts_name, 0))
            {
                hasFiles = true;
                const char* fileName = entry->fts_path;

                Result<Status> result = isRemediation ? RemediateEnsureFilePermissionsHelper(fileName, args, indicators, context) :
                                                        AuditEnsureFilePermissionsHelper(fileName, args, indicators, context);
                if (!result.HasValue())
                {
                    OsConfigLogError(log, "Error processing permissions for '%s'", fileName);
                    return result;
                }
                if (Status::NonCompliant == result.Value())
                {
                    OsConfigLogError(log, "File '%s' does not match expected permissions", fileName);
                    return result;
                }
                OsConfigLogDebug(log, "File '%s' matches expected permissions", fileName);
            }
        }
    }

    if (hasFiles)
    {
        OsConfigLogDebug(log, "All matching files in '%s' match expected permissions", directory.c_str());
        return indicators.Compliant("All matching files in '" + directory + "' match expected permissions");
    }
    else
    {
        OsConfigLogDebug(log, "No files in '%s' match the pattern", directory.c_str());
        return indicators.Compliant("No files in '" + directory + "' match the pattern");
    }
}
} // namespace

AUDIT_FN(EnsureFilePermissions, "filename:Path to the file:M", "owner:Required owner of the file", "group:Required group of the file",
    "permissions:Required octal permissions of the file::^[0-7]{3,4}$", "mask:Required octal permissions of the file - mask::^[0-7]{3,4}$")
{
    auto log = context.GetLogHandle();
    auto it = args.find("filename");
    if (it == args.end())
    {
        OsConfigLogError(log, "No filename provided");
        return Error("No filename provided", EINVAL);
    }
    auto filename = std::move(it->second);
    return AuditEnsureFilePermissionsHelper(filename, args, indicators, context);
}

REMEDIATE_FN(EnsureFilePermissions, "filename:Path to the file:M", "owner:Required owner of the file", "group:Required group of the file",
    "permissions:Required octal permissions of the file::^[0-7]{3,4}$", "mask:Required octal permissions of the file - mask::^[0-7]{3,4}$")
{
    auto log = context.GetLogHandle();
    auto it = args.find("filename");
    if (it == args.end())
    {
        OsConfigLogError(log, "No filename provided");
        return Error("No filename provided", EINVAL);
    }
    auto filename = std::move(it->second);
    return RemediateEnsureFilePermissionsHelper(filename, args, indicators, context);
}

AUDIT_FN(EnsureFilePermissionsCollection, "directory:Directory path:M", "ext:File pattern:M", "owner:Required owner of the file",
    "group:Required group of the file", "permissions:Required octal permissions of the file::^[0-7]{3,4}$",
    "mask:Required octal permissions of the file - mask::^[0-7]{3,4}$")
{
    return EnsureFilePermissionsCollectionHelper(args, indicators, context, false);
}

REMEDIATE_FN(EnsureFilePermissionsCollection, "directory:Directory path:M", "ext:File pattern:M", "owner:Required owner of the file",
    "group:Required group of the file", "permissions:Required octal permissions of the file::^[0-7]{3,4}$",
    "mask:Required octal permissions of the file - mask::^[0-7]{3,4}$")
{
    return EnsureFilePermissionsCollectionHelper(args, indicators, context, true);
}

} // namespace ComplianceEngine
