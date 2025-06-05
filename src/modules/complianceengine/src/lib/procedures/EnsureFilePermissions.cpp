// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include <CommonUtils.h>
#include <Evaluator.h>
#include <FilePermissionsHelpers.h>
#include <errno.h>
#include <fnmatch.h>
#include <fts.h>
#include <iostream>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace ComplianceEngine
{

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
