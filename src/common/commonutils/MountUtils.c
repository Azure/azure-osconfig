// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

static bool MountTypeMatches(const char* entryType, const char* mountType)
{
    const char* token = entryType;
    size_t length = 0;
    size_t mountTypeLength = 0;

    if ((NULL == entryType) || (NULL == mountType))
    {
        return false;
    }

    mountTypeLength = strlen(mountType);
    while ('\0' != *token)
    {
        length = strcspn(token, ",");
        if (((length == mountTypeLength) && (0 == strncmp(token, mountType, length))) ||
            ((0 == strcmp(mountType, "nfs")) && (4 == length) && (0 == strncmp(token, "nfs4", length))))
        {
            return true;
        }

        token += length;
        if (',' == *token)
        {
            token++;
        }
    }

    return false;
}

static bool IsMountEntrySafe(const struct mntent* entry, const char* source, int lineNumber, OsConfigLogHandle log)
{
    const char* fields[] = {entry->mnt_fsname, entry->mnt_dir, entry->mnt_type, entry->mnt_opts};
    size_t i = 0;

    for (i = 0; i < ARRAY_SIZE(fields); i++)
    {
        // The first three fields are required, but the options field is optional.
        if ((NULL == fields[i]) || ((i < 3) && ('\0' == fields[i][0])) || (NULL != strpbrk(fields[i], "\n\r")))
        {
            OsConfigLogInfo(log, "SetFileSystemMountingOption: skipping invalid mount entry in '%s' at line %d (field %zu)",
                source, lineNumber, i);
            return false;
        }
    }

    return true;
}

static bool MountDirectoryMatches(const char* entryDirectory, const char* mountDirectory)
{
    size_t entryLength = strlen(entryDirectory);
    size_t directoryLength = 0;

    if (NULL == mountDirectory)
    {
        return false;
    }

    directoryLength = strlen(mountDirectory);
    while ((1 < entryLength) && ('/' == entryDirectory[entryLength - 1]))
    {
        entryLength--;
    }
    while ((1 < directoryLength) && ('/' == mountDirectory[directoryLength - 1]))
    {
        directoryLength--;
    }

    return ((entryLength == directoryLength) && (0 == strncmp(entryDirectory, mountDirectory, entryLength)));
}

static bool CanImportMountEntry(const struct mntent* entry, const char* source, int lineNumber, OsConfigLogHandle log)
{
    const char* userOptions[] = {"user", "users", "owner", "group", "user_id", "group_id", "uhelper", "helper"};
    size_t i = 0;

    if (0 == strncmp(entry->mnt_type, "fuse", 4))
    {
        OsConfigLogInfo(log, "SetFileSystemMountingOption: not importing FUSE mount from '%s' at line %d", source, lineNumber);
        return false;
    }

    for (i = 0; i < ARRAY_SIZE(userOptions); i++)
    {
        if (NULL != hasmntopt(entry, userOptions[i]))
        {
            OsConfigLogInfo(log, "SetFileSystemMountingOption: not importing user mount from '%s' at line %d", source, lineNumber);
            return false;
        }
    }

    return true;
}

static int FormatMountEntry(const struct mntent* entry, const char* desiredOption, char** line, OsConfigLogHandle log)
{
    struct mntent updated = *entry;
    FILE* stream = NULL;
    char* options = NULL;
    size_t length = 0;
    int status = 0;

    *line = NULL;
    if ('\0' == updated.mnt_opts[0])
    {
        updated.mnt_opts = "defaults";
    }
    if (NULL != desiredOption)
    {
        if (NULL == (options = FormatAllocateString("%s,%s", updated.mnt_opts, desiredOption)))
        {
            OsConfigLogError(log, "FormatMountEntry: out of memory");
            OSConfigTelemetryStatusTrace("FormatAllocateString", ENOMEM);
            return ENOMEM;
        }
        updated.mnt_opts = options;
    }

    // Escape fields before their first serialization; reparsing raw text later is too late.
    if (NULL == (stream = open_memstream(line, &length)))
    {
        status = (0 == errno) ? ENOMEM : errno;
        OsConfigLogError(log, "FormatMountEntry: open_memstream failed (%d)", status);
        OSConfigTelemetryStatusTrace("open_memstream", status);
    }
    else
    {
        if (0 != addmntent(stream, &updated))
        {
            status = (0 == errno) ? EIO : errno;
            OsConfigLogError(log, "FormatMountEntry: addmntent failed (%d)", status);
            OSConfigTelemetryStatusTrace("addmntent", status);
        }

        if (0 != fclose(stream))
        {
            int closeStatus = (0 == errno) ? EIO : errno;
            OsConfigLogError(log, "FormatMountEntry: fclose failed (%d)", closeStatus);
            OSConfigTelemetryStatusTrace("fclose", closeStatus);
            if (0 == status)
            {
                status = closeStatus;
            }
        }
    }

    if (0 != status)
    {
        FREE_MEMORY(*line);
    }
    FREE_MEMORY(options);
    return status;
}

// The mntent.mnt_opts field (which corresponds to the mount options in /etc/fstab) can technically contain credentials.
// For certain network filesystems like CIFS/SMB or NFS, one can specify options such as: 'username', 'password', 'domain'.
// These options are parsed by the mount helper (e.g., mount.cifs) when mounting the share.
// When such credentials appear in the mntent.mnt_opts field we do not want to log contents of mntent.mnt_opts.
char* CheckForMountCreds(char* options)
{
    const char* anyOf[] = {"username", "password", "domain"};
    size_t size = ARRAY_SIZE(anyOf);
    size_t i = 0;
    char* result = NULL;

    if (NULL != options)
    {
        result = options;

        for (i = 0; i < size; i++)
        {
            if (NULL != strstr(options, anyOf[i]))
            {
                result = NULL;
                break;
            }
        }
    }

    return result;
}

int CheckFileSystemMountingOption(const char* mountFileName, const char* mountDirectory, const char* mountType, const char* desiredOption, char** reason, OsConfigLogHandle log)
{
    FILE* mountFileHandle = NULL;
    struct mntent* mountStruct = NULL;
    bool matchFound = false;
    int lineNumber = 1;
    int status = 0;

    if ((NULL == mountFileName) || ((NULL == mountDirectory) && (NULL == mountType)) || (NULL == desiredOption))
    {
        OsConfigLogError(log, "CheckFileSystemMountingOption called with invalid argument(s)");
        OSConfigTelemetryStatusTrace("mountFileName", EINVAL);
        return EINVAL;
    }

    if (!FileExists(mountFileName))
    {
        OsConfigLogInfo(log, "CheckFileSystemMountingOption: file '%s' not found, nothing to check", mountFileName);
        if (OsConfigIsSuccessReason(reason))
        {
            OsConfigCaptureSuccessReason(reason, "'%s' is not found, nothing to check", mountFileName);
        }
        else
        {
            OsConfigCaptureReason(reason, "'%s' is not found", mountFileName);
        }
        return 0;
    }

    if (NULL != (mountFileHandle = setmntent(mountFileName, "r")))
    {
        while (NULL != (mountStruct = getmntent(mountFileHandle)))
        {
            if (((NULL != mountDirectory) && (NULL != mountStruct->mnt_dir) && (NULL != strstr(mountStruct->mnt_dir, mountDirectory))) ||
                MountTypeMatches(mountStruct->mnt_type, mountType))
            {
                matchFound = true;

                if (NULL != hasmntopt(mountStruct, desiredOption))
                {
                    OsConfigLogInfo(log, "CheckFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' found in '%s' at line %d ('%s')",
                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName, lineNumber, CheckForMountCreds(mountStruct->mnt_opts));

                    if (NULL != mountDirectory)
                    {
                        OsConfigCaptureSuccessReason(reason, "Option '%s' for mount directory '%s' found in '%s' at line %d ('%s')",
                            desiredOption, mountDirectory, mountFileName, lineNumber, CheckForMountCreds(mountStruct->mnt_opts));
                    }

                    if (NULL != mountType)
                    {
                        OsConfigCaptureSuccessReason(reason, "Option '%s' for mount type '%s' found in '%s' at line %d ('%s')",
                            desiredOption, mountType, mountFileName, lineNumber, CheckForMountCreds(mountStruct->mnt_opts));
                    }
                }
                else
                {
                    status = ENOENT;
                    OsConfigLogInfo(log, "CheckFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' missing from file '%s' at line %d ('%s')",
                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName, lineNumber, CheckForMountCreds(mountStruct->mnt_opts));

                    if (NULL != mountDirectory)
                    {
                        OsConfigCaptureReason(reason, "Option '%s' for mount directory '%s' is missing from file '%s' at line %d ('%s')",
                            desiredOption, mountDirectory, mountFileName, lineNumber, CheckForMountCreds(mountStruct->mnt_opts));
                    }

                    if (NULL != mountType)
                    {
                        OsConfigCaptureReason(reason, "Option '%s' for mount type '%s' missing from file '%s' at line %d ('%s')",
                            desiredOption, mountType, mountFileName, lineNumber, CheckForMountCreds(mountStruct->mnt_opts));
                    }
                }

                OsConfigLogDebug(log, "CheckFileSystemMountingOption, line %d in '%s': mnt_fsname '%s', mnt_dir '%s', mnt_type '%s', mnt_opts '%s', mnt_freq %d, mnt_passno %d",
                    lineNumber, mountFileName, mountStruct->mnt_fsname, mountStruct->mnt_dir, mountStruct->mnt_type, CheckForMountCreds(mountStruct->mnt_opts),
                    mountStruct->mnt_freq, mountStruct->mnt_passno);
            }

            lineNumber += 1;
        }

        if (false == matchFound)
        {
            status = 0;
            OsConfigLogInfo(log, "CheckFileSystemMountingOption: mount directory '%s' and/or mount type '%s' not found in '%s'",
                mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName);

            if (NULL != mountDirectory)
            {
                OsConfigCaptureSuccessReason(reason, "Found no entries about mount directory '%s' in '%s' to look for option '%s'", mountDirectory, mountFileName, desiredOption);
            }

            if (NULL != mountType)
            {
                OsConfigCaptureSuccessReason(reason, "Found no entries about mount type '%s' in '%s' to look for option '%s'", mountType, mountFileName, desiredOption);
            }
        }

        endmntent(mountFileHandle);
    }
    else
    {
        status = (0 == errno) ? ENOENT : errno;
        OsConfigLogInfo(log, "CheckFileSystemMountingOption: cannot open file '%s', setmntent() failed (%d, errno: %d)", mountFileName, status, errno);
        OsConfigCaptureReason(reason, "Cannot access '%s', setmntent() failed (%d)", mountFileName, status);
    }

    return status;
}

int CopyMountTableFile(const char* source, const char* target, OsConfigLogHandle log)
{
    FILE* sourceHandle = NULL;
    FILE* targetHandle = NULL;
    struct mntent* mountStruct = NULL;
    int status = 0;

    if ((NULL == source) || (NULL == target))
    {
        OsConfigLogError(log, "CopyMountTableFile called with invalid argument(s)");
        OSConfigTelemetryStatusTrace("source", EINVAL);
        return EINVAL;
    }

    if (!FileExists(source))
    {
        OsConfigLogInfo(log, "CopyMountTableFile: file '%s' not found", source);
        return EINVAL;
    }

    if (NULL != (targetHandle = setmntent(target, "w")))
    {
        if (NULL != (sourceHandle = setmntent(source, "r")))
        {
            while (NULL != (mountStruct = getmntent(sourceHandle)))
            {
                if (0 != (status = addmntent(targetHandle, mountStruct)))
                {
                    OsConfigLogInfo(log, "CopyMountTableFile ('%s' to '%s'): failed adding '%s %s %s %s %d %d', addmntent() failed with %d (errno: %d)", source, target,
                        mountStruct->mnt_fsname, mountStruct->mnt_dir, mountStruct->mnt_type, CheckForMountCreds(mountStruct->mnt_opts), mountStruct->mnt_freq, mountStruct->mnt_passno, status, errno);
                    break;
                }
            }
        }
        else
        {
            status = (0 == errno) ? ENOENT : errno;
            OsConfigLogInfo(log, "CopyMountTableFile: could not open source file '%s', setmntent() failed (%d)", source, status);
        }

        fflush(targetHandle);
        endmntent(targetHandle);
        endmntent(sourceHandle);
    }
    else
    {
        status = (0 == errno) ? ENOENT : errno;
        OsConfigLogInfo(log, "CopyMountTableFile: could not open target file '%s', setmntent() failed (%d)", target, status);
    }

    return status;
}

int LineAlreadyExistsInFile(const char* fileName, const char* text, OsConfigLogHandle log)
{
    char* contents = NULL;
    const char* line = NULL;
    const char* end = NULL;
    size_t textLength = 0;
    size_t lineLength = 0;
    int status = 0;

    if ((NULL == fileName) || (NULL == text) || (0 == strlen(text)))
    {
        OsConfigLogError(log, "LineAlreadyExistsInFile called with invalid argument(s)");
        return EINVAL;
    }
    else if (false == FileExists(fileName))
    {
        OsConfigLogInfo(log, "LineAlreadyExistsInFile: '%s' does not exist", fileName);
        return ENOENT;
    }

    if (NULL == (contents = LoadStringFromFile(fileName, false, log)))
    {
        status = EACCES;
        OsConfigLogInfo(log, "LineAlreadyExistsInFile: cannot read '%s'", fileName);
    }
    else
    {
        textLength = strlen(text);
        if ('\n' == text[textLength - 1])
        {
            textLength--;
        }

        status = EEXIST;
        line = contents;
        while ('\0' != *line)
        {
            end = strchr(line, '\n');
            lineLength = (NULL == end) ? strlen(line) : (size_t)(end - line);
            if ((lineLength == textLength) && (0 == strncmp(line, text, textLength)))
            {
                status = 0;
                break;
            }
            if (NULL == end)
            {
                break;
            }
            line = end + 1;
        }

        if (EEXIST == status)
        {
            OsConfigLogInfo(log, "LineAlreadyExistsInFile: entry not found in '%s'", fileName);
        }
        FREE_MEMORY(contents);
    }

    return status;
}

static int CopyMountTableComments(const char* source, const char* target, OsConfigLogHandle log)
{
    FILE* sourceHandle = NULL;
    FILE* targetHandle = NULL;
    char* line = NULL;
    char* content = NULL;
    size_t capacity = 0;
    ssize_t length = 0;
    int status = 0;

    if (NULL == (sourceHandle = fopen(source, "r")))
    {
        status = (0 == errno) ? EIO : errno;
        OsConfigLogInfo(log, "CopyMountTableComments: cannot open '%s' (%d)", source, status);
    }
    else if (NULL == (targetHandle = fopen(target, "w")))
    {
        status = (0 == errno) ? EIO : errno;
        OsConfigLogInfo(log, "CopyMountTableComments: cannot open '%s' (%d)", target, status);
    }
    else
    {
        while (-1 != (length = getline(&line, &capacity, sourceHandle)))
        {
            content = line;
            while (isspace((unsigned char)*content))
            {
                content++;
            }
            if (('#' != *content) && ('\0' != *content))
            {
                continue;
            }
            if (length != (ssize_t)fwrite(line, 1, (size_t)length, targetHandle))
            {
                status = (0 == errno) ? EIO : errno;
                OsConfigLogInfo(log, "CopyMountTableComments: cannot write '%s' (%d)", target, status);
                break;
            }
        }
        if ((0 == status) && !feof(sourceHandle))
        {
            status = (0 == errno) ? EIO : errno;
            OsConfigLogInfo(log, "CopyMountTableComments: cannot read '%s' (%d)", source, status);
        }
    }

    if ((NULL != targetHandle) && (0 != fclose(targetHandle)))
    {
        int closeStatus = (0 == errno) ? EIO : errno;
        OsConfigLogInfo(log, "CopyMountTableComments: cannot close '%s' (%d)", target, closeStatus);
        if (0 == status)
        {
            status = closeStatus;
        }
    }
    if ((NULL != sourceHandle) && (0 != fclose(sourceHandle)))
    {
        int closeStatus = (0 == errno) ? EIO : errno;
        OsConfigLogInfo(log, "CopyMountTableComments: cannot close '%s' (%d)", source, closeStatus);
        if (0 == status)
        {
            status = closeStatus;
        }
    }
    FREE_MEMORY(line);
    return status;
}

int SetFileSystemMountingOption(const char* mountFileName, const char* mountTableFileName, const char* mountDirectory, const char* mountType, const char* desiredOption, OsConfigLogHandle log)
{
    const char* fsMountTable = mountFileName;
    const char* mountTable = mountTableFileName;
    const char* tempFileNameTemplate = "%s/~xtab%d";
    const char* mountAll = "mount -a";

    FILE* fsMountHandle = NULL;
    FILE* mountHandle = NULL;
    char* newLine = NULL;
    char* tempFileNameOne = NULL;
    char* tempFileNameTwo = NULL;
    char* tempFileNameThree = NULL;
    char* fsMountTableCopy = NULL;
    char* fsMountDirectory = NULL;
    struct mntent* mountStruct = NULL;
    bool matchFound = false;
    int lineNumber = 1;
    int status = 0;

    if ((NULL == mountFileName) || (NULL == mountTableFileName) || ((NULL == mountDirectory) && (NULL == mountType)) ||
        (NULL == desiredOption) || ('\0' == *desiredOption) || (NULL != strpbrk(desiredOption, "\n\r")))
    {
        OsConfigLogError(log, "SetFileSystemMountingOption called with invalid argument(s)");
        OSConfigTelemetryStatusTrace("mountDirectory", EINVAL);
        return EINVAL;
    }

    if (!FileExists(fsMountTable))
    {
        OsConfigLogInfo(log, "SetFileSystemMountingOption: '%s' not found, no place to set mounting options", fsMountTable);
        return 0;
    }

    // Keep the scratch files next to the mount table so the final rename stays on the same filesystem
    if (NULL != (fsMountTableCopy = DuplicateString(fsMountTable)))
    {
        fsMountDirectory = dirname(fsMountTableCopy);
    }

    if ((NULL == (tempFileNameOne = FormatAllocateString(tempFileNameTemplate, fsMountDirectory ? fsMountDirectory : "/etc", 1))) ||
        (NULL == (tempFileNameTwo = FormatAllocateString(tempFileNameTemplate, fsMountDirectory ? fsMountDirectory : "/etc", 2))) ||
        (NULL == (tempFileNameThree = FormatAllocateString(tempFileNameTemplate, fsMountDirectory ? fsMountDirectory : "/etc", 3))))
    {
        OsConfigLogError(log, "SetFileSystemMountingOption: out of memory");
        OSConfigTelemetryStatusTrace("FormatAllocateString", ENOMEM);
        status = ENOMEM;
    }

    if (0 == status)
    {
        if (NULL != (fsMountHandle = setmntent(fsMountTable, "r")))
        {
            OsConfigLogInfo(log, "SetFileSystemMountingOption: looking for entries with mount directory '%s' or mount type '%s' in '%s'",
                mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", fsMountTable);

            while (NULL != (mountStruct = getmntent(fsMountHandle)))
            {
                if (!IsMountEntrySafe(mountStruct, fsMountTable, lineNumber, log))
                {
                    lineNumber++;
                    continue;
                }
                if (((NULL != mountDirectory) && (NULL != strstr(mountStruct->mnt_dir, mountDirectory))) ||
                    MountTypeMatches(mountStruct->mnt_type, mountType))
                {
                    matchFound = true;

                    if (NULL != hasmntopt(mountStruct, desiredOption))
                    {
                        OsConfigLogInfo(log, "SetFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' already set in '%s' at line %d ('%s')",
                            desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", fsMountTable, lineNumber, CheckForMountCreds(mountStruct->mnt_opts));

                        // The option is found, copy this mount entry as-is
                        FREE_MEMORY(newLine);
                        status = FormatMountEntry(mountStruct, NULL, &newLine, log);
                    }
                    else
                    {
                        OsConfigLogInfo(log, "SetFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' missing from file '%s' at line %d ('%s')",
                            desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", fsMountTable, lineNumber, CheckForMountCreds(mountStruct->mnt_opts));

                        // The option is not found and is needed for this entry, add the needed option when copying this mount entry
                        FREE_MEMORY(newLine);
                        status = FormatMountEntry(mountStruct, desiredOption, &newLine, log);
                    }

                    if (0 == status)
                    {
                        if ((ENOENT == (status = LineAlreadyExistsInFile(tempFileNameOne, newLine, log))) || (EEXIST == status))
                        {
                            if (0 != (status = AppendPayloadToFile(tempFileNameOne, newLine, (const int)strlen(newLine), log) ? 0 : ENOENT))
                            {
                                OsConfigLogInfo(log, "SetFileSystemMountingOption: cannot collect entries from '%s'", fsMountTable);
                                break;
                            }
                        }
                        else if (0 != status)
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    // No match for this mount entry, copy the entire entry as-is
                    FREE_MEMORY(newLine);
                    if (0 == (status = FormatMountEntry(mountStruct, NULL, &newLine, log)))
                    {
                        if ((ENOENT == (status = LineAlreadyExistsInFile(tempFileNameOne, newLine, log))) || (EEXIST == status))
                        {
                            if (0 != (status = AppendPayloadToFile(tempFileNameOne, newLine, (const int)strlen(newLine), log) ? 0 : ENOENT))
                            {
                                OsConfigLogInfo(log, "SetFileSystemMountingOption: cannot copy existing entries from '%s'", fsMountTable);
                                break;
                            }
                        }
                        else if (0 != status)
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                lineNumber += 1;
            }

            endmntent(fsMountHandle);

            if ((0 == status) && (false == matchFound))
            {
                OsConfigLogInfo(log, "SetFileSystemMountingOption: mount directory '%s' and/or mount type '%s' not found in '%s'",
                    mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", fsMountTable);

                // Import only eligible live mounts; directory selectors must match the mount point exactly.
                if (FileExists(mountTable))
                {
                    if (NULL != (mountHandle = setmntent(mountTable, "r")))
                    {
                        lineNumber = 1;
                        OsConfigLogInfo(log, "SetFileSystemMountingOption: looking for entries with mount directory '%s' or mount type '%s' in '%s'",
                            mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountTable);

                        while (NULL != (mountStruct = getmntent(mountHandle)))
                        {
                            if (!IsMountEntrySafe(mountStruct, mountTable, lineNumber, log) ||
                                !CanImportMountEntry(mountStruct, mountTable, lineNumber, log))
                            {
                                lineNumber++;
                                continue;
                            }
                            if (MountDirectoryMatches(mountStruct->mnt_dir, mountDirectory) ||
                                MountTypeMatches(mountStruct->mnt_type, mountType))
                            {
                                matchFound = true;

                                if (NULL != hasmntopt(mountStruct, desiredOption))
                                {
                                    OsConfigLogInfo(log, "SetFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' found set in '%s' at line %d ('%s')",
                                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountTable, lineNumber, CheckForMountCreds(mountStruct->mnt_opts));

                                    // Copy this mount entry as-is
                                    FREE_MEMORY(newLine);
                                    status = FormatMountEntry(mountStruct, NULL, &newLine, log);
                                }
                                else
                                {
                                    OsConfigLogInfo(log, "SetFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' found missing from '%s' at line %d ('%s')",
                                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountTable, lineNumber, CheckForMountCreds(mountStruct->mnt_opts));

                                    // The option is not found and is needed for this entry, add it when copying this entry
                                    FREE_MEMORY(newLine);
                                    status = FormatMountEntry(mountStruct, desiredOption, &newLine, log);
                                }

                                if (0 == status)
                                {
                                    if ((ENOENT == (status = LineAlreadyExistsInFile(tempFileNameOne, newLine, log))) || (EEXIST == status))
                                    {
                                        if (0 != (status = AppendPayloadToFile(tempFileNameOne, newLine, (const int)strlen(newLine), log) ? 0 : ENOENT))
                                        {
                                            OsConfigLogInfo(log, "SetFileSystemMountingOption: cannot collect entry from '%s'", mountTable);
                                            break;
                                        }
                                    }
                                    else if (0 != status)
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }

                            lineNumber += 1;
                        }

                        endmntent(mountHandle);
                    }
                    else
                    {
                        status = (0 == errno) ? ENOENT : errno;
                        OsConfigLogInfo(log, "SetFileSystemMountingOption: cannot open '%s', setmntent() failed (%d)", mountTable, status);
                    }
                }
            }

            if ((0 == status) && (false == matchFound))
            {
                OsConfigLogInfo(log, "SetFileSystemMountingOption: mount directory '%s' and/or mount type '%s' not found in either '%s' or '%s', nothing to remediate",
                    mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", fsMountTable, mountTable);
            }
        }
        else
        {
            status = (0 == errno) ? ENOENT : errno;
            OsConfigLogInfo(log, "SetFileSystemMountingOption: cannot open '%s', setmntent() failed (%d)", fsMountTable, status);
        }

        if (matchFound && (0 == status))
        {
            // Normalize the collected, already escaped mount entries.
            if ((0 == (status = CopyMountTableFile(tempFileNameOne, tempFileNameTwo, log))) &&
                (0 == (status = CopyMountTableComments(fsMountTable, tempFileNameThree, log))))
            {
                if (!ConcatenateFiles(tempFileNameThree, tempFileNameTwo, false, log))
                {
                    status = EIO;
                    OsConfigLogInfo(log, "SetFileSystemMountingOption: cannot combine comments and mount entries");
                }
                // Replace the table only after assembling comments and encoded mount entries.
                else if (0 == (status = RenameFileWithOwnerAndAccess(tempFileNameThree, fsMountTable, log)))
                {
                    // Only apply the system table; alternate files must not mount the host's filesystems.
                    if (0 == strcmp(fsMountTable, "/etc/fstab"))
                    {
                        // Command may fail when one configured mount point is not present, so ignore failures
                        ExecuteCommand(NULL, mountAll, false, false, 0, 0, NULL, NULL, log);
                    }
                }
                else
                {
                    OsConfigLogInfo(log, "SetFileSystemMountingOption: RenameFileWithOwnerAndAccess('%s' to '%s') returned %d", tempFileNameThree, fsMountTable, status);
                }
            }
        }

        remove(tempFileNameOne);
        remove(tempFileNameTwo);
        remove(tempFileNameThree);
    }

    FREE_MEMORY(newLine);
    FREE_MEMORY(tempFileNameOne);
    FREE_MEMORY(tempFileNameTwo);
    FREE_MEMORY(tempFileNameThree);
    FREE_MEMORY(fsMountTableCopy);

    return status;
}
