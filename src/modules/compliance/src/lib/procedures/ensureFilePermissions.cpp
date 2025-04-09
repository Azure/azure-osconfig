// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace compliance
{

AUDIT_FN(EnsureFilePermissions, "filename:Path to the file:M", "owner:Required owner of the file", "group:Required group of the file",
    "permissions:Required octal permissions of the file::^[0-7]{3,4}$", "mask:Required octal permissions of the file - mask::^[0-7]{3,4}$")
{
    struct stat statbuf;
    if (args.find("filename") == args.end())
    {
        OsConfigLogError(log, "No filename provided");
        return Error("No filename provided", EINVAL);
    }
    const auto filename = args["filename"];

    if (0 != stat(args["filename"].c_str(), &statbuf))
    {
        const int status = errno;
        if (ENOENT == status)
        {
            OsConfigLogDebug(log, "File '%s' does not exist", args["filename"].c_str());
            logstream << "File '" << args["filename"] << "' does not exist";
            return false;
        }

        OsConfigLogError(log, "Stat error %s (%d)", strerror(status), status);
        return Error(std::string("Stat error '") + strerror(status) + "'", status);
    }

    if (args.find("owner") != args.end())
    {
        const struct passwd* pwd = getpwuid(statbuf.st_uid);
        if (nullptr == pwd)
        {
            OsConfigLogDebug(log, "No user with UID %d", statbuf.st_gid);
            logstream << "Invalid '" << filename << "' owner: " << statbuf.st_gid << " - no such user";
            return false;
        }
        if (args["owner"] != pwd->pw_name)
        {
            OsConfigLogDebug(log, "Invalid '%s' owner - is '%s' should be '%s'", filename.c_str(), pwd->pw_name, args["owner"].c_str());
            logstream << "Invalid '" << filename << "' owner - is '" << pwd->pw_name << "' should be '" << args["owner"] << "' ";
            return false;
        }
        else
        {
            OsConfigLogDebug(log, "Matched owner '%s' to '%s'", args["owner"].c_str(), pwd->pw_name);
        }
    }

    if (args.find("group") != args.end())
    {
        struct group* grp = getgrgid(statbuf.st_gid);
        if (nullptr == grp)
        {
            OsConfigLogDebug(log, "No group with GID %d", statbuf.st_gid);
            logstream << "Invalid '" << filename << "' group: " << statbuf.st_gid << " - no such group";
            return false;
        }
        std::istringstream iss(args["group"]);
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
            logstream << "Invalid '" << filename << "' group - is '" << grp->gr_name << "' should be '" << args["group"] << "' ";
            return false;
        }
        else
        {
            OsConfigLogDebug(log, "Matched group '%s' to '%s'", args["group"].c_str(), grp->gr_name);
        }
    }

    bool has_permissions = false;
    bool has_mask = false;
    unsigned short perms = 0;
    unsigned short mask = 0;
    if (args.find("permissions") != args.end())
    {
        char* endptr = nullptr;
        perms = strtol(args["permissions"].c_str(), &endptr, 8);
        if ('\0' != *endptr)
        {
            OsConfigLogError(log, "Invalid permissions: %s", args["permissions"].c_str());
            return Error("Invalid permissions argument: " + args["permissions"], EINVAL);
        }
        has_permissions = true;
    }
    if (args.find("mask") != args.end())
    {
        char* endptr = nullptr;
        mask = strtol(args["mask"].c_str(), &endptr, 8);
        if ('\0' != *endptr)
        {
            OsConfigLogError(log, "Invalid mask argument: %s", args["mask"].c_str());
            return Error("Invalid mask argument: " + args["mask"], EINVAL);
        }
        has_mask = true;
    }
    if ((has_permissions && has_mask) && (0 != (perms & mask)))
    {
        OsConfigLogError(log, "Invalid permissions and mask - same bits set in both");
        return Error("Invalid permissions and mask - same bits set in both");
    }
    const mode_t displayMask = 07777;
    if (has_permissions)
    {
        if (perms != (statbuf.st_mode & perms))
        {
            logstream << "Invalid '" << filename << "' permissions - are " << std::oct << (statbuf.st_mode & displayMask) << " should be at least "
                      << std::oct << perms;
            return false;
        }
        else
        {
            OsConfigLogDebug(log, "Permissions are correct");
        }
    }
    if (has_mask)
    {
        if (0 != (statbuf.st_mode & mask))
        {
            logstream << "Invalid '" << filename << "' permissions - are " << std::oct << (statbuf.st_mode & displayMask) << " while " << std::oct
                      << mask << " should not be set";
            return false;
        }
        else
        {
            OsConfigLogDebug(log, "Mask is correct");
        }
    }

    OsConfigLogDebug(log, "File '%s' has correct permissions", filename.c_str());
    logstream << "File '" << filename << "' has correct permissions";
    return true;
}

REMEDIATE_FN(EnsureFilePermissions, "filename:Path to the file:M", "owner:Required owner of the file", "group:Required group of the file",
    "permissions:Required octal permissions of the file::^[0-7]{3,4}$", "mask:Required octal permissions of the file - mask::^[0-7]{3,4}$")
{
    if (args.find("filename") == args.end())
    {
        OsConfigLogError(log, "No filename provided");
        return Error("No filename provided", EINVAL);
    }
    const auto filename = args["filename"];

    struct stat statbuf;
    if (stat(args["filename"].c_str(), &statbuf) < 0)
    {
        const int status = errno;
        if (ENOENT == status)
        {
            OsConfigLogDebug(log, "File '%s' does not exist", filename.c_str());
            logstream << "File '" << filename << "' does not exist";
            return false;
        }

        OsConfigLogError(log, "Stat error %s (%d)", strerror(status), status);
        return Error(std::string("Stat error '") + strerror(status) + "'", status);
    }

    uid_t uid = statbuf.st_uid;
    gid_t gid = statbuf.st_gid;
    bool owner_changed = false;
    if (args.find("owner") != args.end())
    {
        const struct passwd* pwd = getpwnam(args["owner"].c_str());
        if (pwd == nullptr)
        {
            OsConfigLogDebug(log, "No user with UID %d", statbuf.st_gid);
            logstream << "Invalid '" << filename << "' owner: " << statbuf.st_gid << " - no such user";
            return false;
        }
        uid = pwd->pw_uid;
        if (uid != statbuf.st_uid)
        {
            owner_changed = true;
        }
        else
        {
            OsConfigLogDebug(log, "Matched owner '%s' to '%s'", args["owner"].c_str(), pwd->pw_name);
        }
    }
    if (args.find("group") != args.end())
    {
        const struct group* grp = getgrgid(statbuf.st_gid);
        if (nullptr == grp)
        {
            OsConfigLogDebug(log, "No group with GID %d", statbuf.st_gid);
            logstream << "Invalid '" << filename << "' group: " << statbuf.st_gid << " - no such group";
            return false;
        }
        std::istringstream iss(args["group"]);
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
            struct group* grp = getgrnam(firstGroup.c_str());
            if (grp == nullptr)
            {
                OsConfigLogDebug(log, "No group with GID %d", statbuf.st_gid);
                logstream << "Invalid '" << filename << "' group: " << statbuf.st_gid << " - no such group";
                return false;
            }
            gid = grp->gr_gid;
            if (gid != statbuf.st_gid)
            {
                owner_changed = true;
            }
            else
            {
                OsConfigLogDebug(log, "Matched group '%s' to '%s'", args["group"].c_str(), grp->gr_name);
            }
            owner_changed = true;
        }
    }
    if (owner_changed)
    {
        OsConfigLogInfo(log, "Changing owner of '%s' from %d:%d to %d:%d", args["filename"].c_str(), statbuf.st_uid, statbuf.st_gid, uid, gid);
        if (0 != chown(args["filename"].c_str(), uid, gid))
        {
            int status = errno;
            OsConfigLogError(log, "Chown error %s (%d)", strerror(status), status);
            return Error(std::string("Chown error: ") + strerror(status), status);
        }
    }

    bool has_permissions = false;
    bool has_mask = false;
    unsigned short perms = 0;
    unsigned short mask = 0;
    unsigned short new_perms = statbuf.st_mode;
    if (args.find("permissions") != args.end())
    {
        char* endptr = nullptr;
        perms = strtol(args["permissions"].c_str(), &endptr, 8);
        if ('\0' != *endptr)
        {
            OsConfigLogError(log, "Invalid permissions argument: %s", args["permissions"].c_str());
            return Error("Invalid permissions argument: " + args["permissions"]);
        }
        new_perms |= perms;
        has_permissions = true;
    }
    if (args.find("mask") != args.end())
    {
        char* endptr = nullptr;
        mask = strtol(args["mask"].c_str(), &endptr, 8);
        if ('\0' != *endptr)
        {
            OsConfigLogError(log, "Invalid mask argument: %s", args["mask"].c_str());
            return Error("Invalid mask argument: " + args["mask"], EINVAL);
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
        OsConfigLogInfo(log, "Changing permissions of '%s' from %o to %o", args["filename"].c_str(), statbuf.st_mode, new_perms);
        if (chmod(args["filename"].c_str(), new_perms) < 0)
        {
            int status = errno;
            OsConfigLogError(log, "Chmod error %s (%d)", strerror(status), status);
            return Error(std::string("Chmod error: ") + strerror(status), status);
        }
    }

    OsConfigLogDebug(log, "File '%s' remediation succeeded", filename.c_str());
    return true;
}
} // namespace compliance
