// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Internal.h"

bool FileExists(const char* fileName)
{
    return ((NULL != fileName) && (-1 != access(fileName, F_OK))) ? true : false;
}

static bool LockUnlockFile(FILE* file, bool lock, OsConfigLogHandle log)
{
    int fileDescriptor = -1;
    int lockResult = -1;
    int lockOperation = lock ? (LOCK_EX | LOCK_NB) : LOCK_UN;

    if (NULL == file)
    {
        return false;
    }

    if (-1 == (fileDescriptor = fileno(file)))
    {
        OsConfigLogInfo(log, "LockFile: fileno failed with %d", errno);
    }
    else if (0 != (lockResult = flock(fileDescriptor, lockOperation)))
    {
        OsConfigLogInfo(log, "LockFile: flock(%d) failed with %d", lockOperation, errno);
    }

    return (0 == lockResult) ? true : false;
}

bool LockFile(FILE* file, OsConfigLogHandle log)
{
    return LockUnlockFile(file, true, log);
}

bool UnlockFile(FILE* file, OsConfigLogHandle log)
{
    return LockUnlockFile(file, false, log);
}

char* LoadStringFromFile(const char* fileName, bool stopAtEol, OsConfigLogHandle log)
{
    const int initialSize = 1024;
    int currentSize = 0;
    FILE* file = NULL;
    int i = 0;
    int next = 0;
    char* string = NULL;
    char* temp = NULL;

    if (false == FileExists(fileName))
    {
        return string;
    }

    if (NULL != (file = fopen(fileName, "r")))
    {
        if (LockFile(file, log))
        {
            if (NULL != (string = (char*)malloc(initialSize)))
            {
                currentSize = initialSize;
                memset(&string[0], 0, currentSize);

                while (1)
                {
                    next = fgetc(file);
                    if ((EOF == next) || (stopAtEol && (EOL == next)))
                    {
                        string[i] = 0;
                        break;
                    }

                    string[i] = (char)next;
                    i += 1;

                    if (i >= currentSize)
                    {
                        currentSize += initialSize;
                        if (NULL != (temp = (char*)realloc(string, currentSize)))
                        {
                            string = temp;
                            memset(&string[i], 0, currentSize - i);
                        }
                        else
                        {
                            FREE_MEMORY(string);
                            break;
                        }
                    }
                }
            }

            UnlockFile(file, log);
        }

        fclose(file);
    }

    return string;
}

int RestrictFileAccessToCurrentAccountOnly(const char* fileName)
{
    if (NULL == fileName)
    {
        return EINVAL;
    }

    // S_IRUSR (0400): Read permission, owner
    // S_IWUSR (0200): Write permission, owner
    // S_IRGRP (0040): Read permission, group
    // S_IWGRP (0020): Write permission, group.

    return chmod(fileName, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
}

bool DirectoryExists(const char* fileName)
{
    DIR* directory = NULL;
    bool result = false;

    if (FileExists(fileName) && (NULL != (directory = opendir(fileName))))
    {
        closedir(directory);
        result = true;
    }

    return result;
}

static int CheckAccess(bool directory, const char* name, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, char** reason, OsConfigLogHandle log)
{
    struct stat statStruct = {0};
    mode_t currentMode = 0;
    mode_t desiredMode = 0;
    int result = ENOENT;

    if (NULL == name)
    {
        OsConfigLogError(log, "CheckAccess called with an invalid name argument");
        OSConfigTelemetryStatusTrace("name", EINVAL);
        return EINVAL;
    }

    if (directory ? DirectoryExists(name) : FileExists(name))
    {
        if (0 == (result = stat(name, &statStruct)))
        {
            if (((-1 != desiredOwnerId) && (((uid_t)desiredOwnerId != statStruct.st_uid))) ||
                ((-1 != desiredGroupId) && (((gid_t)desiredGroupId != statStruct.st_gid))))
            {
                OsConfigLogInfo(log, "CheckAccess: ownership of '%s' (%d, %d) does not match expected (%d, %d)",
                    name, statStruct.st_uid, statStruct.st_gid, desiredOwnerId, desiredGroupId);
                OsConfigCaptureReason(reason, "Ownership of '%s' (%d, %d) does not match expected (%d, %d)",
                    name, statStruct.st_uid, statStruct.st_gid, desiredOwnerId, desiredGroupId);
                result = ENOENT;
            }
            else
            {
                if (NULL != log)
                {
                    OsConfigLogInfo(log, "CheckAccess: ownership of '%s' (%d, %d) matches expected (%d, %d)",
                        name, statStruct.st_uid, statStruct.st_gid, desiredOwnerId, desiredGroupId);
                }

                currentMode = statStruct.st_mode & 07777;
                desiredMode = desiredAccess & 07777;

                if (!directory)
                {
                    desiredMode &= ~S_ISVTX;
                }

                if (currentMode != desiredMode)
                {
                    OsConfigLogInfo(log, "CheckAccess: access to '%s' (0%04o) does not match expected (0%04o)", name, currentMode, desiredMode);
                    OsConfigCaptureReason(reason, "Access to '%s' (0%04o) does not match expected (0%04o)", name, currentMode, desiredMode);
                    result = ENOENT;
                }
                else
                {
                    if (NULL != log)
                    {
                        OsConfigLogInfo(log, "CheckAccess: access to '%s' (0%04o) matches expected (0%04o)", name, currentMode, desiredMode);
                    }

                    OsConfigCaptureSuccessReason(reason, "'%s' has required access (0%04o) and ownership (uid: %d, gid: %u)", name, desiredMode, desiredOwnerId, desiredGroupId);
                    result = 0;
                }
            }
        }
        else
        {
            OsConfigLogInfo(log, "CheckAccess: stat('%s') failed with %d", name, errno);
        }
    }
    else
    {
        OsConfigLogInfo(log, "CheckAccess: '%s' is not found, nothing to check", name);
        if (OsConfigIsSuccessReason(reason))
        {
            OsConfigCaptureSuccessReason(reason, "'%s' is not found, nothing to check", name);
        }
        else
        {
            OsConfigCaptureReason(reason, "'%s' is not found", name);
        }
        result = 0;
    }

    return result;
}

static int SetAccess(bool directory, const char* name, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, OsConfigLogHandle log)
{
    int result = ENOENT;

    if (NULL == name)
    {
        OsConfigLogError(log, "SetAccess called with an invalid name argument");
        OSConfigTelemetryStatusTrace("name", EINVAL);
        return EINVAL;
    }

    if (directory ? DirectoryExists(name) : FileExists(name))
    {
        if (0 == CheckAccess(directory, name, desiredOwnerId, desiredGroupId, desiredAccess, NULL, log))
        {
            OsConfigLogInfo(log, "SetAccess: desired '%s' ownership (owner %u, group %u with access 0%04o) already set",
                name, desiredOwnerId, desiredGroupId, desiredAccess);
            result = 0;
        }
        else
        {
            if (0 == (result = chown(name, (uid_t)desiredOwnerId, (gid_t)desiredGroupId)))
            {
                OsConfigLogInfo(log, "SetAccess: successfully set ownership of '%s' to owner %u, group %u", name, desiredOwnerId, desiredGroupId);

                if (0 == (result = chmod(name, desiredAccess)))
                {
                    OsConfigLogInfo(log, "SetAccess: successfully set access to '%s' to 0%04o", name, desiredAccess);
                }
                else
                {
                    result = errno ? errno : ENOENT;
                    OsConfigLogInfo(log, "SetAccess: 'chmod 0%04o %s' failed with %d", desiredAccess, name, result);
                }
            }
            else
            {
                OsConfigLogInfo(log, "SetAccess: chown('%s', %d, %d) failed with %d", name, desiredOwnerId, desiredGroupId, errno);
            }
        }
    }
    else
    {
        OsConfigLogInfo(log, "SetAccess: '%s' not found, nothing to set", name);
        result = 0;
    }

    return result;
}

int CheckFileAccess(const char* fileName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, char** reason, OsConfigLogHandle log)
{
    return CheckAccess(false, fileName, desiredOwnerId, desiredGroupId, desiredAccess, reason, log);
}

int SetFileAccess(const char* fileName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, OsConfigLogHandle log)
{
    return SetAccess(false, fileName, desiredOwnerId, desiredGroupId, desiredAccess, log);
}
