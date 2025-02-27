// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
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

AUDIT_FN(ensureFilePermissions)
{
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

    if (args.find("user") != args.end())
    {
        struct passwd* pwd = getpwuid(statbuf.st_uid);
        if (nullptr == pwd)
        {
            logstream << "No user with uid " << statbuf.st_uid;
            return false;
        }
        if (args["user"] != pwd->pw_name)
        {
            logstream << "Invalid user - is " << pwd->pw_name << " should be " << args["user"];
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
        if (args["group"] != grp->gr_name)
        {
            logstream << "Invalid group - is " << grp->gr_name << " should be " << args["group"];
            return false;
        }
    }

    unsigned short perms = 0xFFF;
    unsigned short mask = 0xFFF;
    bool has_perms_or_mask = false;
    if (args.find("permissions") != args.end())
    {
        char* endptr;
        perms = strtol(args["permissions"].c_str(), &endptr, 8);
        if ('\0' != *endptr)
        {
            logstream << "Invalid permissions: " << args["permissions"];
            return false;
        }
        has_perms_or_mask = true;
    }
    if (args.find("mask") != args.end())
    {
        char* endptr;
        mask = strtol(args["mask"].c_str(), &endptr, 8);
        if ('\0' != *endptr)
        {
            logstream << "Invalid permissions mask: " << args["mask"];
            return false;
        }
        has_perms_or_mask = true;
    }
    if (has_perms_or_mask && ((perms & mask) != (statbuf.st_mode & mask)))
    {
        logstream << "Invalid permissions - are " << std::oct << statbuf.st_mode << " should be " << std::oct << perms << " with mask " << std::oct
                  << mask << std::dec;
        return false;
    }
    return true;
}

REMEDIATE_FN(ensureFilePermissions)
{
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
    if (args.find("user") != args.end())
    {
        struct passwd* pwd = getpwnam(args["user"].c_str());
        if (pwd == nullptr)
        {
            logstream << "ERROR: No user with name " << args["user"];
            return Error("No user with name " + args["user"]);
        }
        uid = pwd->pw_uid;
        owner_changed = true;
    }
    if (args.find("group") != args.end())
    {
        struct group* grp = getgrnam(args["group"].c_str());
        if (grp == nullptr)
        {
            logstream << "ERROR: No group with name " << args["group"];
            return Error("No group with name " + args["group"]);
        }
        gid = grp->gr_gid;
        owner_changed = true;
    }
    if (owner_changed)
    {
        if (0 != chown(args["filename"].c_str(), uid, gid))
        {
            logstream << "ERROR: Chown error " << strerror(errno);
            return Error("Chown error");
        }
    }

    unsigned short perms = 0xFFF;
    unsigned short mask = 0xFFF;
    bool has_perms_or_mask = false;
    if (args.find("permissions") != args.end())
    {
        char* endptr;
        perms = strtol(args["permissions"].c_str(), &endptr, 8);
        if ('\0' != *endptr)
        {
            logstream << "ERROR: Invalid permissions: " << args["permissions"];
            return Error("Invalid permissions: " + args["permissions"]);
        }
        has_perms_or_mask = true;
    }
    if (args.find("mask") != args.end())
    {
        char* endptr;
        mask = strtol(args["mask"].c_str(), &endptr, 8);
        if ('\0' != *endptr)
        {
            logstream << "ERROR: Invalid permissions mask: " << args["mask"];
            return Error("Invalid permissions mask: " + args["mask"]);
        }
        has_perms_or_mask = true;
    }
    unsigned short new_perms = (statbuf.st_mode & ~mask) | (perms & mask);
    if (has_perms_or_mask && (new_perms != statbuf.st_mode))
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
