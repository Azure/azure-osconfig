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
    // S_ISUID (0x04000): Set user ID on execution
    // S_ISGID (0x02000): Set group ID on execution
    // S_IRUSR (0x00400): Read permission, owner
    // S_IWUSR (0x00200): Write permission, owner
    // S_IRGRP (0x00040): Read permission, group
    // S_IWGRP (0x00020): Write permission, group.
    // S_IXUSR (0x00100): Execute/search permission, owner
    // S_IXGRP (0x00010): Execute/search permission, group

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