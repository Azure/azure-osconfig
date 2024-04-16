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

bool SavePayloadToFile(const char* fileName, const char* payload, const int payloadSizeBytes, void* log)
{
    FILE* file = NULL;
    int i = 0;
    bool result = true;

    if (fileName && payload && (0 < payloadSizeBytes))
    {
        if (NULL != (file = fopen(fileName, "w")))
        {
            if (true == (result = LockFile(file, log)))
            {
                for (i = 0; i < payloadSizeBytes; i++)
                {
                    if (payload[i] != fputc(payload[i], file))
                    {
                        result = false;
                        OsConfigLogError(log, "SavePayloadToFile: failed saving '%c' to '%s' (%d)", payload[i], fileName, errno);
                    }
                }

                UnlockFile(file, log);
            }
            else
            {
                OsConfigLogError(log, "SavePayloadToFile: cannot lock '%s' for exclusive access while writing (%d)", fileName, errno);
            }
            
            fclose(file);
        }
        else
        {
            result = false;
            OsConfigLogError(log, "SavePayloadToFile: cannot open for write '%s' (%d)", fileName, errno);
        }
    }
    else
    {
        result = false;
        OsConfigLogError(log, "SavePayloadToFile: invalid arguments (%s, '%s', %d)", fileName, payload, payloadSizeBytes);
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
                    OsConfigLogInfo(log, "CheckAccess: ownership of '%s' (%d, %d) matches expected", name, statStruct.st_uid, statStruct.st_gid);
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

int CheckFileSystemMountingOption(const char* mountFileName, const char* mountDirectory, const char* mountType, const char* desiredOption, char** reason, void* log)
{
    FILE* mountFileHandle = NULL;
    struct mntent* mountStruct = NULL;
    bool matchFound = false;
    int lineNumber = 0;
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
                    OsConfigLogInfo(log, "CheckFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' found in file '%s' at line '%d'", 
                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName, lineNumber);
                    
                    if (NULL != mountDirectory)
                    {
                        OsConfigCaptureSuccessReason(reason, "Option '%s' for mount directory '%s' found in file '%s' at line '%d'", desiredOption, mountDirectory, mountFileName, lineNumber);
                    }

                    if (NULL != mountType)
                    {
                        OsConfigCaptureSuccessReason(reason, "Option '%s' for mount type '%s' found in file '%s' at line '%d'", desiredOption, mountType, mountFileName, lineNumber);
                    }
                }
                else
                {
                    status = ENOENT;
                    OsConfigLogError(log, "CheckFileSystemMountingOption: option '%s' for mount directory '%s' or mount type '%s' missing from file '%s' at line %d",
                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName, lineNumber);
                    
                    if (NULL != mountDirectory)
                    {
                        OsConfigCaptureReason(reason, "Option '%s' for mount directory '%s' is missing from file '%s' at line %d", desiredOption, mountDirectory, mountFileName, lineNumber);
                    }

                    if (NULL != mountType)
                    {
                        OsConfigCaptureReason(reason, "Option '%s' for mount type '%s' missing from file '%s' at line %d", desiredOption, mountType, mountFileName, lineNumber);
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
            status = ENOENT;
            OsConfigLogError(log, "CheckFileSystemMountingOption: mount directory '%s' and/or mount type '%s' not found in file '%s'", mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName);

            if (NULL != mountDirectory)
            {
                OsConfigCaptureReason(reason, "Found no entries about mount directory '%s' in file '%s' to look for option '%s'", mountDirectory, mountFileName, desiredOption);
            }

            if (NULL != mountType)
            {
                OsConfigCaptureReason(reason, "Found no entries about mount type '%s' in file '%s' to look for option '%s'", mountType, mountFileName, desiredOption);
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
            if (entry->d_name && strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
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
        OsConfigCaptureSuccessReason(reason, "'%s' not found to look for '%s'", fileName, text);
        result = 0;
    }
    else
    {
        if (EEXIST == (result = IsLineNotFoundOrCommentedOut(fileName, commentMark, text, reason, log)))
        {
            OsConfigCaptureReason(reason, "'%s' found in '%s' and it's not commented out with '%c'", text, fileName, commentMark);
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
    int value = -999;

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
    int result = -999;

    if (option && (0 == CheckFileExists(fileName, NULL, log)))
    {
        if (NULL == (contents = LoadStringFromFile(fileName, false, log)))
        {
            OsConfigLogError(log, "GetIntegerOptionFromFile: cannot read from '%s'", fileName);
        }
        else
        {
            if (-999 != (result = GetIntegerOptionFromBuffer(contents, option, separator, log)))
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
    int valueFromFile = -999;
    int i = 0;
    int result = ENOENT;

    if ((NULL == values) || (0 == numberOfValues))
    {
        OsConfigLogError(log, "CheckIntegerOptionFromFileEqualWithAny: invalid arguments (%p, %u)", values, numberOfValues);
        return EINVAL;
    }

    if (-999 != (valueFromFile = GetIntegerOptionFromFile(fileName, option, separator, log)))
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
    int valueFromFile = -999;
    int result = ENOENT;

    if (-999 != (valueFromFile = GetIntegerOptionFromFile(fileName, option, separator, log)))
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

int CheckLockoutForFailedPasswordAttempts(const char* fileName, char** reason, void* log)
{
    char* contents = NULL;
    char* buffer = NULL;
    char* value = NULL;
    int option = 0;
    int status = ENOENT;

    if (0 == CheckFileExists(fileName, reason, log))
    {
        if (NULL == (contents = LoadStringFromFile(fileName, false, log)))
        {
            OsConfigLogError(log, "CheckLockoutForFailedPasswordAttempts: cannot read from '%s'", fileName);
        }
        else
        {
            buffer = contents;

            // Example of valid lines: 
            //
            // auth required pam_tally2.so file=/var/log/tallylog deny=5 even_deny_root unlock_time=2000
            // auth required pam_faillock.so deny=1 even_deny_root unlock_time=300
            //
            // To pass, all attributes must be present, including either pam_faillock.so or pam_tally2.so, 
            // the deny value must be between 1 and 5 (inclusive), the unlock_time set to a positive value, 
            // with any number of spaces between.The even_deny_root and any other attribute like it are optional.
            //
            // There can be multiple 'auth' lines in the file. Only the right one matters.

            while (NULL != (value = GetStringOptionFromBuffer(buffer, "auth", ' ', log)))
            {
                if (((0 == strcmp("required", value)) && FreeAndReturnTrue(value)) &&
                    (((NULL != (value = GetStringOptionFromBuffer(buffer, "required", ' ', log))) && (0 == strcmp("pam_faillock.so", value)) && FreeAndReturnTrue(value)) ||
                    (((NULL != (value = GetStringOptionFromBuffer(buffer, "required", ' ', log))) && (0 == strcmp("pam_tally2.so", value)) && FreeAndReturnTrue(value)) &&
                    ((NULL != (value = GetStringOptionFromBuffer(buffer, "pam_tally2.so", ' ', log))) && (0 == strcmp("file=/var/log/tallylog", value)) && FreeAndReturnTrue(value)) &&
                    ((NULL != (value = GetStringOptionFromBuffer(buffer, "file", '=', log))) && (0 == strcmp("/var/log/tallylog", value)) && FreeAndReturnTrue(value)))) &&
                    (0 < (option = GetIntegerOptionFromBuffer(buffer, "deny", '=', log))) && (6 > option) && (0 < (option = GetIntegerOptionFromBuffer(buffer, "unlock_time", '=', log))))
                {
                    status = 0;
                    break;
                }
                else if (NULL == (buffer = strchr(buffer, EOL)))
                {
                    break;
                }
                else
                {
                    buffer += 1;
                }
            }

            FREE_MEMORY(contents);
        }
    }

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckLockoutForFailedPasswordAttempts: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);
        OsConfigCaptureSuccessReason(reason, "Valid lockout for failed password attempts line found in '%s'", fileName);
    }
    else
    {
        OsConfigLogInfo(log, "CheckLockoutForFailedPasswordAttempts: %s (%d)", PLAIN_STATUS_FROM_ERRNO(status), status);
        OsConfigCaptureReason(reason, "'%s' does not exist, or lockout for failed password attempts not set, "
            "'auth', 'pam_faillock.so' or 'pam_tally2.so' and 'file=/var/log/tallylog' not found, or 'deny' or "
            "'unlock_time' not found, or 'deny' not in between 1 and 5, or 'unlock_time' not set to greater than 0", fileName);
    }

    return status;
}