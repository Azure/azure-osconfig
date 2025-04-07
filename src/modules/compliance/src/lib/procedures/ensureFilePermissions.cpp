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
    UNUSED(log);
    struct stat statbuf;
    if (args.find("filename") == args.end())
    {
        return Error("No filename provided");
    }
    logstream << "ensureFilePermissions for '" << args["filename"] << "' ";

    if (0 != stat(args["filename"].c_str(), &statbuf))
    {
        logstream << "Stat error '" << strerror(errno) << "'";
        return false;
    }

    if (args.find("owner") != args.end())
    {
        struct passwd* pwd = getpwuid(statbuf.st_uid);
        if (nullptr == pwd)
        {
            logstream << "No user with uid " << statbuf.st_uid;
            return false;
        }
        if (args["owner"] != pwd->pw_name)
        {
            logstream << "Invalid owner - is " << pwd->pw_name << " should be " << args["owner"];
            return false;
        }
    }

    if (args.find("group") != args.end())
    {
        struct group* grp = getgrgid(statbuf.st_gid);
        if (nullptr == grp)
        {
            logstream << "No group with gid " << statbuf.st_gid;
            return false;
        }
        std::istringstream iss(args["group"]);
        std::string group;
        bool groupOk = false;
        while (std::getline(iss, group, '|'))
        {
            if (group == grp->gr_name)
            {
                groupOk = true;
                break;
            }
        }
        if (!groupOk)
        {
            logstream << "Invalid group - is '" << grp->gr_name << "' should be '" << args["group"] << "' ";
            return false;
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
            logstream << "Invalid permissions: " << args["permissions"];
            return false;
        }
        has_permissions = true;
    }
    if (args.find("mask") != args.end())
    {
        char* endptr = nullptr;
        mask = strtol(args["mask"].c_str(), &endptr, 8);
        if ('\0' != *endptr)
        {
            logstream << "Invalid permissions mask: " << args["mask"];
            return false;
        }
        has_mask = true;
    }
    if ((has_permissions && has_mask) && (0 != (perms & mask)))
    {
        logstream << "ERROR: Invalid permissions and mask - same bits set in both";
        return Error("Invalid permissions and mask - same bits set in both");
    }
    if (has_permissions && (perms != (statbuf.st_mode & perms)))
    {
        logstream << "Invalid permissions - are " << std::oct << statbuf.st_mode << " should be at least " << std::oct << perms;
        return false;
    }
    if (has_mask && (0 != (statbuf.st_mode & mask)))
    {
        logstream << "Invalid permissions - are " << std::oct << statbuf.st_mode << " while " << std::oct << mask << " should not be set";
        return false;
    }
    return true;
}

REMEDIATE_FN(EnsureFilePermissions, "filename:Path to the file:M", "owner:Required owner of the file", "group:Required group of the file",
    "permissions:Required octal permissions of the file::^[0-7]{3,4}$", "mask:Required octal permissions of the file - mask::^[0-7]{3,4}$")
{
    UNUSED(log);
    if (args.find("filename") == args.end())
    {
        logstream << "ERROR: No filename provided";
        return Error("No filename provided");
    }

    struct stat statbuf;
    if (stat(args["filename"].c_str(), &statbuf) < 0)
    {
        logstream << "ERROR: Stat error " << strerror(errno);
        return false;
    }

    uid_t uid = statbuf.st_uid;
    gid_t gid = statbuf.st_gid;
    bool owner_changed = false;
    if (args.find("owner") != args.end())
    {
        struct passwd* pwd = getpwnam(args["owner"].c_str());
        if (pwd == nullptr)
        {
            logstream << "ERROR: No user with name " << args["owner"];
            return false;
        }
        uid = pwd->pw_uid;
        owner_changed = true;
    }
    if (args.find("group") != args.end())
    {
        struct group* grp = getgrgid(statbuf.st_gid);
        if (nullptr == grp)
        {
            logstream << "ERROR: No group with gid " << statbuf.st_gid;
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
                groupOk = true;
                break;
            }
        }
        if (!groupOk)
        {
            struct group* grp = getgrnam(firstGroup.c_str());
            if (grp == nullptr)
            {
                logstream << "ERROR: No group with name " << args["group"];
                return Error("No group with name " + args["group"]);
            }
            gid = grp->gr_gid;
            owner_changed = true;
        }
    }
    if (owner_changed)
    {
        if (0 != chown(args["filename"].c_str(), uid, gid))
        {
            logstream << "ERROR: Chown error " << strerror(errno);
            return Error("Chown error");
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
            logstream << "ERROR: Invalid permissions: " << args["permissions"];
            return Error("Invalid permissions: " + args["permissions"]);
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
            logstream << "ERROR: Invalid permissions mask: " << args["mask"];
            return Error("Invalid permissions mask: " + args["mask"]);
        }
        new_perms &= ~mask;
        has_mask = true;
    }
    // Sanity check - we can't have bits set in the mask and permissions at the same time.
    if ((has_permissions && has_mask) && (0 != (perms & mask)))
    {
        logstream << "ERROR: Invalid permissions and mask - same bits set in both";
        return Error("Invalid permissions and mask - same bits set in both");
    }
    if (new_perms != statbuf.st_mode)
    {
        if (chmod(args["filename"].c_str(), new_perms) < 0)
        {
            logstream << "ERROR: Chmod error";
            return Error("Chmod error");
        }
    }

    return true;
}
} // namespace compliance
