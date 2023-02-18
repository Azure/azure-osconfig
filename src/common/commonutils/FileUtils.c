// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

char* LoadStringFromFile(const char* fileName, bool stopAtEol, void* log)
{
    FILE* file = NULL;
    int fileSize = 0;
    int i = 0;
    int next = 0;
    char* string = NULL;

    if ((NULL == fileName) || (-1 == access(fileName, F_OK)))
    {
        return string;
    }

    file = fopen(fileName, "r");
    if (file)
    {
        if (LockFile(file, log))
        {
            fseek(file, 0, SEEK_END);
            fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            string = (char*)malloc(fileSize + 1);
            if (string)
            {
                memset(&string[0], 0, fileSize + 1);
                for (i = 0; i < fileSize; i++)
                {
                    next = fgetc(file);
                    if ((EOF == next) || (stopAtEol && (EOL == next)))
                    {
                        string[i] = 0;
                        break;
                    }

                    string[i] = (char)next;
                }
            }

            UnlockFile(file, log);
        }

        fclose(file);
    }

    return string;
}

bool SavePayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log)
{
    FILE* file = NULL;
    int i = 0;
    bool result = false;

    if (fileName && payload && (0 < payloadSizeBytes))
    {
        file = fopen(fileName, "w");
        if (file)
        {
            result = LockFile(file, log);
            if (result)
            {
                for (i = 0; i < payloadSizeBytes; i++)
                {
                    if (payload[i] != fputc(payload[i], file))
                    {
                        result = false;
                    }
                }

                UnlockFile(file, log);
            }
            fclose(file);
        }
    }

    return result;
}

int RestrictFileAccessToCurrentAccountOnly(const char* fileName)
{
    // S_ISUID (4000): Set user ID on execution
    // S_ISGID (2000): Set group ID on execution
    // S_IRUSR (0400): Read permission, owner
    // S_IWUSR (0200): Write permission, owner
    // S_IRGRP (0040): Read permission, group
    // S_IWGRP (0020): Write permission, group.
    // S_IXUSR (0100): Execute/search permission, owner
    // S_IXGRP (0010): Execute/search permission, group

    return chmod(fileName, S_ISUID | S_ISGID | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IXUSR | S_IXGRP);
}

bool FileExists(const char* name)
{
    return ((NULL != name) && (-1 != access(name, F_OK))) ? true : false;
}

static bool LockUnlockFile(FILE* file, bool lock, void* log)
{
    int fileDescriptor = -1;
    int lockResult = -1;
    int lockOperation = lock ? (LOCK_EX | LOCK_NB) : LOCK_UN;

    if (NULL == file)
    {
        return ENOENT;
    }

    if (-1 == (fileDescriptor = fileno(file)))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "LockFile: fileno failed with %d", errno);
        }
    }
    else if (0 != (lockResult = flock(fileDescriptor, lockOperation)))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "LockFile: flock(%d) failed with %d", lockOperation, errno);
        }
    }

    return (0 == lockResult) ? true : false;
}

bool LockFile(FILE* file, void* log)
{
    return LockUnlockFile(file, true, log);
}

bool UnlockFile(FILE* file, void* log)
{
    return LockUnlockFile(file, false, log);
}

static unsigned int FilterFileAccessFlags(unsigned int mode)
{
    // S_IRWXU (00700): Read, write, execute/search by owner
    // S_IRUSR (00400): Read permission, owner
    // S_IWUSR (00200): Write permission, owner
    // S_IXUSR (00100): Execute/search permission, owner
    // S_IRWXG (00070): Read, write, execute/search by group
    // S_IRGRP (00040): Read permission, group
    // S_IWGRP (00020): Write permission, group
    // S_IXGRP (00010): Execute/search permission, group
    // S_IRWXO (00007): Read, write, execute/search by others
    // S_IROTH (00004): Read permission, others
    // S_IWOTH (00002): Write permission, others
    // S_IXOTH (00001): Execute/search permission, others
    // S_ISUID (04000): Set-user-ID on execution
    // S_ISGID (02000): Set-group-ID on execution
    // S_ISVTX (01000): On directories, restricted deletion flag

    mode_t flags = 0;

    if (mode & S_IRWXU)
    {
        flags |= S_IRWXU;
    }
    else
    {
        if (mode & S_IRUSR)
        {
            flags |= S_IRUSR;
        }

        if (mode & S_IWUSR)
        {
            flags |= S_IWUSR;
        }

        if (mode & S_IXUSR)
        {
            flags |= S_IXUSR;
        }
    }

    if (mode & S_IRWXG)
    {
        flags |= S_IRWXG;
    }
    else
    {
        if (mode & S_IRGRP)
        {
            flags |= S_IRGRP;
        }

        if (mode & S_IWGRP)
        {
            flags |= S_IWGRP;
        }

        if (mode & S_IXGRP)
        {
            flags |= S_IXGRP;
        }
    }

    if (mode & S_IRWXO)
    {
        flags |= S_IRWXO;
    }
    else
    {
        if (mode & S_IROTH)
        {
            flags |= S_IROTH;
        }

        if (mode & S_IWOTH)
        {
            flags |= S_IWOTH;
        }

        if (mode & S_IXOTH)
        {
            flags |= S_IXOTH;
        }
    }

    if (mode & S_ISUID)
    {
        flags |= S_ISUID;
    }

    if (mode & S_ISGID)
    {
        flags |= S_ISGID;
    }

    if (mode & S_ISVTX)
    {
        flags |= S_ISVTX;
    }

    return flags;
}

int CheckFileAccess(const char* fileName, unsigned int desiredUserId, unsigned int desiredGroupId, unsigned int desiredFileAccess, void* log)
{
    struct stat statStruct = {0};
    mode_t currentMode = 0;
    mode_t desiredMode = 0;
    int result = ENOENT;

    if (NULL == fileName)
    {
        OsConfigLogError(log, "CheckFileAccess called with an invalid file name argument");
        return EINVAL;
    }

    if (FileExists(fileName))
    {
        if (0 == (result = stat(fileName, &statStruct)))
        {
            if (((uid_t)desiredUserId == statStruct.st_uid) && ((gid_t)desiredGroupId == statStruct.st_gid))
            {
                currentMode = FilterFileAccessFlags(statStruct.st_mode);
                desiredMode = FilterFileAccessFlags(desiredFileAccess);

                if ((((desiredMode & S_IRWXU) == (currentMode & S_IRWXU)) || (0 == (desiredMode & S_IRWXU))) &&
                    (((desiredMode & S_IRWXG) == (currentMode & S_IRWXG)) || (0 == (desiredMode & S_IRWXG))) &&
                    (((desiredMode & S_IRWXO) == (currentMode & S_IRWXO)) || (0 == (desiredMode & S_IRWXO))))
                {
                    OsConfigLogInfo(log, "File %s (%u, %u, %u-%u) matches expected (%u, %u, %u-%u)",
                        fileName, statStruct.st_uid, statStruct.st_gid, statStruct.st_mode, currentMode,
                        desiredUserId, desiredGroupId, desiredFileAccess, desiredMode);
                    result = 0;
                }
                else
                {
                    OsConfigLogError(log, "No matching access permissions for %s (%u-%u) versus expected (%u-%u)",
                        fileName, statStruct.st_mode, currentMode, desiredFileAccess, desiredMode);
                    result = ENOENT;
                }
            }
            else
            {
                OsConfigLogError(log, "No matching ownership for %s (user: %u, group: %u) versus expected (user: %u, group: %u)",
                    fileName, statStruct.st_uid, statStruct.st_gid, desiredUserId, desiredGroupId);
                result = ENOENT;
            }
        }
        else
        {
            OsConfigLogError(log, "stat(%s) failed with %d", fileName, errno);
        }
    }
    else
    {
        OsConfigLogInfo(log, "%s not found, nothing to check", fileName);
        result = 0;
    }

    return result;
}

int SetFileAccess(const char* fileName, unsigned int desiredUserId, unsigned int desiredGroupId, unsigned int desiredFileAccess, void* log)
{
    int result = ENOENT;

    if (NULL == fileName)
    {
        OsConfigLogError(log, "SetFileAccess called with an invalid file name argument");
        return EINVAL;
    }

    if (FileExists(fileName))
    {
        if (0 == (result = CheckFileAccess(fileName, desiredUserId, desiredGroupId, desiredFileAccess, log)))
        {
            OsConfigLogInfo(log, "Desired %s ownership (user %u, group %u with access %u) already set",
                fileName, desiredUserId, desiredGroupId, desiredFileAccess);
            result = 0;
        }
        else
        {
            if (0 == (result = chown(fileName, (uid_t)desiredUserId, (gid_t)desiredGroupId)))
            {
                OsConfigLogInfo(log, "Successfully set %s ownership to user %u, group %u", fileName, desiredUserId, desiredGroupId);

                if (0 == (result = chmod(fileName, desiredFileAccess)))
                {
                    OsConfigLogInfo(log, "Successfully set %s access to %u", fileName, desiredFileAccess);
                    result = 0;
                }
                else
                {
                    OsConfigLogError(log, "chmod(%s, %d) failed with %d", fileName, desiredFileAccess, errno);
                }
            }
            else
            {
                OsConfigLogError(log, "chown(%s, %d, %d) failed with %d", fileName, desiredUserId, desiredGroupId, errno);
            }

        }
    }
    else
    {
        OsConfigLogInfo(log, "%s not found, nothing to set", fileName);
        result = 0;
    }

    return result;
}

int CheckFileSystemMountingOption(const char* mountFileName, const char* mountDirectory, const char* mountType, const char* desiredOption, void* log)
{
    FILE* mountFileHandle = NULL;
    struct mntent* mountStruct = NULL;
    int status = ENOENT;
    
    if ((NULL == mountFileName) || ((NULL == mountDirectory) && (NULL == mountType)) || (NULL == desiredOption))
    {
        OsConfigLogError(log, "CheckFileSystemMountingOption called with invalid argument(s)");
        return EINVAL;
    }

    if (NULL != (mountFileHandle = setmntent(mountFileName, "r")))
    {
        status = 0;

        while (NULL != (mountStruct = getmntent(mountFileHandle)))
        {
            OsConfigLogInfo(log, "mnt_fsname '%s', mnt_dir '%s', mnt_type '%s', mnt_opts '%s', mnt_freq %d, mnt_passno %d",
                mountStruct->mnt_fsname, mountStruct->mnt_dir, mountStruct->mnt_type, mountStruct->mnt_opts, mountStruct->mnt_freq, mountStruct->mnt_passno);

            if (((NULL != mountDirectory) && (NULL != mountStruct->mnt_dir) && (NULL != strstr(mountStruct->mnt_dir, mountDirectory))) ||
                ((NULL != mountType) && (NULL != mountStruct->mnt_type) && (NULL != strstr(mountStruct->mnt_type, mountType))))
            {
                if (NULL != hasmntopt(mountStruct, desiredOption))
                {
                    OsConfigLogInfo(log, "Option %s for directory %s or mount type %s found in file %s", 
                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName);
                }
                else
                {
                    OsConfigLogError(log, "Option %s for directory %s or mount type %s not found (missing from) in file %s",
                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName);
                    status = ENOENT;
                }
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
        
        OsConfigLogError(log, "setmntent(%s) failed (%d)", mountFileName, status);
    }

    return status;
}