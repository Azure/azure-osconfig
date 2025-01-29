// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

char* LoadStringFromFile(const char* fileName, bool stopAtEol, void* log)
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

static bool SaveToFile(const char* fileName, const char* mode, const char* payload, const int payloadSizeBytes, void* log)
{
    FILE* file = NULL;
    int i = 0;
    bool result = true;

    if (fileName && mode && payload && (0 < payloadSizeBytes))
    {
        RestrictFileAccessToCurrentAccountOnly(fileName);

        if (NULL != (file = fopen(fileName, mode)))
        {
            if (true == (result = LockFile(file, log)))
            {
                for (i = 0; i < payloadSizeBytes; i++)
                {
                    if (payload[i] != (char)fputc(payload[i], file))
                    {
                        result = false;
                        OsConfigLogError(log, "SaveToFile: failed saving '%c' to '%s' (%d)", payload[i], fileName, errno);
                    }
                }

                UnlockFile(file, log);
            }
            else
            {
                OsConfigLogError(log, "SaveToFile: cannot lock '%s' for exclusive access while writing (%d)", fileName, errno);
            }

            fflush(file);
            fclose(file);
        }
        else
        {
            result = false;
            OsConfigLogError(log, "SaveToFile: cannot open '%s' in mode '%s' (%d)", fileName, mode, errno);
        }
    }
    else
    {
        result = false;
        OsConfigLogError(log, "SaveToFile: invalid arguments ('%s', '%s', '%.*s', %d)", fileName, mode, payloadSizeBytes, payload, payloadSizeBytes);
    }

    return result;
}

bool SavePayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log)
{
    return SaveToFile(fileName, "w", payload, payloadSizeBytes, log);
}

bool FileEndsInEol(const char* fileName, void* log)
{
    struct stat statStruct = {0};
    FILE* file = NULL;
    int status = 0;
    bool result = false;

    if (0 == (status = stat(fileName, &statStruct)))
    {
        if (statStruct.st_size > 0)
        {
            if (NULL != (file = fopen(fileName, "r")))
            {
                if (0 == (status = fseek(file, -1, SEEK_END)))
                {
                    if (EOL == fgetc(file))
                    {
                        result = true;
                    }
                }
                else
                {
                    OsConfigLogError(log, "FileEndsInEol: fseek to end of '%s' failed with %d (errno: %d)", fileName, status, errno);
                }

                fclose(file);
            }
            else
            {
                OsConfigLogError(log, "FileEndsInEol: failed to open '%s' for reading", fileName);
            }
        }
    }
    else
    {
        OsConfigLogError(log, "FileEndsInEol: stat('%s') failed with %d (errno: %d)", fileName, status, errno);
    }

    return result;
}

bool AppendPayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log)
{
    bool result = false;

    if ((NULL == fileName) || (NULL == payload) || (0 >= payloadSizeBytes))
    {
        OsConfigLogError(log, "AppendPayloadToFile: invalid arguments");
        return result;
    }

    // If the file exists and there is no EOL at the end of file, try to add one before the append
    if (FileExists(fileName) && (false == FileEndsInEol(fileName, log)))
    {
        if (false == SaveToFile(fileName, "a", "\n", 1, log))
        {
            OsConfigLogError(log, "AppendPayloadToFile: failed to append EOL to '%s'", fileName);
        }
    }

    if (false == (result = SaveToFile(fileName, "a", payload, payloadSizeBytes, log)))
    {
        OsConfigLogError(log, "AppendPayloadToFile: failed to append '%.*s' to '%s'", payloadSizeBytes, payload, fileName);
    }

    return result;
}

static bool InternalSecureSaveToFile(const char* fileName, const char* mode, const char* payload, const int payloadSizeBytes, void* log)
{
    const char* tempFileNameTemplate = "%s/~OSConfig%u";
    char* fileDirectory = NULL;
    char* fileNameCopy = NULL;
    char* tempFileName = NULL;
    char* fileContents = NULL;
    unsigned int ownerId = 0;
    unsigned int groupId = 0;
    unsigned int access = 644;
    int status = 0;
    bool result = false;

    if ((NULL == fileName) || (NULL == payload) || (0 >= payloadSizeBytes))
    {
        OsConfigLogError(log, "InternalSecureSaveToFile: invalid arguments");
        return false;
    }
    else if (NULL == (fileNameCopy = DuplicateString(fileName)))
    {
        OsConfigLogError(log, "InternalSecureSaveToFile: out of memory");
        return false;
    }

    if (NULL == (fileDirectory = dirname(fileNameCopy)))
    {
        OsConfigLogInfo(log, "InternalSecureSaveToFile: no directory name for '%s' (%d)", fileNameCopy, errno);
    }

    if (DirectoryExists(fileDirectory))
    {
        if (0 == GetDirectoryAccess(fileDirectory, &ownerId, &groupId, &access, log))
        {
            OsConfigLogInfo(log, "InternalSecureSaveToFile: directory '%s' exists, is owned by user (%u, %u) and has access mode %u",
                fileDirectory, ownerId, groupId, access);
        }
    }

    if (NULL != (tempFileName = FormatAllocateString(tempFileNameTemplate, fileDirectory ? fileDirectory : "/tmp", rand())))
    {
        if ((0 == strcmp(mode, "a") && FileExists(fileName)))
        {
            if (NULL != (fileContents = LoadStringFromFile(fileName, false, log)))
            {
                if (true == (result = SaveToFile(tempFileName, "a", fileContents, strlen(fileContents), log)))
                {
                    // If there is no EOL at the end of file, add one before the append
                    if (EOL != fileContents[strlen(fileContents) - 1])
                    {
                        SaveToFile(tempFileName, "w", "\n", 1, log);
                    }

                    result = SaveToFile(tempFileName, "a", payload, payloadSizeBytes, log);
                }

                FREE_MEMORY(fileContents);
            }
            else
            {
                OsConfigLogError(log, "InternalSecureSaveToFile: failed to read from '%s'", fileName);
                result = false;
            }
        }
        else
        {
            result = SaveToFile(tempFileName, "w", payload, payloadSizeBytes, log);
        }
    }
    else
    {
        OsConfigLogError(log, "InternalSecureSaveToFile: out of memory");
        result = false;
    }

    if (result && (false == FileExists(tempFileName)))
    {
        OsConfigLogError(log, "InternalSecureSaveToFile: failed to create temporary file");
        result = false;
    }

    if (result)
    {
        if (0 != (status = RenameFileWithOwnerAndAccess(tempFileName, fileName, log)))
        {
            OsConfigLogError(log, "InternalSecureSaveToFile: RenameFileWithOwnerAndAccess('%s' to '%s') failed with %d", tempFileName, fileName, status);
            result = false;
        }

        remove(tempFileName);
    }

    FREE_MEMORY(tempFileName);
    FREE_MEMORY(fileNameCopy);

    return result;
}

bool SecureSaveToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log)
{
    return InternalSecureSaveToFile(fileName, "w", payload, payloadSizeBytes, log);
}

bool AppendToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log)
{
    return InternalSecureSaveToFile(fileName, "a", payload, payloadSizeBytes, log);
}

bool MakeFileBackupCopy(const char* fileName, const char* backupName, bool preserveAccess, void* log)
{
    char* fileContents = NULL;
    char* newFileName = NULL;
    bool result = true;

    if (fileName && backupName)
    {
        if (FileExists(fileName))
        {
            if (NULL != (fileContents = LoadStringFromFile(fileName, false, log)))
            {
                if (preserveAccess)
                {
                    result = SecureSaveToFile(backupName, fileContents, strlen(fileContents), log);
                }
                else
                {
                    result = SavePayloadToFile(backupName, fileContents, strlen(fileContents), log);
                }
            }
            else
            {
                result = false;
                OsConfigLogError(log, "MakeFileBackupCopy: failed to make a file copy of '%s'", fileName);
            }
        }
        else
        {
            result = false;
            OsConfigLogError(log, "MakeFileBackupCopy: file '%s' does not exist", fileName);
        }
    }
    else
    {
        result = false;
        OsConfigLogError(log, "MakeFileBackupCopy: invalid arguments ('%s', '%s')", fileName, backupName);
    }

    FREE_MEMORY(fileContents);
    FREE_MEMORY(newFileName);

    return result;
}

bool ConcatenateFiles(const char* firstFileName, const char* secondFileName, bool preserveAccess, void* log)
{
    char* contents = NULL;
    bool result = false;

    if ((NULL == firstFileName) || (NULL == secondFileName))
    {
        OsConfigLogError(log, "ConcatenateFiles: invalid arguments");
        return false;
    }

    if (NULL != (contents = LoadStringFromFile(secondFileName, false, log)))
    {
        if (preserveAccess)
        {
            result = AppendToFile(firstFileName, contents, strlen(contents), log);
        }
        else
        {
            result = AppendPayloadToFile(firstFileName, contents, strlen(contents), log);
        }

        FREE_MEMORY(contents);
    }

    return result;
}

int RestrictFileAccessToCurrentAccountOnly(const char* fileName)
{
    if (NULL == fileName)
    {
        return EINVAL;
    }

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

static bool IsATrueFileOrDirectory(bool directory, const char* name, void* log)
{
    struct stat statStruct = {0};
    int format = 0;
    int status = 0;
    bool result = false;

    if (NULL == name)
    {
        OsConfigLogError(log, "IsATrueFileOrDirectoryFileOrDirectory: invalid argument");
        return false;
    }

    if (-1 != (status = lstat(name, &statStruct)))
    {
        format = S_IFMT & statStruct.st_mode;

        switch (format)
        {
            case S_IFBLK:
                OsConfigLogError(log, "IsATrueFileOrDirectory: '%s' is a block device", name);
                break;

            case S_IFCHR:
                OsConfigLogError(log, "IsATrueFileOrDirectory: '%s' is a character device", name);
                break;

            case S_IFDIR:
                if (directory)
                {
                    OsConfigLogInfo(log, "IsATrueFileOrDirectory: '%s' is a directory", name);
                    result = true;
                }
                else
                {
                    OsConfigLogError(log, "IsATrueFileOrDirectory: '%s' is a directory", name);
                }
                break;

            case S_IFIFO:
                OsConfigLogError(log, "IsATrueFileOrDirectory: '%s' is a FIFO pipe", name);
                break;

            case S_IFLNK:
                OsConfigLogError(log, "IsATrueFileOrDirectory: '%s' is a symnlink", name);
                break;

            case S_IFREG:
                if (false == directory)
                {
                    OsConfigLogInfo(log, "IsATrueFileOrDirectory: '%s' is a regular file", name);
                    result = true;
                }
                else
                {
                    OsConfigLogError(log, "IsATrueFileOrDirectory: '%s' is a regular file", name);
                }
                break;

            case S_IFSOCK:
                OsConfigLogError(log, "IsATrueFileOrDirectory: '%s' is a socket", name);
                break;

            default:
                OsConfigLogError(log, "IsATrueFileOrDirectory: '%s' is of an unknown format 0x%X", name, format);
        }
    }
    else
    {
        OsConfigLogError(log, "IsATrueFileOrDirectory: stat('%s') failed with %d (errno: %d)", name, status, errno);
    }

    return result;
}

bool IsAFile(const char* fileName, void* log)
{
    return IsATrueFileOrDirectory(false, fileName, log);
}

bool IsADirectory(const char* fileName, void* log)
{
    return IsATrueFileOrDirectory(true, fileName, log);
}

bool FileExists(const char* fileName)
{
    return ((NULL != fileName) && (-1 != access(fileName, F_OK))) ? true : false;
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

int CheckFileExists(const char* fileName, char** reason, void* log)
{
    int status = 0;

    if (FileExists(fileName))
    {
        OsConfigLogInfo(log, "CheckFileExists: file '%s' exists", fileName);
        OsConfigCaptureSuccessReason(reason, "File '%s' exists", fileName);
    }
    else
    {
        OsConfigLogInfo(log, "CheckFileExists: file '%s' is not found", fileName);
        OsConfigCaptureReason(reason, "File  '%s' is not found", fileName);
        status = ENOENT;
    }

    return status;
}

int CheckFileNotFound(const char* fileName, char** reason, void* log)
{
    int status = 0;

    if (false == FileExists(fileName))
    {
        OsConfigLogInfo(log, "CheckFileNotFound: file '%s' is not found", fileName);
        OsConfigCaptureSuccessReason(reason, "File '%s' is not found", fileName);
    }
    else
    {
        OsConfigLogInfo(log, "CheckFileNotFound: file '%s' exists", fileName);
        OsConfigCaptureReason(reason, "File  '%s' exists", fileName);
        status = ENOENT;
    }

    return status;
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
        OsConfigLogError(log, "LockFile: fileno failed with %d", errno);
    }
    else if (0 != (lockResult = flock(fileDescriptor, lockOperation)))
    {
        OsConfigLogError(log, "LockFile: flock(%d) failed with %d", lockOperation, errno);
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

static int DecimalToOctal(int decimal)
{
    char buffer[10] = {0};
    snprintf(buffer, ARRAY_SIZE(buffer), "%o", decimal);
    return atoi(buffer);
}

static int OctalToDecimal(int octal)
{
    int internalOctal = octal;
    int decimal = 0;
    int i = 0;

    while (internalOctal)
    {
        decimal += (internalOctal % 10) * pow(8, i++);
        internalOctal = internalOctal / 10;
    }

    return decimal;
}

static int CheckAccess(bool directory, const char* name, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, bool rootCanOverwriteOwnership, char** reason, void* log)
{
    struct stat statStruct = {0};
    mode_t currentMode = 0;
    mode_t desiredMode = 0;
    int result = ENOENT;

    if (NULL == name)
    {
        OsConfigLogError(log, "CheckAccess called with an invalid name argument");
        return EINVAL;
    }

    if (directory ? DirectoryExists(name) : FileExists(name))
    {
        if (0 == (result = stat(name, &statStruct)))
        {
            if (((-1 != desiredOwnerId) && (((uid_t)desiredOwnerId != statStruct.st_uid) && (directory && rootCanOverwriteOwnership && ((0 != statStruct.st_uid))))) ||
                ((-1 != desiredGroupId) && (((gid_t)desiredGroupId != statStruct.st_gid) && (directory && rootCanOverwriteOwnership && ((0 != statStruct.st_gid))))))
            {
                OsConfigLogError(log, "CheckAccess: ownership of '%s' (%d, %d) does not match expected (%d, %d)",
                    name, statStruct.st_uid, statStruct.st_gid, desiredOwnerId, desiredGroupId);
                OsConfigCaptureReason(reason, "Ownership of '%s' (%d, %d) does not match expected (%d, %d)",
                    name, statStruct.st_uid, statStruct.st_gid, desiredOwnerId, desiredGroupId);
                result = ENOENT;
            }
            else
            {
                // Special case for the MPI Client
                if (NULL != log)
                {
                    OsConfigLogInfo(log, "CheckAccess: ownership of '%s' (%d, %d) matches expected (%d, %d)",
                        name, statStruct.st_uid, statStruct.st_gid, desiredOwnerId, desiredGroupId);
                }

                // S_IXOTH (00001): Execute/search permission, others
                // S_IWOTH (00002): Write permission, others
                // S_IROTH (00004): Read permission, others
                // S_IRWXO (00007): Read, write, execute/search by others
                // S_IXGRP (00010): Execute/search permission, group
                // S_IWGRP (00020): Write permission, group
                // S_IRGRP (00040): Read permission, group
                // S_IRWXG (00070): Read, write, execute/search by group
                // S_IXUSR (00100): Execute/search permission, owner
                // S_IWUSR (00200): Write permission, owner
                // S_IRUSR (00400): Read permission, owner
                // S_IRWXU (00700): Read, write, execute/search by owner
                // S_ISVTX (01000): On directories, restricted deletion flag
                // S_ISGID (02000): Set-group-ID on execution
                // S_ISUID (04000): Set-user-ID on execution

                currentMode = DecimalToOctal(statStruct.st_mode & 07777);
                desiredMode = desiredAccess;

                if (((desiredMode & S_IRWXU) && ((desiredMode & S_IRWXU) != (currentMode & S_IRWXU))) ||
                    ((desiredMode & S_IRWXG) && ((desiredMode & S_IRWXG) != (currentMode & S_IRWXG))) ||
                    ((desiredMode & S_IRWXO) && ((desiredMode & S_IRWXO) != (currentMode & S_IRWXO))) ||
                    ((desiredMode & S_IRUSR) && ((desiredMode & S_IRUSR) != (currentMode & S_IRUSR))) ||
                    ((desiredMode & S_IRGRP) && ((desiredMode & S_IRGRP) != (currentMode & S_IRGRP))) ||
                    ((desiredMode & S_IROTH) && ((desiredMode & S_IROTH) != (currentMode & S_IROTH))) ||
                    ((desiredMode & S_IWUSR) && ((desiredMode & S_IWUSR) != (currentMode & S_IWUSR))) ||
                    ((desiredMode & S_IWGRP) && ((desiredMode & S_IWGRP) != (currentMode & S_IWGRP))) ||
                    ((desiredMode & S_IWOTH) && ((desiredMode & S_IWOTH) != (currentMode & S_IWOTH))) ||
                    ((desiredMode & S_IXUSR) && ((desiredMode & S_IXUSR) != (currentMode & S_IXUSR))) ||
                    ((desiredMode & S_IXGRP) && ((desiredMode & S_IXGRP) != (currentMode & S_IXGRP))) ||
                    ((desiredMode & S_IXOTH) && ((desiredMode & S_IXOTH) != (currentMode & S_IXOTH))) ||
                    ((desiredMode & S_ISUID) && ((desiredMode & S_ISUID) != (currentMode & S_ISUID))) ||
                    ((desiredMode & S_ISGID) && ((desiredMode & S_ISGID) != (currentMode & S_ISGID))) ||
                    (directory && (desiredMode & S_ISVTX) && ((desiredMode & S_ISVTX) != (currentMode & S_ISVTX))) ||
                    (currentMode > desiredMode))
                {
                    OsConfigLogError(log, "CheckAccess: access to '%s' (%d) does not match expected (%d)", name, currentMode, desiredMode);
                    OsConfigCaptureReason(reason, "Access to '%s' (%d) does not match expected (%d)", name, currentMode, desiredMode);
                    result = ENOENT;
                }
                else
                {
                    // Special case for the MPI Client
                    if (NULL != log)
                    {
                        OsConfigLogInfo(log, "CheckAccess: access to '%s' (%d) matches expected (%d)", name, currentMode, desiredMode);
                    }

                    OsConfigCaptureSuccessReason(reason, "'%s' has required access (%d) and ownership (uid: %d, gid: %u)", name, desiredMode, desiredOwnerId, desiredGroupId);
                    result = 0;
                }
            }
        }
        else
        {
            OsConfigLogError(log, "CheckAccess: stat('%s') failed with %d", name, errno);
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

static int SetAccess(bool directory, const char* name, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, void* log)
{
    mode_t mode = OctalToDecimal(desiredAccess);
    int result = ENOENT;

    if (NULL == name)
    {
        OsConfigLogError(log, "SetAccess called with an invalid name argument");
        return EINVAL;
    }

    if (directory ? DirectoryExists(name) : FileExists(name))
    {
        if (0 == (result = CheckAccess(directory, name, desiredOwnerId, desiredGroupId, desiredAccess, false, NULL, log)))
        {
            OsConfigLogInfo(log, "SetAccess: desired '%s' ownership (owner %u, group %u with access %u) already set",
                name, desiredOwnerId, desiredGroupId, desiredAccess);
            result = 0;
        }
        else
        {
            if (0 == (result = chown(name, (uid_t)desiredOwnerId, (gid_t)desiredGroupId)))
            {
                OsConfigLogInfo(log, "SetAccess: successfully set ownership of '%s' to owner %u, group %u", name, desiredOwnerId, desiredGroupId);

                if (0 == (result = chmod(name, mode)))
                {
                    OsConfigLogInfo(log, "SetAccess: successfully set access to '%s' to %u", name, desiredAccess);
                }
                else
                {
                    result = errno ? errno : ENOENT;
                    OsConfigLogError(log, "SetAccess: 'chmod %d %s' failed with %d", desiredAccess, name, result);
                }
            }
            else
            {
                OsConfigLogError(log, "SetAccess: chown('%s', %d, %d) failed with %d", name, desiredOwnerId, desiredGroupId, errno);
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

int CheckFileAccess(const char* fileName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, char** reason, void* log)
{
    return CheckAccess(false, fileName, desiredOwnerId, desiredGroupId, desiredAccess, false, reason, log);
}

int SetFileAccess(const char* fileName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, void* log)
{
    return SetAccess(false, fileName, desiredOwnerId, desiredGroupId, desiredAccess, log);
}

int CheckDirectoryAccess(const char* directoryName, int desiredOwnerId, int desiredGroupId, unsigned int desiredAccess, bool rootCanOverwriteOwnership, char** reason, void* log)
{
    return CheckAccess(true, directoryName, desiredOwnerId, desiredGroupId, desiredAccess, rootCanOverwriteOwnership, reason, log);
}

int SetDirectoryAccess(const char* directoryName, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, void* log)
{
    return SetAccess(true, directoryName, desiredOwnerId, desiredGroupId, desiredAccess, log);
}

static unsigned int GetNumberOfCharacterInstancesInFile(const char* fileName, char what)
{
    unsigned int numberOf = 0;
    FILE* file = NULL;
    int fileSize = 0;
    int i = 0;
    int next = 0;

    if (FileExists(fileName))
    {
        if (NULL != (file = fopen(fileName, "r")))
        {
            fseek(file, 0, SEEK_END);
            fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            for (i = 0; i < fileSize; i++)
            {
                if (what == (next = fgetc(file)))
                {
                    numberOf += 1;
                }
                else if (EOF == next)
                {
                    break;
                }
            }

            fclose(file);
        }
    }

    return numberOf;
}

unsigned int GetNumberOfLinesInFile(const char* fileName)
{
    return GetNumberOfCharacterInstancesInFile(fileName, EOL);
}

bool CharacterFoundInFile(const char* fileName, char what)
{
    return (GetNumberOfCharacterInstancesInFile(fileName, what) > 0) ? true : false;
}

int CheckNoLegacyPlusEntriesInFile(const char* fileName, char** reason, void* log)
{
    int status = 0;

    if (FileExists(fileName) && CharacterFoundInFile(fileName, '+'))
    {
        OsConfigLogError(log, "CheckNoLegacyPlusEntriesInFile(%s): there are '+' lines in file '%s'", fileName, fileName);
        OsConfigCaptureReason(reason, "There are '+' lines in file '%s'", fileName);
        status = ENOENT;
    }
    else
    {
        OsConfigLogInfo(log, "CheckNoLegacyPlusEntriesInFile(%s): there are no '+' lines in file '%s'", fileName, fileName);
        OsConfigCaptureSuccessReason(reason, "There are no '+' lines in file '%s'", fileName);
    }

    return status;
}

static int GetAccess(bool isDirectory, const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, void* log)
{
    struct stat statStruct = { 0 };
    int status = ENOENT;

    if ((NULL == name) || (NULL == ownerId) || (NULL == groupId) || (NULL == mode))
    {
        OsConfigLogError(log, "GetAccess: invalid arguments");
        return EINVAL;
    }

    *ownerId = 0;
    *groupId = 0;
    *mode = 0;

    if ((isDirectory && DirectoryExists(name)) || ((false == isDirectory) && FileExists(name)))
    {
        if (0 == (status = stat(name, &statStruct)))
        {
            *ownerId = statStruct.st_uid;
            *groupId = statStruct.st_gid;
            *mode = DecimalToOctal(statStruct.st_mode & 07777);
        }
        else
        {
            OsConfigLogError(log, "GetAccess: stat('%s') failed with %d", name, errno);
        }
    }
    else
    {
        OsConfigLogInfo(log, "GetAccess: '%s' does not exist", name);
    }

    return status;
}

int GetFileAccess(const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, void* log)
{
    return GetAccess(false, name, ownerId, groupId, mode, log);
}

int GetDirectoryAccess(const char* name, unsigned int* ownerId, unsigned int* groupId, unsigned int* mode, void* log)
{
    return GetAccess(true, name, ownerId, groupId, mode, log);
}

static int RestoreSelinuxContext(const char* target, void* log)
{
    char* restoreCommand = NULL;
    char* textResult = NULL;
    int status = 0;

    if (NULL == target)
    {
        OsConfigLogError(log, "RestoreSelinuxContext called with an invalid argument");
        status = EINVAL;
    }
    else if (NULL == (restoreCommand = FormatAllocateString("restorecon -F '%s'", target)))
    {
        OsConfigLogError(log, "RestoreSelinuxContext: out of memory");
        status = ENOMEM;
    }
    else if (0 != (status = ExecuteCommand(NULL, restoreCommand, false, false, 0, 0, &textResult, NULL, log)))
    {
        OsConfigLogError(log, "RestoreSelinuxContext: restorecon failed %d: %s", status, textResult);
    }

    FREE_MEMORY(textResult);
    FREE_MEMORY(restoreCommand);

    return status;
}

int RenameFile(const char* original, const char* target, void* log)
{
    int status = 0;

    if ((NULL == original) || (NULL == target))
    {
        OsConfigLogError(log, "RenameFile: invalid arguments");
        return EINVAL;
    }
    else if (false == FileExists(original))
    {
        OsConfigLogError(log, "RenameFile: original file '%s' does not exist", original);
        return EINVAL;
    }

    if (0 == (status = rename(original, target)))
    {
        if (IsSelinuxPresent())
        {
            RestoreSelinuxContext(target, log);
        }
    }
    else
    {
        OsConfigLogError(log, "RenameFile: rename('%s' to '%s') failed with %d", original, target, errno);
        status = (0 == errno) ? ENOENT : errno;
    }

    return status;
}

int RenameFileWithOwnerAndAccess(const char* original, const char* target, void* log)
{
    unsigned int ownerId = 0;
    unsigned int groupId = 0;
    unsigned int mode = 0;
    int status = 0;

    if ((NULL == original) || (NULL == target))
    {
        OsConfigLogError(log, "RenameFileWithOwnerAndAccess: invalid arguments");
        return EINVAL;
    }
    else if (false == FileExists(original))
    {
        OsConfigLogError(log, "RenameFileWithOwnerAndAccess: original file '%s' does not exist", original);
        return EINVAL;
    }

    if (0 != GetFileAccess(target, &ownerId, &groupId, &mode, log))
    {
        OsConfigLogInfo(log, "RenameFileWithOwnerAndAccess: cannot read owner and access mode for original target file '%s', using defaults", target);

        ownerId = 0;
        groupId = 0;

        // S_IRUSR (00400): Read permission, owner
        // S_IWUSR (00200): Write permission, owner
        // S_IRGRP (00040): Read permission, group
        // S_IROTH (00004): Read permission, others
        mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    }

    if (0 == (status = rename(original, target)))
    {
        if (0 != SetFileAccess(target, ownerId, groupId, mode, log))
        {
            OsConfigLogError(log, "RenameFileWithOwnerAndAccess: '%s' renamed to '%s' without restored original owner and access mode", original, target);
        }
        else if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(log, "RenameFileWithOwnerAndAccess: '%s' renamed to '%s' with restored original owner %u, group %u and access mode %u",
                original, target, ownerId, groupId, mode);
        }

        if (IsSelinuxPresent())
        {
            RestoreSelinuxContext(target, log);
        }
    }
    else
    {
        OsConfigLogError(log, "RenameFileWithOwnerAndAccess: rename('%s' to '%s') failed with %d", original, target, errno);
        status = (0 == errno) ? ENOENT : errno;
    }

    return status;
}

int ReplaceMarkedLinesInFile(const char* fileName, const char* marker, const char* newline, char commentCharacter, bool preserveAccess, void* log)
{
    const char* tempFileNameTemplate = "%s/~OSConfig.ReplacingLines%u";
    char* tempFileName = NULL;
    char* fileDirectory = NULL;
    char* fileNameCopy = NULL;
    FILE* fileHandle = NULL;
    FILE* tempHandle = NULL;
    int tempDescriptor = -1;
    char* line = NULL;
    long lineMax = sysconf(_SC_LINE_MAX);
    long newlineLength = newline ? (long)strlen(newline) : 0;
    bool skipLine = false;
    bool replacedLine = false;
    int status = 0;

    if ((NULL == fileName) || (NULL == marker))
    {
        OsConfigLogError(log, "ReplaceMarkedLinesInFile called with invalid arguments");
        return EINVAL;
    }
    else if (false == FileExists(fileName))
    {
        OsConfigLogInfo(log, "ReplaceMarkedLinesInFile called for a file that does not exist: '%s'", fileName);
        return 0;
    }
    else if (NULL == (line = malloc(lineMax + 1)))
    {
        OsConfigLogError(log, "ReplaceMarkedLinesInFile: out of memory");
        return ENOMEM;
    }

    if (NULL != (fileNameCopy = DuplicateString(fileName)))
    {
        fileDirectory = dirname(fileNameCopy);
    }

    if (NULL != (tempFileName = FormatAllocateString(tempFileNameTemplate, fileDirectory ? fileDirectory : "/tmp", rand())))
    {
        if (NULL != (fileHandle = fopen(fileName, "r")))
        {
            // S_IRUSR (0400): Read permission, owner
            // S_IWUSR (0200): Write permission, owner
            if (-1 != (tempDescriptor = open(tempFileName, O_EXCL | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR)))
            {
                if (NULL != (tempHandle = fdopen(tempDescriptor, "w")))
                {
                    while (NULL != fgets(line, lineMax + 1, fileHandle))
                    {
                        if (NULL != strstr(line, marker))
                        {
                            if ((commentCharacter != line[0]) && (EOL != line[0]) && (NULL != newline) && (1 < newlineLength))
                            {
                                if (replacedLine)
                                {
                                    // Already replaced this line once
                                    skipLine = true;
                                }
                                else
                                {
                                    memset(line, 0, lineMax + 1);
                                    memcpy(line, newline, (newlineLength > lineMax) ? lineMax : newlineLength);
                                    skipLine = false;
                                    replacedLine = true;
                                }
                            }
                            else if (commentCharacter == line[0])
                            {
                                skipLine = false;
                            }
                            else
                            {
                                skipLine = true;
                            }
                        }

                        if (false == skipLine)
                        {
                            if (EOF == fputs(line, tempHandle))
                            {
                                status = (0 == errno) ? EPERM : errno;
                                OsConfigLogError(log, "ReplaceMarkedLinesInFile: failed writing to temporary file '%s' (%d)", tempFileName, status);
                            }
                        }

                        memset(line, 0, lineMax + 1);
                        skipLine = false;
                    }

                    fclose(tempHandle);
                }
                else
                {
                    close(tempDescriptor);

                    OsConfigLogError(log, "ReplaceMarkedLinesInFile: failed to open temporary file '%s', fdopen() failed (%d)", tempFileName, errno);
                    status = EACCES;
                }
            }
            else
            {
                OsConfigLogError(log, "ReplaceMarkedLinesInFile: failed to open temporary file '%s', open() failed (%d)", tempFileName, errno);
                status = EACCES;
            }

            fclose(fileHandle);
        }
        else
        {
            OsConfigLogError(log, "ReplaceMarkedLinesInFile: cannot read from '%s'", fileName);
            status = EACCES;
        }
    }
    else
    {
        OsConfigLogError(log, "ReplaceMarkedLinesInFile: out of memory");
        status = ENOMEM;
    }

    FREE_MEMORY(line);

    if ((0 == status) && (false == replacedLine) && (NULL != newline))
    {
        OsConfigLogInfo(log, "ReplaceMarkedLinesInFile: line '%s' did not replace any '%s' line, to be appended at end of '%s'",
            newline, marker, fileName);

        if (false == AppendPayloadToFile(tempFileName, newline, strlen(newline), log))
        {
            OsConfigLogError(log, "ReplaceMarkedLinesInFile: failed to append line '%s' at end of '%s'", newline, fileName);
        }
    }

    if (0 == status)
    {
        if (preserveAccess)
        {
            if (0 != (status = RenameFileWithOwnerAndAccess(tempFileName, fileName, log)))
            {
                OsConfigLogError(log, "ReplaceMarkedLinesInFile: RenameFileWithOwnerAndAccess('%s' to '%s') failed with %d", tempFileName, fileName, status);
            }
        }
        else
        {
            if (0 != (status = RenameFile(tempFileName, fileName, log)))
            {
                OsConfigLogError(log, "ReplaceMarkedLinesInFile: RenameFile('%s' to '%s') failed with %d", tempFileName, fileName, status);
            }
        }

        remove(tempFileName);
    }

    FREE_MEMORY(tempFileName);
    FREE_MEMORY(fileNameCopy);

    OsConfigLogInfo(log, "ReplaceMarkedLinesInFile('%s', '%s') complete with %d", fileName, marker, status);

    return status;
}

int FindTextInFile(const char* fileName, const char* text, void* log)
{
    char* contents = NULL;
    int status = 0;

    if ((NULL == fileName) || (NULL == text) || (0 == strlen(text)))
    {
        OsConfigLogError(log, "FindTextInFile called with invalid arguments");
        return EINVAL;
    }

    if (false == FileExists(fileName))
    {
        OsConfigLogInfo(log, "FindTextInFile: file '%s' not found", fileName);
        return ENOENT;
    }

    if (NULL == (contents = LoadStringFromFile(fileName, false, log)))
    {
        OsConfigLogError(log, "FindTextInFile: cannot read from '%s'", fileName);
        status = ENOENT;
    }
    else
    {
        if (NULL != strstr(contents, text))
        {
            OsConfigLogInfo(log, "FindTextInFile: '%s' found in '%s'", text, fileName);
        }
        else
        {
            OsConfigLogInfo(log, "FindTextInFile: '%s' not found in '%s'", text, fileName);
            status = ENOENT;
        }

        FREE_MEMORY(contents);
    }

    return status;
}

int CheckTextIsFoundInFile(const char* fileName, const char* text, char** reason, void* log)
{
    int result = 0;

    if ((NULL != fileName) && (false == FileExists(fileName)))
    {
        OsConfigCaptureReason(reason, "'%s' not found", fileName);
        result = ENOENT;
    }
    else
    {
        if (0 == (result = FindTextInFile(fileName, text, log)))
        {
            OsConfigCaptureSuccessReason(reason, "'%s' found in '%s'", text, fileName);
        }
        else if (ENOENT == result)
        {
            OsConfigCaptureReason(reason, "'%s' not found in '%s'", text, fileName);
        }
    }

    return result;
}

int CheckTextIsNotFoundInFile(const char* fileName, const char* text, char** reason, void* log)
{
    int result = 0;

    if ((NULL != fileName) && (false == FileExists(fileName)))
    {
        OsConfigCaptureSuccessReason(reason, "'%s' not found", fileName);
    }
    else
    {
        if (ENOENT == (result = FindTextInFile(fileName, text, log)))
        {
            OsConfigCaptureSuccessReason(reason, "'%s' not found in '%s'", text, fileName);
            result = 0;
        }
        else if (0 == result)
        {
            OsConfigCaptureReason(reason, "'%s' found in '%s'", text, fileName);
            result = ENOENT;
        }
    }

    return result;
}

static bool IsValidGrepArgument(const char* text)
{
    return IsValidDaemonName(text);
}

// Some of the common comment characters that can be encountered, add more if necessary
static bool IsValidCommentCharacter(char c)
{
    return (('#' == c) || ('/' == c) || ('*' == c) || (';' == c) || ('!' == c)) ? true : false;
}

int CheckMarkedTextNotFoundInFile(const char* fileName, const char* text, const char* marker, char commentCharacter, char** reason, void* log)
{
    const char* commandTemplate = "grep -v '^%c' %s | grep %s";

    char* command = NULL;
    char* results = NULL;
    char* found = NULL;
    bool foundMarker = false;
    int status = 0;

    if ((false == FileExists(fileName)) || (NULL == text) || (NULL == marker) || (0 == strlen(text)) || (0 == strlen(marker)) ||
        (false == IsValidGrepArgument(text)) || (false == IsValidCommentCharacter(commentCharacter)))
    {
        OsConfigLogError(log, "CheckMarkedTextNotFoundInFile called with invalid arguments");
        return EINVAL;
    }
    else if (NULL == (command = FormatAllocateString(commandTemplate, commentCharacter, fileName, text)))
    {
        OsConfigLogError(log, "CheckMarkedTextNotFoundInFile: out of memory");
        return ENOMEM;
    }
    else
    {
        if ((0 == (status = ExecuteCommand(NULL, command, true, false, 0, 0, &results, NULL, log))) && results)
        {
            found = results;
            while (NULL != (found = strstr(found, marker)))
            {
                found += 1;
                if (0 == found[0])
                {
                    break;
                }
                else if (0 == isalpha(found[0]))
                {
                    OsConfigLogInfo(log, "CheckMarkedTextNotFoundInFile: '%s' containing '%s' found in '%s' uncommented with '%c'", text, marker, fileName, commentCharacter);
                    OsConfigCaptureReason(reason, "'%s' containing '%s' found in '%s'", text, marker, fileName);
                    foundMarker = true;
                    status = EEXIST;
                }
            }

            if (false == foundMarker)
            {
                OsConfigLogInfo(log, "CheckMarkedTextNotFoundInFile: '%s' containing '%s' not found in '%s' uncommented with '%c'", text, marker, fileName, commentCharacter);
                OsConfigCaptureSuccessReason(reason, "'%s' containing '%s' not found in '%s'", text, marker, fileName);
                status = 0;
            }
        }
        else
        {
            OsConfigLogInfo(log, "CheckMarkedTextNotFoundInFile: '%s' not found in '%s'  uncommented with '%c' (%d)", text, fileName, commentCharacter, status);
            OsConfigCaptureSuccessReason(reason, "'%s' not found in '%s' (%d)", text, fileName, status);
            status = 0;
        }

        FREE_MEMORY(results);
        FREE_MEMORY(command);
    }

    return status;
}

int CheckTextNotFoundInEnvironmentVariable(const char* variableName, const char* text, bool strictCompare, char** reason, void* log)
{
    const char* commandTemplate = "printenv %s";
    char* command = NULL;
    size_t commandLength = 0;
    char* variableValue = NULL;
    char* found = NULL;
    bool foundText = false;
    int status = 0;

    if ((NULL == variableName) || (NULL == text) || (0 == strlen(variableName)) || (0 == strlen(text) || (false == IsValidDaemonName(variableName))))
    {
        OsConfigLogError(log, "CheckTextNotFoundInEnvironmentVariable called with invalid arguments");
        return EINVAL;
    }

    commandLength = strlen(commandTemplate) + strlen(variableName) + 1;
    if (NULL == (command = malloc(commandLength)))
    {
        OsConfigLogError(log, "CheckTextNotFoundInEnvironmentVariable: out of memory");
        status = ENOMEM;
    }
    else
    {
        memset(command, 0, commandLength);
        snprintf(command, commandLength, commandTemplate, variableName);

        if ((0 == (status = ExecuteCommand(NULL, command, true, false, 0, 0, &variableValue, NULL, log))) && variableValue)
        {
            if (strictCompare)
            {
                if (0 == strcmp(variableValue, text))
                {
                    OsConfigLogError(log, "CheckTextNotFoundInEnvironmentVariable: '%s' found set for '%s' ('%s')", text, variableName, variableValue);
                    OsConfigCaptureReason(reason, "'%s' found set for '%s' ('%s')", text, variableName, variableValue);
                    status = EEXIST;
                }
                else
                {
                    OsConfigLogInfo(log, "CheckTextNotFoundInEnvironmentVariable: '%s' not found set for '%s' ('%s')", text, variableName, variableValue);
                    OsConfigCaptureSuccessReason(reason, "'%s' not found set for '%s' to '%s'", text, variableName, variableValue);
                }
            }
            else
            {
                found = variableValue;
                while (NULL != (found = strstr(found, text)))
                {
                    found += 1;
                    if (0 == found[0])
                    {
                        break;
                    }
                    else if (0 == isalpha(found[0]))
                    {
                        OsConfigLogError(log, "CheckTextNotFoundInEnvironmentVariable: '%s' found in '%s' ('%s')", text, variableName, found);
                        OsConfigCaptureReason(reason, "'%s' found in '%s' ('%s')", text, variableName, found);
                        foundText = true;
                        status = EEXIST;
                    }
                }

                if (false == foundText)
                {
                    OsConfigLogInfo(log, "CheckTextNotFoundInEnvironmentVariable: '%s' not found in '%s'", text, variableName);
                    OsConfigCaptureSuccessReason(reason, "'%s' not found in '%s'", text, variableName);
                }
            }
        }
        else
        {
            OsConfigLogInfo(log, "CheckTextNotFoundInEnvironmentVariable: variable '%s' not found (%d)", variableName, status);
            OsConfigCaptureSuccessReason(reason, "Environment variable '%s' not found (%d)", variableName, status);
        }

        FREE_MEMORY(command);
        FREE_MEMORY(variableValue);
    }

    return status;
}

int CheckSmallFileContainsText(const char* fileName, const char* text, char** reason, void* log)
{
    struct stat statStruct = {0};
    char* contents = NULL;
    size_t textLength = 0, contentsLength = 0;
    int status = 0;

    if ((NULL == fileName) || (NULL == text) || (0 == strlen(fileName)) || (0 == (textLength = strlen(text))))
    {
        OsConfigLogError(log, "CheckSmallFileContainsText called with invalid arguments");
        return EINVAL;
    }
    else if ((0 == stat(fileName, &statStruct)) && ((statStruct.st_size > MAX_STRING_LENGTH)))
    {
        OsConfigLogError(log, "CheckSmallFileContainsText: file is too large (%lu bytes, maximum supported: %d bytes)", statStruct.st_size, MAX_STRING_LENGTH);
        return EINVAL;
    }

    if (NULL != (contents = LoadStringFromFile(fileName, false, log)))
    {
        contentsLength = strlen(contents);

        if (0 == strncmp(contents, text, (textLength <= contentsLength) ? textLength : contentsLength))
        {
            OsConfigLogInfo(log, "CheckSmallFileContainsText: '%s' matches contents of '%s'", text, fileName);
            OsConfigCaptureSuccessReason(reason, "'%s' matches contents of '%s'", text, fileName);
        }
        else
        {
            OsConfigLogInfo(log, "CheckSmallFileContainsText: '%s' does not match contents of '%s' ('%s')", text, fileName, contents);
            OsConfigCaptureReason(reason, "'%s' does not match contents of '%s'", text, fileName);
            status = ENOENT;
        }
    }

    FREE_MEMORY(contents);

    return status;
}

int FindTextInFolder(const char* directory, const char* text, void* log)
{
    const char* pathTemplate = "%s/%s";

    DIR* home = NULL;
    struct dirent* entry = NULL;
    char* path = NULL;
    size_t length = 0;
    int status = ENOENT, _status = 0;

    if ((NULL == directory) || (false == DirectoryExists(directory)) || (NULL == text))
    {
        OsConfigLogError(log, "FindTextInFolder called with invalid arguments");
        return EINVAL;
    }

    if (NULL != (home = opendir(directory)))
    {
        while (NULL != (entry = readdir(home)))
        {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            {
                length = strlen(pathTemplate) + strlen(directory) + strlen(entry->d_name);
                if (NULL == (path = malloc(length + 1)))
                {
                    OsConfigLogError(log, "FindTextInFolder: out of memory");
                    status = ENOMEM;
                    break;
                }

                memset(path, 0, length + 1);
                snprintf(path, length, pathTemplate, directory, entry->d_name);

                if ((0 == (_status = FindTextInFile(path, text, log))) && (0 != status))
                {
                    status = _status;
                }

                FREE_MEMORY(path);
            }
        }

        closedir(home);
    }

    if (status)
    {
        OsConfigLogInfo(log, "FindTextInFolder: '%s' not found in any file under '%s'", text, directory);
    }

    return status;
}

int CheckTextNotFoundInFolder(const char* directory, const char* text, char** reason, void* log)
{
    int result = 0;

    if (ENOENT == (result = FindTextInFolder(directory, text, log)))
    {
        OsConfigCaptureSuccessReason(reason, "Text '%s' not found in any file under directory '%s'", text, directory);
        result = 0;
    }
    else if (0 == result)
    {
        OsConfigCaptureReason(reason, "Text '%s' found in at least one file under directory '%s'", text, directory);
        result = ENOENT;
    }

    return result;
}

int CheckTextFoundInFolder(const char* directory, const char* text, char** reason, void* log)
{
    int result = 0;

    if (0 == (result = FindTextInFolder(directory, text, log)))
    {
        OsConfigCaptureSuccessReason(reason, "Text '%s' found in at least one file under directory '%s'", text, directory);
    }
    else if (ENOENT == result)
    {
        OsConfigCaptureReason(reason, "Text '%s' not found in any file under directory '%s'", text, directory);
    }

    return result;
}

static int IsLineNotFoundOrCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, void* log)
{
    char* contents = NULL;
    char* found = NULL;
    char* index = NULL;
    bool foundUncommented = false;
    int status = ENOENT;

    if ((NULL == fileName) || (NULL == text) || (0 == strlen(text)))
    {
        OsConfigLogError(log, "IsLineNotFoundOrCommentedOut called with invalid arguments");
        return EINVAL;
    }

    if (FileExists(fileName))
    {
        if (NULL == (contents = LoadStringFromFile(fileName, false, log)))
        {
            OsConfigLogError(log, "IsLineNotFoundOrCommentedOut: cannot read from '%s'", fileName);
            OsConfigCaptureReason(reason, "Cannot read from file '%s'", fileName);
        }
        else
        {
            found = contents;

            while (NULL != (found = strstr(found, text)))
            {
                index = found;
                status = ENOENT;

                while (index > contents)
                {
                    index--;
                    if (commentMark == index[0])
                    {
                        status = 0;
                        break;
                    }
                    else if (EOL == index[0])
                    {
                        break;
                    }
                }

                if (0 == status)
                {
                    OsConfigLogInfo(log, "IsLineNotFoundOrCommentedOut: '%s' found in '%s' at position %ld but is commented out with '%c'",
                        text, fileName, (long)(found - contents), commentMark);
                }
                else
                {
                    foundUncommented = true;
                    OsConfigLogInfo(log, "IsLineNotFoundOrCommentedOut: '%s' found in '%s' at position %ld and it's not commented out with '%c'",
                        text, fileName, (long)(found - contents), commentMark);
                }

                found += strlen(text);
            }

            status = foundUncommented ? EEXIST : 0;

            FREE_MEMORY(contents);
        }
    }
    else
    {
        OsConfigLogInfo(log, "IsLineNotFoundOrCommentedOut: file '%s' not found, nothing to look for", fileName);
        if (OsConfigIsSuccessReason(reason))
        {
            OsConfigCaptureSuccessReason(reason, "'%s' is not found, nothing to look for", fileName);
        }
        else
        {
            OsConfigCaptureReason(reason, "'%s' is not found", fileName);
        }
        status = 0;
    }

    return status;
}

int CheckLineNotFoundOrCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, void* log)
{
    int result = 0;

    if ((NULL != fileName) && (false == FileExists(fileName)))
    {
        if (OsConfigIsSuccessReason(reason))
        {
            OsConfigCaptureSuccessReason(reason, "'%s' not found to look for '%s'", fileName, text);
        }
        else
        {
            OsConfigCaptureReason(reason, "'%s' is not found to look for '%s'", fileName, text);
        }
    }
    else
    {
        if (EEXIST == (result = IsLineNotFoundOrCommentedOut(fileName, commentMark, text, reason, log)))
        {
            OsConfigCaptureReason(reason, "'%s' found in '%s' and it's not commented out with '%c'", text, fileName, commentMark);
            result = EEXIST;
        }
        else if (0 == result)
        {
            OsConfigCaptureSuccessReason(reason, "'%s' not found in '%s' or it's commented out with '%c'", text, fileName, commentMark);
        }
    }

    return result;
}

int CheckLineFoundNotCommentedOut(const char* fileName, char commentMark, const char* text, char** reason, void* log)
{
    int result = 0;

    if ((NULL != fileName) && (false == FileExists(fileName)))
    {
        OsConfigCaptureReason(reason, "'%s' not found to look for '%s'", fileName, text);
        result = ENOENT;
    }
    else
    {
        if (EEXIST == (result = IsLineNotFoundOrCommentedOut(fileName, commentMark, text, reason, log)))
        {
            OsConfigCaptureSuccessReason(reason, "'%s' found in '%s' and it's not commented out with '%c'", text, fileName, commentMark);
            result = 0;
        }
        else if (0 == result)
        {
            OsConfigCaptureReason(reason, "'%s' not found in '%s' or it's commented out with '%c'", text, fileName, commentMark);
            result = EEXIST;
        }
    }

    return result;
}

static int FindTextInCommandOutput(const char* command, const char* text, void* log)
{
    char* results = NULL;
    int status = 0;

    if ((NULL == command) || (NULL == text) || (0 == strlen(command)) || (0 == strlen(text)))
    {
        OsConfigLogError(log, "FindTextInCommandOutput called with invalid argument");
        return EINVAL;
    }

    // Execute this command with a 60 seconds timeout
    if (0 == (status = ExecuteCommand(NULL, command, true, false, 0, 60, &results, NULL, log)))
    {
        if ((NULL != results) && (0 < strlen(results)) && (NULL != strstr(results, text)))
        {
            OsConfigLogInfo(log, "FindTextInCommandOutput: '%s' found in '%s' output", text, command);
        }
        else
        {
            status = ENOENT;
            OsConfigLogInfo(log, "FindTextInCommandOutput: '%s' not found in '%s' output", text, command);
        }
    }
    else
    {
        OsConfigLogInfo(log, "FindTextInCommandOutput: command '%s' failed with %d", command, status);
    }

    FREE_MEMORY(results);
    return status;
}

int CheckTextFoundInCommandOutput(const char* command, const char* text, char** reason, void* log)
{
    int result = 0;

    if (0 == (result = FindTextInCommandOutput(command, text, log)))
    {
        OsConfigCaptureSuccessReason(reason, "'%s' found in response from command '%s'", text, command);
    }
    else if (ENOENT == result)
    {
        OsConfigCaptureReason(reason, "'%s' not found in response from command '%s'", text, command);
    }
    else
    {
        OsConfigCaptureReason(reason, "Command '%s' failed with %d", command, result);
    }

    return result;
}

int CheckTextNotFoundInCommandOutput(const char* command, const char* text, char** reason, void* log)
{
    int result = 0;

    if (ENOENT == (result = FindTextInCommandOutput(command, text, log)))
    {
        OsConfigCaptureSuccessReason(reason, "'%s' not found in response from command '%s'", text, command);
        result = 0;
    }
    else if (0 == result)
    {
        OsConfigCaptureReason(reason, "'%s' found in response from command '%s'", text, command);
        result = ENOENT;
    }
    else
    {
        OsConfigCaptureReason(reason, "Command '%s' failed with %d", command, result);
    }

    return result;
}

char* GetStringOptionFromBuffer(const char* buffer, const char* option, char separator, void* log)
{
    char* found = NULL;
    char* temp = NULL;
    char* result = NULL;

    if ((NULL == buffer) || (NULL == option))
    {
        OsConfigLogError(log, "GetStringOptionFromBuffer called with invalid arguments");
        return result;
    }

    if (NULL == (temp = DuplicateString(buffer)))
    {
        OsConfigLogError(log, "GetStringOptionFromBuffer: failed to duplicate buffer string failed (%d)", errno);
    }
    else if (NULL != (found = strstr(temp, option)))
    {
        RemovePrefixUpTo(found, separator);
        RemovePrefix(found, separator);
        RemovePrefixBlanks(found);
        RemoveTrailingBlanks(found);
        TruncateAtFirst(found, EOL);
        TruncateAtFirst(found, ' ');

        OsConfigLogInfo(log, "GetStringOptionFromBuffer: found '%s' for '%s'", found, option);

        if (NULL == (result = DuplicateString(found)))
        {
            OsConfigLogError(log, "GetStringOptionFromBuffer: failed to duplicate result string (%d)", errno);
        }
    }

    FREE_MEMORY(temp);
    return result;
}

int GetIntegerOptionFromBuffer(const char* buffer, const char* option, char separator, void* log)
{
    char* stringValue = NULL;
    int value = INT_ENOENT;

    if (NULL != (stringValue = GetStringOptionFromBuffer(buffer, option, separator, log)))
    {
        value = atoi(stringValue);
        FREE_MEMORY(stringValue);
    }

    return value;
}

char* GetStringOptionFromFile(const char* fileName, const char* option, char separator, void* log)
{
    char* contents = NULL;
    char* result = NULL;

    if (option && (0 == CheckFileExists(fileName, NULL, log)))
    {
        if (NULL == (contents = LoadStringFromFile(fileName, false, log)))
        {
            OsConfigLogError(log, "GetStringOptionFromFile: cannot read from '%s'", fileName);
        }
        else
        {
            if (NULL != (result = GetStringOptionFromBuffer(contents, option, separator, log)))
            {
                OsConfigLogInfo(log, "GetStringOptionFromFile: found '%s' in '%s' for '%s'", result, fileName, option);
            }
            else
            {
                OsConfigLogInfo(log, "GetStringOptionFromFile: '%s' not found in '%s'", option, fileName);
            }

            FREE_MEMORY(contents);
        }
    }

    return result;
}

int GetIntegerOptionFromFile(const char* fileName, const char* option, char separator, void* log)
{
    char* contents = NULL;
    int result = INT_ENOENT;

    if (option && (0 == CheckFileExists(fileName, NULL, log)))
    {
        if (NULL == (contents = LoadStringFromFile(fileName, false, log)))
        {
            OsConfigLogError(log, "GetIntegerOptionFromFile: cannot read from '%s'", fileName);
        }
        else
        {
            if (INT_ENOENT != (result = GetIntegerOptionFromBuffer(contents, option, separator, log)))
            {
                OsConfigLogInfo(log, "GetIntegerOptionFromFile: found '%d' in '%s' for '%s'", result, fileName, option);
            }
            else
            {
                OsConfigLogInfo(log, "GetIntegerOptionFromFile: '%s' not found in '%s'", option, fileName);
            }

            FREE_MEMORY(contents);
        }
    }

    return result;
}

int CheckIntegerOptionFromFileEqualWithAny(const char* fileName, const char* option, char separator, int* values, int numberOfValues, char** reason, void* log)
{
    int valueFromFile = INT_ENOENT;
    int i = 0;
    int result = ENOENT;

    if ((NULL == values) || (0 == numberOfValues))
    {
        OsConfigLogError(log, "CheckIntegerOptionFromFileEqualWithAny: invalid arguments (%p, %u)", values, numberOfValues);
        return EINVAL;
    }

    if (INT_ENOENT != (valueFromFile = GetIntegerOptionFromFile(fileName, option, separator, log)))
    {
        for (i = 0; i < numberOfValues; i++)
        {
            if (valueFromFile == values[i])
            {
                OsConfigCaptureSuccessReason(reason, "Option '%s' from file '%s' set to expected value of '%d'", option, fileName, values[i]);
                result = 0;
                break;
            }
        }

        if (ENOENT == result)
        {
            OsConfigCaptureReason(reason, "Option '%s' from file '%s' not found or found set to '%d'", option, fileName, valueFromFile);
        }
    }
    else
    {
        OsConfigCaptureReason(reason, "File '%s' not found or does not contain option '%s'", fileName, option);
    }

    return result;
}

int CheckIntegerOptionFromFileLessOrEqualWith(const char* fileName, const char* option, char separator, int value, char** reason, void* log)
{
    int valueFromFile = INT_ENOENT;
    int result = ENOENT;

    if (INT_ENOENT != (valueFromFile = GetIntegerOptionFromFile(fileName, option, separator, log)))
    {
        if (valueFromFile <= value)
        {
            OsConfigCaptureSuccessReason(reason, "Option '%s' from file '%s' value of '%d' is less or equal with '%d'", option, fileName, valueFromFile, value);
            result = 0;
        }
        else
        {
            OsConfigCaptureReason(reason, "Option '%s' from file '%s' not found ('%d') or not less or equal with '%d'", option, fileName, valueFromFile, value);
        }
    }
    else
    {
        OsConfigCaptureReason(reason, "File '%s' not found or does not contain option '%s'", fileName, option);
    }

    return result;
}

int SetEtcConfValue(const char* file, const char* name, const char* value, void* log)
{
    const char* newlineTemplate = "%s %s\n";
    char* newline = NULL;
    int status = 0;

    if ((NULL == file) || (NULL == name) || (0 == strlen(name)) || (NULL == value) || (0 == strlen(value)))
    {
        OsConfigLogError(log, "SetEtcConfValue: invalid argument");
        return EINVAL;
    }
    else if (false == FileExists(file))
    {
        OsConfigLogError(log, "SetEtcConfValue: file '%s' does not exist", file);
        return ENOENT;
    }
    else if (NULL == (newline = FormatAllocateString(newlineTemplate, name, value)))
    {
        OsConfigLogError(log, "SetEtcConfValue: out of memory");
        return ENOMEM;
    }

    if (0 == (status = ReplaceMarkedLinesInFile(file, name, newline, '#', true, log)))
    {
        OsConfigLogInfo(log, "SetEtcConfValue: successfully set '%s' to '%s' in '%s'", name, value, file);
    }
    else
    {
        OsConfigLogError(log, "SetEtcConfValue: failed to set '%s' to '%s' in '%s' (%d)", name, value, file, status);
    }

    FREE_MEMORY(newline);

    return status;
}

int SetEtcLoginDefValue(const char* name, const char* value, void* log)
{
    return SetEtcConfValue("/etc/login.defs", name, value, log);
}

int DisablePostfixNetworkListening(void* log)
{
    const char* etcPostfix = "/etc/postfix/";
    const char* etcPostfixMainCf = "/etc/postfix/main.cf";
    const char* inetInterfacesLocalhost = "inet_interfaces localhost";

    // S_IRUSR (00400): Read permission, owner
    // S_IWUSR (00200): Write permission, owner
    // S_IRGRP (00040): Read permission, group
    // S_IROTH (00004): Read permission, others
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int status = 0;

    if (false == DirectoryExists(etcPostfix))
    {
        OsConfigLogInfo(log, "DisablePostfixNetworkListening: directory '%s' does not exist", etcPostfix);
        if (0 == (status = mkdir(etcPostfix, mode)))
        {
            OsConfigLogInfo(log, "DisablePostfixNetworkListening: created directory '%s' with %d access", etcPostfix, mode);
        }
        else
        {
            OsConfigLogError(log, "DisablePostfixNetworkListening: failed creating directory '%s' with %d access", etcPostfix, mode);
        }
    }

    if (0 == status)
    {
        if (AppendToFile(etcPostfixMainCf, inetInterfacesLocalhost, strlen(inetInterfacesLocalhost), log))
        {
            OsConfigLogInfo(log, "DisablePostfixNetworkListening: '%s' was written to '%s'", inetInterfacesLocalhost, etcPostfixMainCf);
        }
        else
        {
            OsConfigLogError(log, "DisablePostfixNetworkListening: failed writing '%s' to '%s' (%d)", inetInterfacesLocalhost, etcPostfixMainCf, errno);
            status = ENOENT;
        }
    }

    return status;
}
