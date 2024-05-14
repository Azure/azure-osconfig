// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

#define INT_ENOENT -999

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

    if (NULL != (file = fopen(fileName, "r")))
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
                for (i = 0; i <= fileSize; i++)
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

static bool SaveToFile(const char* fileName, const char* mode, const char* payload, const int payloadSizeBytes, void* log)
{
    FILE* file = NULL;
    int i = 0;
    bool result = true;

    if (fileName && mode && payload && (0 < payloadSizeBytes))
    {
        if (NULL != (file = fopen(fileName, mode)))
        {
            if (true == (result = LockFile(file, log)))
            {
                for (i = 0; i < payloadSizeBytes; i++)
                {
                    if (payload[i] != fputc(payload[i], file))
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
        OsConfigLogError(log, "SaveToFile: invalid arguments ('%s', '%s', '%s', %d)", fileName, mode, payload, payloadSizeBytes);
    }

    return result;
}

bool SavePayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log)
{
    return SaveToFile(fileName, "w", payload, payloadSizeBytes, log);
}

static bool InternalSecureSaveToFile(const char* fileName, const char* mode, const char* payload, const int payloadSizeBytes, void* log)
{
    const char* tempFileNameTemplate = "/tmp/~OSConfig.Temp%u";
    char* tempFileName = NULL;
    char* fileContents = NULL;
    int status = 0;
    bool result = false;

    if ((NULL == fileName) || (NULL == payload) || (0 >= payloadSizeBytes))
    {
        OsConfigLogError(log, "InternalSecureSaveToFile: invalid arguments");
        return false;
    }
    else if (NULL == (tempFileName = FormatAllocateString(tempFileNameTemplate, rand())))
    {
        OsConfigLogError(log, "InternalSecureSaveToFile: out of memory");
        return false;
    }

    if ((0 == strcmp(mode, "a") && FileExists(fileName)))
    {
        if (NULL != (fileContents = LoadStringFromFile(fileName, false, log)))
        {
            if (true == (result = SaveToFile(tempFileName, "w", fileContents, strlen(fileContents), log)))
            {
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
        
    if (result && (false == FileExists(tempFileName)))
    {
        OsConfigLogError(log, "InternalSecureSaveToFile: failed to create temporary file");
        result = false;
    }

    if (result)
    {
        if (0 != (status = rename(tempFileName, fileName)))
        {
            OsConfigLogError(log, "InternalSecureSaveToFile: rename('%s' to '%s') failed with %d", tempFileName, fileName, errno);
            result = false;
        }

        remove(tempFileName);
    }

    FREE_MEMORY(tempFileName);

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

bool MakeFileBackupCopy(const char* fileName, const char* backupName, void* log)
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
                result = SecureSaveToFile(backupName, fileContents, strlen(fileContents), log);
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

bool ConcatenateFiles(const char* firstFileName, const char* secondFileName, void* log)
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
        result = AppendToFile(firstFileName, contents, strlen(contents), log);
        FREE_MEMORY(contents);
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
        status = EEXIST;
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
        status = EEXIST;
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

int ReplaceMarkedLinesInFile(const char* fileName, const char* marker, const char* newline, char commentCharacter, void* log)
{
    const char* tempFileNameTemplate = "/tmp/~OSConfig.ReplacingLines%u";
    char* tempFileName = NULL;
    FILE* fileHandle = NULL;
    FILE* tempHandle = NULL;
    char* line = NULL;
    long lineMax = sysconf(_SC_LINE_MAX);
    long newlineLength = newline ? (long)strlen(newline) : 0;
    bool skipLine = false;
    int status = 0;

    if ((NULL == fileName) || (false == FileExists(fileName)) || (NULL == marker))
    {
        OsConfigLogError(log, "ReplaceMarkedLinesInFile called with invalid arguments");
        return EINVAL;
    }
    else if (NULL == (line = malloc(lineMax + 1)))
    {
        OsConfigLogError(log, "ReplaceMarkedLinesInFile: out of memory");
        return ENOMEM;
    }

    if (NULL != (tempFileName = FormatAllocateString(tempFileNameTemplate, rand())))
    {
        if (NULL != (fileHandle = fopen(fileName, "r")))
        {
            if (NULL != (tempHandle = fopen(tempFileName, "w")))
            {
                while (NULL != fgets(line, lineMax + 1, fileHandle))
                {
                    if (NULL != strstr(line, marker))
                    {
                        if ((commentCharacter != line[0]) && (EOL != line[0]) && (NULL != newline) && (1 < newlineLength))
                        {
                            memset(line, 0, lineMax + 1);
                            memcpy(line, newline, (newlineLength > lineMax) ? lineMax : newlineLength);
                            skipLine = false;
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
                            status =  (0 == errno) ? EPERM : errno;
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
                OsConfigLogError(log, "ReplaceMarkedLinesInFile: failed to create temporary file '%s'", tempFileName);
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

    if (0 == status)
    {
        if (0 != (status = rename(tempFileName, fileName)))
        {
            OsConfigLogError(log, "ReplaceMarkedLinesInFile: rename('%s' to '%s') failed with %d", tempFileName, fileName, errno);
            status = (0 == errno) ? ENOENT : errno;
        }
        
        remove(tempFileName);
    }

    FREE_MEMORY(tempFileName);

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

int CheckMarkedTextNotFoundInFile(const char* fileName, const char* text, const char* marker, char** reason, void* log)
{
    const char* commandTemplate = "cat %s | grep %s";
    char* command = NULL;
    char* results = NULL;
    char* found = 0;
    size_t commandLength = 0;
    bool foundMarker = false;
    int status = 0;

    if ((!FileExists(fileName)) || (NULL == text) || (NULL == marker) || (0 == strlen(text)) || (0 == strlen(marker)))
    {
        OsConfigLogError(log, "CheckMarkedTextNotFoundInFile called with invalid arguments");
        return EINVAL;
    }

    commandLength = strlen(commandTemplate) + strlen(fileName) + strlen(text) + 1;
    if (NULL == (command = malloc(commandLength)))
    {
        OsConfigLogError(log, "CheckMarkedTextNotFoundInFile: out of memory");
        status = ENOMEM;
    }
    else
    {
        memset(command, 0, commandLength);
        snprintf(command, commandLength, commandTemplate, fileName, text);

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
                    OsConfigLogInfo(log, "CheckMarkedTextNotFoundInFile: '%s' containing '%s' found in '%s' ('%s')", text, marker, fileName, found);
                    OsConfigCaptureReason(reason, "'%s' containing '%s' found in '%s' ('%s')", text, marker, fileName, found);
                    foundMarker = true;
                    status = EEXIST;
                } 
            } 
            
            if (false == foundMarker)
            {
                OsConfigLogInfo(log, "CheckMarkedTextNotFoundInFile: '%s' containing '%s' not found in '%s'", text, marker, fileName);
                OsConfigCaptureSuccessReason(reason, "'%s' containing '%s' not found in '%s'", text, marker, fileName);
            }
        }
        else
        {
            OsConfigLogInfo(log, "CheckMarkedTextNotFoundInFile: '%s' not found in '%s' (%d)", text, fileName, status);
            OsConfigCaptureSuccessReason(reason, "'%s' not found in '%s' (%d)", text, fileName, status);
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

    if ((NULL == variableName) || (NULL == text) || (0 == strlen(variableName)) || (0 == strlen(text)))
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

int CheckFileContents(const char* fileName, const char* text, char** reason, void* log)
{
    char* contents = NULL;
    int status = 0;

    if ((NULL == fileName) || (NULL == text) || (0 == strlen(fileName)) || (0 == strlen(text)))
    {
        OsConfigLogError(log, "CheckFileContents called with invalid arguments");
        return EINVAL;
    }

    if (NULL != (contents = LoadStringFromFile(fileName, false, log)))
    {
        if (0 == strncmp(contents, text, strlen(text)))
        {
            OsConfigLogInfo(log, "CheckFileContents: '%s' matches contents of '%s'", text, fileName);
            OsConfigCaptureSuccessReason(reason, "'%s' matches contents of '%s'", text, fileName);
        }
        else
        {
            OsConfigLogInfo(log, "CheckFileContents: '%s' does not match contents of '%s' ('%s')", text, fileName, contents);
            OsConfigCaptureReason(reason, "'%s' does not match contents of '%s' ('%s')", text, fileName, contents);
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
        OsConfigCaptureSuccessReason(reason, "Text '%s' found in at least one file under directory '%s'", text, directory);
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
        OsConfigCaptureSuccessReason(reason, "Text '%s' not found in any file under directory '%s'", text, directory);
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

    if ((NULL == command) || (NULL == text))
    {
        OsConfigLogError(log, "FindTextInCommandOutput called with invalid argument");
        return EINVAL;
    }

    if (0 == (status = ExecuteCommand(NULL, command, true, false, 0, 0, &results, NULL, log)))
    {
        if (NULL != strstr(results, text))
        {
            OsConfigLogInfo(log, "FindTextInCommandOutput: '%s' found in '%s' output", text, command);
        }
        else
        {
            status = ENOENT;
            OsConfigLogInfo(log, "FindTextInCommandOutput: '%s' not found in '%s' output", text, command);
        }

        FREE_MEMORY(results);
    }
    else
    {
        OsConfigLogInfo(log, "FindTextInCommandOutput: command '%s' failed with %d", command, status);
    }

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

static char* GetStringOptionFromBuffer(const char* buffer, const char* option, char separator, void* log)
{
    char* found = NULL;
    char* internal = NULL;
    char* result = NULL;
    
    if ((NULL == buffer) || (NULL == option))
    {
        OsConfigLogError(log, "GetStringOptionFromBuffer called with invalid arguments");
        return result;
    }

    if (NULL == (internal = DuplicateString(buffer)))
    {
        OsConfigLogError(log, "GetStringOptionFromBuffer: failed to duplicate buffer string failed (%d)", errno);
    }
    else if (NULL != (found = strstr(internal, option)))
    {
        RemovePrefixUpTo(found, separator);
        RemovePrefixBlanks(found);
        RemoveTrailingBlanks(found);
        TruncateAtFirst(found, EOL);
        TruncateAtFirst(found, ' ');

        OsConfigLogInfo(log, "GetStringOptionFromBuffer: found '%s' for '%s'", found, option);

        if (NULL == (result = DuplicateString(found)))
        {
            OsConfigLogError(log, "GetStringOptionFromBuffer: failed to duplicate result string (%d)", errno);
        }

        FREE_MEMORY(internal);
    }

    return result;
}

static int GetIntegerOptionFromBuffer(const char* buffer, const char* option, char separator, void* log)
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

int SetEtcLoginDefValue(const char* name, const char* value, void* log)
{
    const char* etcLoginDefs = "/etc/login.defs";
    const char* tempLoginDefs = "/etc/~login.defs.copy";
    const char* newlineTemplate = "%s %s\n";
    char* newline = NULL;
    char* original = NULL;
    int status = 0;

    if ((NULL == name) || (0 == strlen(name)) || (NULL == value) || (0 == strlen(value)))
    {
        OsConfigLogError(log, "SetEtcLoginDefValue: invalid argument");
        return EINVAL;
    }
    else if (NULL == (newline = FormatAllocateString(newlineTemplate, name, value)))
    {
        OsConfigLogError(log, "SetEtcLoginDefValue: out of memory");
        return ENOMEM;
    }

    if (NULL != (original = LoadStringFromFile(etcLoginDefs, false, log)))
    {
        if (SavePayloadToFile(tempLoginDefs, original, strlen(original), log))
        {
            if (0 == (status = ReplaceMarkedLinesInFile(tempLoginDefs, name, newline, '#', log)))
            {
                if (0 != (status = rename(tempLoginDefs, etcLoginDefs)))
                {
                    OsConfigLogError(log, "SetEtcLoginDefValue: rename('%s' to '%s') failed with %d", tempLoginDefs, etcLoginDefs, errno);
                    status = (0 == errno) ? ENOENT : errno;
                }
            }
            
            remove(tempLoginDefs);
        }
        else
        {
            OsConfigLogError(log, "SetEtcLoginDefValue: failed saving copy of '%s' to temp file '%s", etcLoginDefs, tempLoginDefs);
            status = EPERM;
        }

        FREE_MEMORY(original);
    }
    else
    {
        OsConfigLogError(log, "SetEtcLoginDefValue: failed reading '%s", etcLoginDefs);
        status = EACCES;
    }

    FREE_MEMORY(newline);

    return status;
}

int CheckLockoutForFailedPasswordAttempts(const char* fileName, const char* pamSo, char commentCharacter, char** reason, void* log)
{
    const char* auth = "auth";
    const char* required = "required";
    FILE* fileHandle = NULL;
    char* line = NULL;
    char* authValue = NULL;
    int deny = INT_ENOENT;
    int unlockTime = INT_ENOENT;
    long lineMax = sysconf(_SC_LINE_MAX);
    int status = ENOENT;

    if ((NULL == fileName) || (NULL == pamSo))
    {
        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: invalid arguments");
        return EINVAL;
    }
    else if (0 != CheckFileExists(fileName, reason, log))
    {
        // CheckFileExists logs
        return ENOENT;
    }
    else if (NULL == (line = malloc(lineMax + 1)))
    {
        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: out of memory");
        return ENOMEM;
    }
    else
    {
        memset(line, 0, lineMax + 1);
    }

    if (NULL == (fileHandle = fopen(fileName, "r")))
    {
        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: cannot read from '%s'", fileName);
        status = EACCES;
    }
    else
    {
        status = ENOENT;

        while (NULL != fgets(line, lineMax + 1, fileHandle))
        {
            // Example of valid lines: 
            //
            // 'auth required pam_tally2.so onerr=fail audit silent deny=5 unlock_time=900' in /etc/pam.d/login
            // 'auth required pam_faillock.so preauth silent audit deny=3 unlock_time=900' in /etc/pam.d/system-auth

            if ((commentCharacter == line[0]) || (EOL == line[0]))
            {
                status = 0;
                continue;
            }
            else if ((NULL != strstr(line, auth)) && (NULL != strstr(line, pamSo)) && 
                (NULL != (authValue = GetStringOptionFromBuffer(line, auth, ' ', log))) && (0 == strcmp(authValue, required)) && FreeAndReturnTrue(authValue) &&
                (0 <= (deny = GetIntegerOptionFromBuffer(line, "deny", '=', log))) && (deny <= 5) &&
                (0 < (unlockTime = GetIntegerOptionFromBuffer(line, "unlock_time", '=', log))))
            {
                OsConfigLogInfo(log, "CheckLockoutForFailedPasswordAttempts: '%s %s %s' found uncommented with 'deny' set to %d and 'unlock_time' set to %d in '%s'",
                    auth, required, pamSo, deny, unlockTime, fileName);
                OsConfigCaptureSuccessReason(reason, "'%s %s %s' found uncommented with 'deny' set to %d and 'unlock_time' set to %d in '%s'",
                    auth, required, pamSo, deny, unlockTime, fileName);
                
                status = 0;
                break;
            }
            else
            {
                status = ENOENT;
            }

            memset(line, 0, lineMax + 1);
        }
        
        if (status)
        {
            if (INT_ENOENT == deny)
            {
                OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: 'deny' not found in '%s' for '%s'", fileName, pamSo);
                OsConfigCaptureReason(reason, "'deny' not found in '%s' for '%s'", fileName, pamSo);
            }
            else
            {
                OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: 'deny' found set to %d in '%s' for '%s' instead of a value between 0 and 5", 
                    deny, fileName, pamSo);
                OsConfigCaptureReason(reason, "'deny' found set to %d in '%s' for '%s' instead of a value between 0 and 5", deny, fileName, pamSo);
            }
        
            if (INT_ENOENT == unlockTime)
            {
                OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: 'unlock_time' not found in '%s' for '%s'", fileName, pamSo);
                OsConfigCaptureReason(reason, "'unlock_time' not found in '%s' for '%s'", fileName, pamSo);
            }
            else
            {
                OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: 'unlock_time' found set to %d in '%s' for '%s' instead of a positive value",
                    unlockTime, fileName, pamSo);
                OsConfigCaptureReason(reason, "'unlock_time' found set to %d in '%s' for '%s' instead of a positive value", unlockTime, fileName, pamSo);
            }
        }

        fclose(fileHandle);
    }

    FREE_MEMORY(line);

    return status;
}

int SetLockoutForFailedPasswordAttempts(void* log)
{
    // These configuration lines are used in the PAM (Pluggable Authentication Module) settings to count
    // number of attempted accesses and lock user accounts after a specified number of failed login attempts.
    //
    // For /etc/pam.d/login:
    //
    // 'auth required pam_tally2.so file=/var/log/tallylog onerr=fail audit silent deny=5 unlock_time=900 even_deny_root'
    //
    // For /etc/pam.d/system-auth and /etc/pam.d/password-auth:
    //
    // 'auth required [default=die] pam_faillock.so preauth silent audit deny=3 unlock_time=900 even_deny_root'
    //
    // Where:
    //
    // - 'auth': specifies that the module is invoked during authentication
    // - 'required': the module is essential for authentication to proceed
    // - '[default=die]': sets the default behavior if the module fails (e.g., due to too many failed login attempts), then the authentication process will terminate immediately
    // - 'pam_tally2.so': the PAM pam_tally2 module, which maintains a count of attempted accesses during the authentication process
    // - 'pam_faillock.so': the PAM_faillock module, which maintains a list of failed authentication attempts per user
    // - 'file=/var/log/tallylog': the default log file used to keep login counts
    // - 'onerr=fail': if an error occurs (e.g., unable to open a file), return with a PAM error code
    // - 'audit': generate an audit record for this event
    // - 'silent': do not display any error messages
    // - 'deny=5': deny access if the tally (failed login attempts) for this user exceeds 5 times
    // - 'unlock_time=900': allow access after 900 seconds (15 minutes) following a failed attempt

    const char* pamTally2Line = "auth required pam_tally2.so file=/var/log/tallylog onerr=fail audit silent deny=5 unlock_time=900 even_deny_root\n";
    const char* pamFailLockLine = "auth required [default=die] pam_faillock.so preauth silent audit deny=3 unlock_time=900 even_deny_root";
    const char* etcPamdLogin = "/etc/pam.d/login";
    const char* etcPamdSystemAuth = "/etc/pam.d/system-auth"; 
    const char* etcPamdPasswordAuth = "/etc/pam.d/password-auth";
    const char* marker = "auth";

    int status = ENOENT;

    if (0 == CheckFileExists(etcPamdSystemAuth, NULL, log))
    {
        if (0 != (status = ReplaceMarkedLinesInFile(etcPamdSystemAuth, marker, pamFailLockLine, '#', log)))
        {
            if (AppendToFile(etcPamdSystemAuth, pamFailLockLine, strlen(pamFailLockLine), log))
            {
                OsConfigLogInfo(log, "SetLockoutForFailedPasswordAttempts: line '%s' was added to '%s'", pamFailLockLine, etcPamdSystemAuth);
                status = 0;
            }
            else
            {
                OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed to append line '%s' to '%s'", pamFailLockLine, etcPamdSystemAuth);
            }
        }
    }
    
    if (0 == CheckFileExists(etcPamdPasswordAuth, NULL, log))
    {
        if (0 != (status = ReplaceMarkedLinesInFile(etcPamdPasswordAuth, marker, pamFailLockLine, '#', log)))
        {
            if (AppendToFile(etcPamdPasswordAuth, pamFailLockLine, strlen(pamFailLockLine), log))
            {
                OsConfigLogInfo(log, "SetLockoutForFailedPasswordAttempts: line '%s' was added to '%s'", pamFailLockLine, etcPamdPasswordAuth);
                status = 0;
            }
            else
            {
                OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed to append line '%s' to '%s'", pamFailLockLine, etcPamdPasswordAuth);
            }
        }
    }

    if (0 == CheckFileExists(etcPamdLogin, NULL, log))
    {
        if (0 != (status = ReplaceMarkedLinesInFile(etcPamdLogin, marker, pamTally2Line, '#', log)))
        {
            if (AppendToFile(etcPamdLogin, pamTally2Line, strlen(pamTally2Line), log))
            {
                OsConfigLogInfo(log, "SetLockoutForFailedPasswordAttempts: line '%s' was added to '%s'", pamTally2Line, etcPamdLogin);
                status = 0;
            }
            else
            {
                OsConfigLogError(log, "SetLockoutForFailedPasswordAttempts: failed to append line '%s' to '%s'", pamTally2Line, etcPamdLogin);
            }
        }
    }

    return status;
}

/*
'password requisite pam_pwquality.so retry=3 minlen=12 difok=1 lcredit=1 ucredit=1 ocredit=1 dcredit=-1' in file /etc/pam.d/common-password
-------------------------------------------------------------------------------------------------------------------------------------------
password requisite pam_pwquality.so: This section specifies that the pam_pwquality module is required during password authentication. It performs strength-checking for passwords.
retry=3: The retry parameter indicates that the user will be prompted at most 3 times to enter a valid password before an error is returned.
minlen=12: The minlen parameter sets the minimum acceptable length for a password to 12 characters.
difok=1: The difok parameter controls the number of character changes (inserts, removals, or replacements) between the old and new password that are enough to accept the new password. In this case, it allows 1 change.
lcredit=1, ucredit=1, ocredit=1, dcredit=-1:
lcredit: Specifies the minimum number of lowercase letters required in the password (here, at least 1).
ucredit: Specifies the minimum number of uppercase letters required in the password (again, at least 1).
ocredit: Specifies the minimum number of other (non-alphanumeric) characters required in the password (at least 1).
dcredit: Specifies the minimum number of digits required in the password (here, -1 means no requirement).
How to set password creation requirements via minlen, minclass, dcredit, ucredit, ocredit, lcredit in /etc/pam.d/common-password and /etc/security/pwquality.conf?

'minclass = 4 OR dcredit = -1 ucredit = -1 ocredit = -1 lcredit = -1' in file /etc/security/pwquality.conf
----------------------------------------------------------------------------------------------------------
minclass = 4: This parameter specifies the minimum number of required classes of characters for a new password. These classes include:
Digits
Uppercase letters
Lowercase letters
Other characters (e.g., symbols)
In this case, the requirement is that a password must contain characters from at least 4 different classes to be considered valid.
dcredit = -1, ucredit = -1, ocredit = -1, lcredit = -1:
dcredit: This parameter sets the maximum credit for having digits in the new password. If the value is less than 0 (as in this case), it means there is no specific requirement for the number of digits.
ucredit: Similarly, this parameter sets the maximum credit for having uppercase letters. Again, with a value less than 0, there is no specific requirement for uppercase letters.
ocredit: Sets the maximum credit for having other characters (non-alphanumeric symbols). Once more, a value less than 0 means no specific requirement for other characters.
lcredit: Finally, this parameter sets the maximum credit for having lowercase letters. As before, a value less than 0 implies no specific requirement for lowercase letters.

minlen: Specifies the minimum password length.
minclass: Sets the minimum number of character types that must be used (e.g., uppercase, lowercase, digits, other).
dcredit: Limits the maximum number of digits allowed.
ucredit: Limits the maximum number of uppercase characters allowed.
ocredit: Limits the maximum number of other characters (e.g., punctuation marks) allowed.
lcredit: Limits the maximum number of lowercase characters allowed.

Here’s how you can configure these settings:

For Debian-based systems (e.g., Ubuntu): edit the file /etc/pam.d/common-password.
Add or modify the following line to enforce complexity:
'password requisite pam_pwquality.so retry=3 minlen=12 difok=1 lcredit=1 ucredit=1 ocredit=1 dcredit=-1' in file /etc/pam.d/common-password

For RedHat-based systems (e.g., CentOS, Rocky Linux): edit the file /etc/security/pwquality.conf.
Add or modify the following line to enforce complexity:
'minclass = 4 OR dcredit = -1 ucredit = -1 ocredit = -1 lcredit = -1' in file /etc/security/pwquality.conf

The ASB requires:

Ensure password creation requirements are configured.
(5.3.1)	Description: Strong passwords protect systems from being hacked through brute force methods.
Set the following key/value pairs in the appropriate PAM for your distro: 
minlen=14, minclass = 4, dcredit = -1, ucredit = -1, ocredit = -1, lcredit = -1
*/

int CheckPasswordCreationRequirements(int retry, int minlen, int minclass, int dcredit, int ucredit, int ocredit, int lcredit, char** reason, void* log)
{
    const char* etcPamdCommonPassword = "/etc/pam.d/common-password";
    const char* etcSecurityPwQualityConf = "/etc/security/pwquality.conf";
    bool etcPamdCommonPasswordExists = (0 == CheckFileExists(etcPamdCommonPassword, NULL, log)) ? true : false;
    bool etcSecurityPwQualityConfExists = (0 == CheckFileExists(etcSecurityPwQualityConf, NULL, log)) ? true : false;
    const char* fileName = etcSecurityPwQualityConfExists ? etcSecurityPwQualityConf : etcPamdCommonPassword;
    int retryOption = 0;
    int minlenOption = 0;
    int minclassOption = 0;
    int dcreditOption = 0;
    int ucreditOption = 0;
    int ocreditOption = 0;
    int lcreditOption = 0;
    const char* password = "password";
    const char* requisite = "requisite";
    const char* pamPwQualitySo = "pam_pwquality.so";
    const char* ucreditName = "ucredit";
    const char* minclassName = "minclass";
    char commentCharacter = '#';
    FILE* fileHandle = NULL;
    char* line = NULL;
    long lineMax = sysconf(_SC_LINE_MAX);
    int status = ENOENT;

    if ((false == etcPamdCommonPasswordExists) && (false == etcSecurityPwQualityConfExists))
    {
        OsConfigLogError(log, "CheckPasswordCreationRequirements: neither '%s' or '%s' exist", etcPamdCommonPassword, etcSecurityPwQualityConf);
        OsConfigCaptureReason(reason, "Neither '%s' or '%s' exist", etcPamdCommonPassword, etcSecurityPwQualityConf);
        return ENOMEM;
    }
    if (NULL == (line = malloc(lineMax + 1)))
    {
        OsConfigLogError(log, "CheckPasswordCreationRequirements: out of memory");
        return ENOMEM;
    }
    else
    {
        memset(line, 0, lineMax + 1);
    }

    if (NULL == (fileHandle = fopen(fileName, "r")))
    {
        OsConfigLogError(log, "CheckPasswordCreationRequirements: cannot read from '%s'", fileName);
        OsConfigCaptureReason(reason, "Cannot read from '%s'", fileName);
        status = EACCES;
    }
    else
    {
        status = ENOENT;

        while (NULL != fgets(line, lineMax + 1, fileHandle))
        {
            // Example of valid lines: 
            //
            // 'password requisite pam_pwquality.so retry=3 minlen=14 difok=1 lcredit=-1 ucredit=1 ocredit=-1 dcredit=-1' for file/etc/pam.d/common-password
            // 'minclass = 4 OR dcredit = -1 ucredit = -1 ocredit = -1 lcredit = -1' for file/etc/security/ pwquality.conf

            if ((commentCharacter == line[0]) || (EOL == line[0]))
            {
                status = 0;
                continue;
            }
            else if (etcPamdCommonPasswordExists && (NULL != strstr(line, password)) && (NULL != strstr(line, requisite)) && (NULL != strstr(line, pamPwQualitySo)))
            {
                if ((retry == (retryOption = GetIntegerOptionFromBuffer(line, "retry", '=', log))) &&
                    (minlen == (minlenOption = GetIntegerOptionFromBuffer(line, "minlen", '=', log))) &&
                    (minclass == (minclassOption = GetIntegerOptionFromBuffer(line, "minclass", '=', log))) &&
                    (dcredit == (dcreditOption = GetIntegerOptionFromBuffer(line, "dcredit", '=', log))) &&
                    (ucredit == (ucreditOption = GetIntegerOptionFromBuffer(line, "ucredit", '=', log))) &&
                    (ocredit == (ocreditOption = GetIntegerOptionFromBuffer(line, "ocredit", '=', log))) &&
                    (lcredit == (lcreditOption = GetIntegerOptionFromBuffer(line, "lcredit", '=', log))))
                {
                    OsConfigLogInfo(log, "CheckLockoutForFailedPasswordAttempts: '%s' contains uncommented '%s %s %s' with the expected password creation requirements "
                        "(retry: %d, minlen: %d, minclass: %d, dcredit: %d, ucredit: %d, ocredit: %d, lcredit: %d)", etcPamdCommonPassword, password, requisite,
                        pamPwQualitySo, retryOption, minlenOption, minclassOption, dcreditOption, ucreditOption, ocreditOption, lcreditOption);
                    OsConfigCaptureSuccessReason(reason, "'%s' contains uncommented '%s %s %s' with the expected password creation requirements "
                        "(retry: %d, minlen: %d, minclass: %d, dcredit: %d, ucredit: %d, ocredit: %d, lcredit: %d)", etcPamdCommonPassword, password, requisite,
                        pamPwQualitySo, retryOption, minlenOption, minclassOption, dcreditOption, ucreditOption, ocreditOption, lcreditOption);
                    status = 0;
                }
                else
                {
                    if (INT_ENOENT == retryOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'retry' is missing", etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'retry' is missing", etcPamdCommonPassword);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'retry' is set to %d instead of %d", etcPamdCommonPassword, minlenOption, minlen);
                        OsConfigCaptureReason(reason, "In '%s' 'retry' is set to %d instead of %d", etcPamdCommonPassword, minlenOption, minlen);
                    }

                    if (INT_ENOENT == minlenOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'minlen' is missing", etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'minlen' is missing", etcPamdCommonPassword);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'minlen' is set to %d instead of %d", etcPamdCommonPassword, minlenOption, minlen);
                        OsConfigCaptureReason(reason, "In '%s' 'minlen' is set to %d instead of %d", etcPamdCommonPassword, minlenOption, minlen);
                    }

                    if (INT_ENOENT == minclassOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'minclass' is missing", etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'minclass' is missing", etcPamdCommonPassword);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'minclass' is set to %d instead of %d", etcPamdCommonPassword, minclassOption, minclass);
                        OsConfigCaptureReason(reason, "In '%s' 'minclass' is set to %d instead of %d", etcPamdCommonPassword, minclassOption, minclass);
                    }

                    if (INT_ENOENT == dcreditOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'dcredit' is missing", etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'dcredit' is missing", etcPamdCommonPassword);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'dcredit' is set to '%d' instead of %d", etcPamdCommonPassword, dcreditOption, dcredit);
                        OsConfigCaptureReason(reason, "In '%s' 'dcredit' is set to '%d' instead of %d", etcPamdCommonPassword, dcreditOption, dcredit);
                    }

                    if (INT_ENOENT == ucreditOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'ucredit' missing", etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'ucredit' missing", etcPamdCommonPassword);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'ucredit' set to '%d' instead of %d", etcPamdCommonPassword, ucreditOption, ucredit);
                        OsConfigCaptureReason(reason, "In '%s' 'ucredit' set to '%d' instead of %d", etcPamdCommonPassword, ucreditOption, ucredit);
                    }

                    if (INT_ENOENT == ocreditOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'ocredit' missing", etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'ocredit' missing", etcPamdCommonPassword);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'ocredit' set to '%d' instead of %d", etcPamdCommonPassword, ocreditOption, ocredit);
                        OsConfigCaptureReason(reason, "In '%s' 'ocredit' set to '%d' instead of %d", etcPamdCommonPassword, ocreditOption, ocredit);
                    }

                    if (INT_ENOENT == lcreditOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'lcredit' missing", etcPamdCommonPassword);
                        OsConfigCaptureReason(reason, "In '%s' 'lcredit' missing", etcPamdCommonPassword);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'lcredid' set to '%d' instead of %d", etcPamdCommonPassword, lcreditOption, lcredit);
                        OsConfigCaptureReason(reason, "In '%s' 'lcredid' set to '%d' instead of %d", etcPamdCommonPassword, lcreditOption, lcredit);
                    }

                    status = ENOENT;
                }

                break;
            } 
            else if (etcSecurityPwQualityConfExists && (NULL != strstr(line, minclassName)) && (NULL != strstr(line, ucreditName)))
            {
                if ((minclass == (minclassOption = GetIntegerOptionFromFile(etcSecurityPwQualityConf, "minclass", '=', log))) &&
                    (dcredit == (dcreditOption = GetIntegerOptionFromFile(etcSecurityPwQualityConf, "dcredit", '=', log))) &&
                    (ucredit == (ucreditOption = GetIntegerOptionFromFile(etcSecurityPwQualityConf, "ucredit", '=', log))) &&
                    (ocredit == (ocreditOption = GetIntegerOptionFromFile(etcSecurityPwQualityConf, "ocredit", '=', log))) &&
                    (lcredit == (lcreditOption = GetIntegerOptionFromFile(etcSecurityPwQualityConf, "lcredit", '=', log))))
                {
                    OsConfigLogInfo(log, "CheckLockoutForFailedPasswordAttempts: '%s' contains the expected password creation requirements "
                        "(minclass: %d, dcredit : %d, ucredit : %d, ocredit : %d, lcredit : %d)", etcSecurityPwQualityConf,
                        minclassOption, dcreditOption, ucreditOption, ocreditOption, lcreditOption);
                    OsConfigCaptureSuccessReason(reason, "'%s' contains the expected password creation requirements "
                        "(minclass: %d, dcredit: %d, ucredit: %d, ocredit: %d, lcredit: %d)", etcSecurityPwQualityConf,
                        minclassOption, dcreditOption, ucreditOption, ocreditOption, lcreditOption);
                    status = 0;
                }
                else
                {
                    if (INT_ENOENT == minclassOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: iIn '%s' 'minclass' is missing", etcSecurityPwQualityConf);
                        OsConfigCaptureReason(reason, "In '%s' 'minclass' is missing", etcSecurityPwQualityConf);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'minclass' is set to %d instead of %d", etcSecurityPwQualityConf, minclassOption, minclass);
                        OsConfigCaptureReason(reason, "In '%s' 'minclass' is set to %d instead of %d", etcSecurityPwQualityConf, minclassOption, minclass);
                    }

                    if (INT_ENOENT == dcreditOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'dcredit' is missing", etcSecurityPwQualityConf);
                        OsConfigCaptureReason(reason, "In '%s' 'dcredit' is missing", etcSecurityPwQualityConf);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'dcredit' is set to '%d' instead of %d", etcSecurityPwQualityConf, dcreditOption, dcredit);
                        OsConfigCaptureReason(reason, "In '%s' 'dcredit' is set to '%d' instead of %d", etcSecurityPwQualityConf, dcreditOption, dcredit);
                    }

                    if (INT_ENOENT == ucreditOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'ucredit' missing", etcSecurityPwQualityConf);
                        OsConfigCaptureReason(reason, "In '%s' 'ucredit' missing", etcSecurityPwQualityConf);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'ucredit' set to '%d' instead of %d", etcSecurityPwQualityConf, ucreditOption, ucredit);
                        OsConfigCaptureReason(reason, "In '%s' 'ucredit' set to '%d' instead of %d", etcSecurityPwQualityConf, ucreditOption, ucredit);
                    }

                    if (INT_ENOENT == ocreditOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'ocredit' missing", etcSecurityPwQualityConf);
                        OsConfigCaptureReason(reason, "In '%s' 'ocredit' missing", etcSecurityPwQualityConf);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'ocredit' set to '%d' instead of %d", etcSecurityPwQualityConf, ocreditOption, ocredit);
                        OsConfigCaptureReason(reason, "In '%s' 'ocredit' set to '%d' instead of %d", etcSecurityPwQualityConf, ocreditOption, ocredit);
                    }

                    if (INT_ENOENT == lcreditOption)
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'lcredit' missing", etcSecurityPwQualityConf);
                        OsConfigCaptureReason(reason, "In '%s' 'lcredit' missing", etcSecurityPwQualityConf);
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: in '%s' 'lcredid' set to '%d' instead of %d", etcSecurityPwQualityConf, lcreditOption, lcredit);
                        OsConfigCaptureReason(reason, "In '%s' 'lcredid' set to '%d' instead of %d", etcSecurityPwQualityConf, lcreditOption, lcredit);
                    }

                    status = ENOENT;

                }
                
                break;
            }
            else
            {
                status = ENOENT;
            }

            memset(line, 0, lineMax + 1);
        }

        fclose(fileHandle);
    }

    FREE_MEMORY(line);

    return result;
}