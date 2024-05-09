// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

int CheckFileSystemMountingOption(const char* mountFileName, const char* mountDirectory, const char* mountType, const char* desiredOption, char** reason, void* log)
{
    FILE* mountFileHandle = NULL;
    struct mntent* mountStruct = NULL;
    bool matchFound = false;
    int lineNumber = 1;
    int status = 0;
    
    if ((NULL == mountFileName) || ((NULL == mountDirectory) && (NULL == mountType)) || (NULL == desiredOption))
    {
        OsConfigLogError(log, "CheckFileSystemMountingOption called with invalid argument(s)");
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
                ((NULL != mountType) && (NULL != mountStruct->mnt_type) && (NULL != strstr(mountStruct->mnt_type, mountType))))
            {
                matchFound = true;
                
                if (NULL != hasmntopt(mountStruct, desiredOption))
                {
                    OsConfigLogInfo(log, "CheckFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' found in '%s' at line %d ('%s')", 
                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName, lineNumber, mountStruct->mnt_opts);
                    
                    if (NULL != mountDirectory)
                    {
                        OsConfigCaptureSuccessReason(reason, "Option '%s' for mount directory '%s' found in '%s' at line %d ('%s')", 
                            desiredOption, mountDirectory, mountFileName, lineNumber, mountStruct->mnt_opts);
                    }

                    if (NULL != mountType)
                    {
                        OsConfigCaptureSuccessReason(reason, "Option '%s' for mount type '%s' found in '%s' at line %d ('%s')", 
                            desiredOption, mountType, mountFileName, lineNumber, mountStruct->mnt_opts);
                    }
                }
                else
                {
                    status = ENOENT;
                    OsConfigLogError(log, "CheckFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' missing from file '%s' at line %d ('%s')",
                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName, lineNumber, mountStruct->mnt_opts);
                    
                    if (NULL != mountDirectory)
                    {
                        OsConfigCaptureReason(reason, "Option '%s' for mount directory '%s' is missing from file '%s' at line %d ('%s')", 
                            desiredOption, mountDirectory, mountFileName, lineNumber, mountStruct->mnt_opts);
                    }

                    if (NULL != mountType)
                    {
                        OsConfigCaptureReason(reason, "Option '%s' for mount type '%s' missing from file '%s' at line %d ('%s')", 
                            desiredOption, mountType, mountFileName, lineNumber, mountStruct->mnt_opts);
                    }
                }

                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(log, "CheckFileSystemMountingOption, line %d in %s: mnt_fsname '%s', mnt_dir '%s', mnt_type '%s', mnt_opts '%s', mnt_freq %d, mnt_passno %d", 
                        lineNumber, mountFileName, mountStruct->mnt_fsname, mountStruct->mnt_dir, mountStruct->mnt_type, mountStruct->mnt_opts, 
                        mountStruct->mnt_freq, mountStruct->mnt_passno);
                }
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
        status = errno;
        
        if (0 == status)
        {
            status = ENOENT;
        }
        
        OsConfigLogError(log, "CheckFileSystemMountingOption: could not open file '%s', setmntent() failed (%d)", mountFileName, status);
        OsConfigCaptureReason(reason, "Cannot access '%s', setmntent() failed (%d)", mountFileName, status);
    }

    return status;
}

static int CopyMountTableFile(const char* source, const char* target, void* log)
{
    FILE* sourceHandle = NULL;
    FILE* targetHandle = NULL;
    struct mntent* mountStruct = NULL;
    int status = 0;

    if ((NULL == source) || (NULL == target))
    {
        OsConfigLogError(log, "CopyMountTableFile called with invalid argument(s)");
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
                if (0 != addmntent(targetHandle, mountStruct))
                {
                    if (0 == (status = errno))
                    {
                        status = ENOENT;
                    }
                    
                    OsConfigLogError(log, "CopyMountTableFile ('%s' to '%s'): failed adding '%s %s %s %s %d %d', addmntent() failed with %d", source, target, 
                        mountStruct->mnt_fsname, mountStruct->mnt_dir, mountStruct->mnt_type, mountStruct->mnt_opts, mountStruct->mnt_freq, mountStruct->mnt_passno, status);
                    break;
                }
            }
        }
        else
        {
            if (0 == (status = errno))
            {
                status = ENOENT;
            }
            
            OsConfigLogError(log, "CopyMountTableFile: could not open source file '%s', setmntent() failed (%d)", source, status);
        }
        
        fflush(targetHandle);
        endmntent(targetHandle);
        endmntent(sourceHandle);
    }
    else
    {
        if (0 == (status = errno))
        {
            status = ENOENT;
        }

        OsConfigLogError(log, "CopyMountTableFile: could not open target file '%s', setmntent() failed (%d)", target, status);
    }
        
    return status;
}

int SetFileSystemMountingOption(const char* mountDirectory, const char* mountType, const char* desiredOption, void* log)
{
    const char* fsMountTable = "/etc/fstab";
    const char* mountTable = "/etc/mtab";
    const char tempFileNameTemplate[] = "/tmp/~xtab%d";
    const char* newLineAsIsTemplate = "\n%s %s %s %s %d %d";
    const char* newLineAddNewTemplate = "\n%s %s %s %s,%s %d %d";

    FILE* fsMountHandle = NULL;
    FILE* mountHandle = NULL;
    char* newLine = NULL;
    char* tempFileNameOne = NULL;
    char* tempFileNameTwo = NULL;
    struct mntent* mountStruct = NULL;
    bool matchFound = false;
    int lineNumber = 1;
    int status = 0;

    if (((NULL == mountDirectory) && (NULL == mountType)) || (NULL == desiredOption))
    {
        OsConfigLogError(log, "SetFileSystemMountingOption called with invalid argument(s)");
        return EINVAL;
    }

    if (!FileExists(fsMountTable))
    {
        OsConfigLogInfo(log, "SetFileSystemMountingOption: '%s' not found, no place to set mounting options", fsMountTable);
        return 0;
    }

    if ((NULL == (tempFileNameOne = FormatAllocateString(tempFileNameTemplate, 1))) ||
        (NULL == (tempFileNameTwo = FormatAllocateString(tempFileNameTemplate, 2))))
    {
        OsConfigLogError(log, "SetFileSystemMountingOption: out of memory");
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
                if (((NULL != mountDirectory) && (NULL != mountStruct->mnt_dir) && (NULL != strstr(mountStruct->mnt_dir, mountDirectory))) ||
                    ((NULL != mountType) && (NULL != mountStruct->mnt_type) && (NULL != strstr(mountStruct->mnt_type, mountType))))
                {
                    matchFound = true;

                    if (NULL != hasmntopt(mountStruct, desiredOption))
                    {
                        OsConfigLogInfo(log, "SetFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' already set in '%s' at line %d ('%s')",
                            desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", fsMountTable, lineNumber, mountStruct->mnt_opts);

                        // The option is found, copy this mount entry as-is
                        FREE_MEMORY(newLine);
                        newLine = FormatAllocateString(newLineAsIsTemplate, mountStruct->mnt_fsname, mountStruct->mnt_dir, mountStruct->mnt_type,
                            mountStruct->mnt_opts, mountStruct->mnt_freq, mountStruct->mnt_passno);
                    }
                    else
                    {
                        OsConfigLogInfo(log, "SetFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' missing from file '%s' at line %d ('%s')",
                            desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", fsMountTable, lineNumber, mountStruct->mnt_opts);
                        
                        // The option is not found and is needed for this entry, add the needed option when copying this mount entry
                        FREE_MEMORY(newLine);
                        newLine = FormatAllocateString(newLineAddNewTemplate, mountStruct->mnt_fsname, mountStruct->mnt_dir, mountStruct->mnt_type,
                            mountStruct->mnt_opts, desiredOption, mountStruct->mnt_freq, mountStruct->mnt_passno);
                    }

                    if (NULL != newLine)
                    {
                        if (0 != (status = AppendToFile(tempFileNameOne, newLine, (const int)strlen(newLine), log) ? 0 : ENOENT))
                        {
                            OsConfigLogError(log, "SetFileSystemMountingOption: failed collecting entries from '%s'", fsMountTable);
                            break;
                        }
                    }
                    else
                    {
                        OsConfigLogError(log, "SetFileSystemMountingOption: out of memory");
                        status = ENOMEM;
                        break;
                    }
                }
                else
                {
                    // No match for this mount entry, copy the entire entry as-is
                    FREE_MEMORY(newLine);
                    if (NULL != (newLine = FormatAllocateString(newLineAsIsTemplate, mountStruct->mnt_fsname, mountStruct->mnt_dir, mountStruct->mnt_type,
                        mountStruct->mnt_opts, mountStruct->mnt_freq, mountStruct->mnt_passno)))
                    {
                        if (0 != (status = AppendToFile(tempFileNameOne, newLine, (const int)strlen(newLine), log) ? 0 : ENOENT))
                        {
                            OsConfigLogError(log, "SetFileSystemMountingOption: failed copying existing entries from '%s'", fsMountTable);
                            break;
                        }
                    }
                }

                lineNumber += 1;
            }

            endmntent(fsMountHandle);

            if (false == matchFound)
            {
                OsConfigLogInfo(log, "SetFileSystemMountingOption: mount directory '%s' and/or mount type '%s' not found in '%s'",
                    mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", fsMountTable);

                // No relevant mount entries found in /etc/fstab, try to find and copy entries from /etc/tab if there are any matching
                if (FileExists(mountTable))
                {
                    if (NULL != (mountHandle = setmntent(mountTable, "r")))
                    {
                        lineNumber = 1;
                        OsConfigLogInfo(log, "SetFileSystemMountingOption: looking for entries with mount directory '%s' or mount type '%s' in '%s'",
                            mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountTable);

                        while (NULL != (mountStruct = getmntent(mountHandle)))
                        {
                            if (((NULL != mountDirectory) && (NULL != mountStruct->mnt_dir) && (NULL != strstr(mountStruct->mnt_dir, mountDirectory))) ||
                                ((NULL != mountType) && (NULL != mountStruct->mnt_type) && (NULL != strstr(mountStruct->mnt_type, mountType))))
                            {
                                matchFound = true;

                                if (NULL != hasmntopt(mountStruct, desiredOption))
                                {
                                    OsConfigLogInfo(log, "SetFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' found set in '%s' at line %d ('%s')",
                                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountTable, lineNumber, mountStruct->mnt_opts);

                                    // Copy this mount entry as-is
                                    FREE_MEMORY(newLine);
                                    newLine = FormatAllocateString(newLineAsIsTemplate, mountStruct->mnt_fsname, mountStruct->mnt_dir, mountStruct->mnt_type,
                                        mountStruct->mnt_opts, mountStruct->mnt_freq, mountStruct->mnt_passno);
                                }
                                else
                                {
                                    OsConfigLogInfo(log, "SetFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' found missing from '%s' at line %d ('%s')",
                                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountTable, lineNumber, mountStruct->mnt_opts);

                                    // The option is not found and is needed for this entry, add it when copying this entry
                                    FREE_MEMORY(newLine);
                                    newLine = FormatAllocateString(newLineAddNewTemplate, mountStruct->mnt_fsname, mountStruct->mnt_dir, mountStruct->mnt_type,
                                        mountStruct->mnt_opts, desiredOption, mountStruct->mnt_freq, mountStruct->mnt_passno);
                                }

                                if (NULL != newLine)
                                {
                                    if (0 != (status = AppendToFile(tempFileNameOne, newLine, (const int)strlen(newLine), log) ? 0 : ENOENT))
                                    {
                                        OsConfigLogError(log, "SetFileSystemMountingOption: failed collecting entry from '%s'", mountTable);
                                        break;
                                    }
                                }
                                else
                                {
                                    OsConfigLogError(log, "SetFileSystemMountingOption: out of memory");
                                    status = ENOMEM;
                                    break;
                                }
                            }
                            
                            lineNumber += 1;
                        }
                        
                        endmntent(mountHandle);
                    }
                    else
                    {
                        if (0 == (status = errno))
                        {
                            status = ENOENT;
                        }
                        OsConfigLogError(log, "SetFileSystemMountingOption: could not open '%s', setmntent() failed (%d)", mountTable, status);
                    }
                }
            }

            if (false == matchFound)
            {
                OsConfigLogInfo(log, "SetFileSystemMountingOption: mount directory '%s' and/or mount type '%s' not found in either '%s' or '%s', nothing to remediate",
                    mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", fsMountTable, mountTable);
            }
        }
        else
        {
            if (0 == (status = errno))
            {
                status = ENOENT;
            }
            OsConfigLogError(log, "SetFileSystemMountingOption: could not open '%s', setmntent() failed (%d)", fsMountTable, status);
        }

        if (0 == status)
        {
            // Copy from the manually built temp mount file one to the temp mount file two using the *mntent API to ensure correct format
            if (0 == (status = CopyMountTableFile(tempFileNameOne, tempFileNameTwo, log)))
            {
                // When done assembling the final temp mount file two, move it in an atomic step to real mount file
                rename(tempFileNameTwo, fsMountTable);
            }
        }

        remove(tempFileNameOne);
        remove(tempFileNameTwo);
    }

    FREE_MEMORY(newLine);
    FREE_MEMORY(tempFileNameOne);
    FREE_MEMORY(tempFileNameTwo);

    return status;
}