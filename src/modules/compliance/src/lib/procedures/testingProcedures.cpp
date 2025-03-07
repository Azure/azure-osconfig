// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Result.h>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

namespace compliance
{
REMEDIATE_FN(remediationFailure)
{
    UNUSED(args);
    UNUSED(logstream);
    return false;
}

REMEDIATE_FN(remediationSuccess)
{
    UNUSED(args);
    UNUSED(logstream);
    return true;
}

AUDIT_FN(auditFailure)
{
    UNUSED(args);
    UNUSED(logstream);
    return false;
}

AUDIT_FN(auditSuccess)
{
    UNUSED(args);
    UNUSED(logstream);
    return true;
}

REMEDIATE_FN(remediationParametrized)
{
    UNUSED(logstream);
    auto it = args.find("result");
    if (it == args.end())
    {
        return Error("Missing 'result' parameter");
    }

    if (it->second == "success")
    {
        return true;
    }
    else if (it->second == "failure")
    {
        return false;
    }

    return Error("Invalid 'result' parameter");
}

REMEDIATE_FN(createFile)
{
    if (args.find("filename") == args.end())
    {
        logstream << "ERROR: No filename provided";
        return Error("No filename provided");
    }

    struct stat statbuf;
    if (stat(args["filename"].c_str(), &statbuf) == 0)
    {
        logstream << "ERROR: File " << args["filename"] << " already exists";
        return Error("File " + args["filename"] + " already exists");
    }

    std::ofstream file(args["filename"]);
    if (!file.is_open())
    {
        logstream << "ERROR: Failed to create file " << args["filename"];
        return Error("Failed to create file " + args["filename"]);
    }

    auto it = args.find("content");
    if (it != args.end())
    {
        file << it->second;
    }

    it = args.find("user");
    if (it != args.end())
    {
        struct passwd* pwd = getpwnam(args["user"].c_str());
        if (pwd == nullptr)
        {
            logstream << "ERROR: No user with name " << args["user"];
            return Error("No user with name " + args["user"]);
        }

        if (chown(args["filename"].c_str(), pwd->pw_uid, -1) < 0)
        {
            logstream << "ERROR: Failed to change owner of file " << args["filename"];
            return Error("Failed to change owner of file " + args["filename"]);
        }
    }

    it = args.find("group");
    if (it != args.end())
    {
        struct group* grp = getgrnam(args["group"].c_str());
        if (grp == nullptr)
        {
            logstream << "ERROR: No group with name " << args["group"];
            return Error("No group with name " + args["group"]);
        }

        if (chown(args["filename"].c_str(), -1, grp->gr_gid) < 0)
        {
            logstream << "ERROR: Failed to change group of file " << args["filename"];
            return Error("Failed to change group of file " + args["filename"]);
        }
    }

    it = args.find("permissions");
    if (it != args.end())
    {
        char* endptr = nullptr;
        unsigned long perms = strtoul(it->second.c_str(), &endptr, 8);
        if ('\0' != *endptr)
        {
            logstream << "ERROR: Invalid permissions: " << it->second;
            return Error("Invalid permissions: " + it->second);
        }

        if (chmod(args["filename"].c_str(), perms) < 0)
        {
            logstream << "ERROR: Failed to change permissions of file " << args["filename"];
            return Error("Failed to change permissions of file " + args["filename"]);
        }
    }

    return true;
}

REMEDIATE_FN(removeFile)
{
    if (args.find("filename") == args.end())
    {
        logstream << "ERROR: No filename provided";
        return Error("No filename provided");
    }

    if (remove(args["filename"].c_str()) < 0)
    {
        logstream << "ERROR: Failed to remove file " << args["filename"];
        return Error("Failed to remove file " + args["filename"]);
    }

    return true;
}
} // namespace compliance
