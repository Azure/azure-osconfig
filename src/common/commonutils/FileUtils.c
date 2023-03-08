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

bool DirectoryExists(const char* name)
{
    DIR* directory = NULL;
    bool result = false;

    if (FileExists(name) && (NULL != (directory = opendir(name))))
    {
        closedir(directory);
        result = true;
    }

    return result;
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

static int CheckAccess(bool directory, const char* name, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, bool rootCanOverwriteOwnership, void* log)
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
            if ((((uid_t)desiredOwnerId == statStruct.st_uid) && ((gid_t)desiredGroupId == statStruct.st_gid)) ||
                (directory && rootCanOverwriteOwnership && (0 == statStruct.st_uid) && (0 == statStruct.st_gid)))
            {
                currentMode = FilterFileAccessFlags(statStruct.st_mode);
                desiredMode = FilterFileAccessFlags(desiredAccess);

                if ((((desiredMode & S_IRWXU) == (currentMode & S_IRWXU)) || (0 == (desiredMode & S_IRWXU))) &&
                    (((desiredMode & S_IRWXG) == (currentMode & S_IRWXG)) || (0 == (desiredMode & S_IRWXG))) &&
                    (((desiredMode & S_IRWXO) == (currentMode & S_IRWXO)) || (0 == (desiredMode & S_IRWXO))))
                {
                    OsConfigLogInfo(log, "CheckAccess: access to '%s' (%u, %u, %u-%u) matches expected (%u, %u, %u-%u)",
                        name, statStruct.st_uid, statStruct.st_gid, statStruct.st_mode, currentMode,
                        desiredOwnerId, desiredGroupId, desiredAccess, desiredMode);
                    result = 0;
                }
                else
                {
                    OsConfigLogError(log, "CheckAccess: access to '%s' (%u-%u) does not match expected (%u-%u)",
                        name, statStruct.st_mode, currentMode, desiredAccess, desiredMode);
                    result = ENOENT;
                }
            }
            else
            {
                OsConfigLogError(log, "CheckAccess: ownership of '%s' (%u, %u) does not match expected (%u, %u)", 
                    name, statStruct.st_uid, statStruct.st_gid, desiredOwnerId, desiredGroupId);
                result = ENOENT;
            }
        }
        else
        {
            OsConfigLogError(log, "CheckAccess: stat('%s') failed with %d", name, errno);
        }
    }
    else
    {
        OsConfigLogInfo(log, "CheckAccess: '%s' not found, nothing to check", name);
        result = 0;
    }

    return result;
}

static int SetAccess(bool directory, const char* name, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, void* log)
{
    int result = ENOENT;

    if (NULL == name)
    {
        OsConfigLogError(log, "SetAccess called with an invalid name argument");
        return EINVAL;
    }

    if (directory ? DirectoryExists(name) : FileExists(name))
    {
        if (0 == (result = CheckAccess(directory, name, desiredOwnerId, desiredGroupId, desiredAccess, false, log)))
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

                if (0 == (result = chmod(name, desiredAccess)))
                {
                    OsConfigLogInfo(log, "SetAccess: successfully set '%s' access to %u", name, desiredAccess);
                    result = 0;
                }
                else
                {
                    OsConfigLogError(log, "SetAccess: chmod('%s', %d) failed with %d", name, desiredAccess, errno);
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

int CheckFileAccess(const char* name, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, void* log)
{
    return CheckAccess(false, name, desiredOwnerId, desiredGroupId, desiredAccess, false, log);
}

int SetFileAccess(const char* name, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, void* log)
{
    return SetAccess(false, name, desiredOwnerId, desiredGroupId, desiredAccess, log);
}

int CheckDirectoryAccess(const char* name, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, bool rootCanOverwriteOwnership, void* log)
{
    return CheckAccess(true, name, desiredOwnerId, desiredGroupId, desiredAccess, rootCanOverwriteOwnership, log);
}

int SetDirectoryAccess(const char* name, unsigned int desiredOwnerId, unsigned int desiredGroupId, unsigned int desiredAccess, void* log)
{
    return SetAccess(true, name, desiredOwnerId, desiredGroupId, desiredAccess, log);
}

int CheckFileSystemMountingOption(const char* mountFileName, const char* mountDirectory, const char* mountType, const char* desiredOption, void* log)
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
                    OsConfigLogInfo(log, "CheckFileSystemMountingOption: option '%s' for directory '%s' or mount type '%s' found in file '%s' at line '%d'", 
                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName, lineNumber);
                }
                else
                {
                    status = ENOENT;

                    OsConfigLogError(log, "CheckFileSystemMountingOption: option '%s' for directory '%s' or mount type '%s' missing from file '%s' at line %d",
                        desiredOption, mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName, lineNumber);
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
            OsConfigLogInfo(log, "CheckFileSystemMountingOption: directory '%s' or mount type '%s' not found in file '%s', nothing to check", 
                mountDirectory ? mountDirectory : "-", mountType ? mountType : "-", mountFileName);
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
    }

    return status;
}

static int CheckOrInstallPackage(const char* commandTemplate, const char* packageName, void* log)
{
    char* command = NULL;
    size_t packageNameLength = 0;
    int status = ENOENT;

    if ((NULL == commandTemplate) || (NULL == packageName) || ((0 == (packageNameLength = strlen(packageName)))))
    {
        OsConfigLogError(log, "CheckOrInstallPackage called with invalid arguments");
        return EINVAL;
    }

    packageNameLength += strlen(commandTemplate) + 1;

    if (NULL == (command = (char*)malloc(packageNameLength)))
    {
        OsConfigLogError(log, "CheckOrInstallPackage: out of memory");
        return ENOMEM;
    }

    memset(command, 0, packageNameLength);
    snprintf(command, packageNameLength, commandTemplate, packageName);

    status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log);

    FREE_MEMORY(command);

    return status;
}

int CheckPackageInstalled(const char* packageName, void* log)
{
    const char* commandTemplate = "dpkg -l %s | grep ^ii";
    int status = ENOENT;

    if (0 == (status = CheckOrInstallPackage(commandTemplate, packageName, log)))
    {
        OsConfigLogInfo(log, "CheckPackageInstalled: '%s' is installed", packageName);
    }
    else if (EINVAL != status)
    {
        OsConfigLogInfo(log, "CheckPackageInstalled: '%s' is not installed", packageName);
    }

    return status;
}

int InstallPackage(const char* packageName, void* log)
{
    const char* commandTemplate = "apt-get install -y %s";
    int status = 0;

    if (0 != (status = CheckPackageInstalled(packageName, log)))
    {
        if (0 == (status = CheckOrInstallPackage(commandTemplate, packageName, log)))
        {
            OsConfigLogInfo(log, "InstallPackage: '%s' was successfully installed", packageName);
        }
        else
        {
            OsConfigLogError(log, "InstallPackage: installation of '%s' failed with %d", packageName, status);
        }
    }
    else
    {
        OsConfigLogInfo(log, "InstallPackage: '%s' is already installed", packageName);
    }

    return status;
}

int UninstallPackage(const char* packageName, void* log)
{
    const char* commandTemplate = "apt-get remove -y --purge %s";
    int status = 0;

    if (0 == (status = CheckPackageInstalled(packageName, log)))
    {
        if (0 == (status = CheckOrInstallPackage(commandTemplate, packageName, log)))
        {
            OsConfigLogInfo(log, "UninstallPackage: '%s' was successfully uninstalled", packageName);
        }
        else
        {
            OsConfigLogError(log, "UninstallPackage: uninstallation of '%s' failed with %d", packageName, status);
        }
    }
    else if (EINVAL != status)
    {
        // Nothing to uninstall
        status = 0;
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

int FindTextInFile(const char* fileName, const char* text, void* log)
{
    const char* commandTemplate = "cat %s | grep %s";
    char* command = NULL;
    size_t commandLength = 0;
    int status = 0;

    if ((!FileExists(fileName)) || (NULL == text) || (0 == strlen(text)))
    {
        OsConfigLogError(log, "FindTextInFile called with invalid arguments");
        return EINVAL;
    }

    commandLength = strlen(commandTemplate) + strlen(fileName) + strlen(text) + 1;

    if (NULL == (command = malloc(commandLength + 1)))
    {
        OsConfigLogError(log, "FindTextInFile: out of memory");
        status = ENOMEM;
    }
    else
    {
        memset(command, 0, commandLength);
        snprintf(command, commandLength, commandTemplate, fileName, text);

        if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogInfo(log, "FindTextInFile: '%s' found in '%s'", text, fileName);
        }
        else
        {
            OsConfigLogError(log, "FindTextInFile: '%s' not found in '%s' (%d)", text, fileName, status);
            status = ENOENT;
        }

        FREE_MEMORY(command);
    }

    return status;
}

int FindTextInEnvironmentVariable(const char* variableName, const char* text, void* log)
{
    const char* commandTemplate = "echo $%s | grep %s";
    char* command = NULL;
    char* results = NULL;
    size_t commandLength = 0;
    int status = 0;

    if ((NULL == variableName) || (NULL == text) || (0 == strlen(variableName)) || (0 == strlen(text)))
    {
        OsConfigLogError(log, "FindCharacterInEnvironmentVariable called with invalid arguments");
        return EINVAL;
    }

    commandLength = strlen(commandTemplate) + strlen(variableName) + + strlen(text) + 1;

    if (NULL == (command = malloc(commandLength + 1)))
    {
        OsConfigLogError(log, "FindCharacterInEnvironmentVariable: out of memory");
        status = ENOMEM;
    }
    else
    {
        memset(command, 0, commandLength);
        snprintf(command, commandLength, commandTemplate, variableName, text);

        if (0 == (status = ExecuteCommand(NULL, command, true, false, 0, 0, &results, NULL, log)))
        {
            if (NULL != strstr(text, results))
            {
                OsConfigLogInfo(log, "FindTextInEnvironmentVariable: '%s' found in '%s'", text, variableName);
            }
            else
            {
                OsConfigLogInfo(log, "FindTextInEnvironmentVariable: '%s' not found in '%s'", text, variableName);
                status = ENOENT;
            }
        }
        else
        {
            OsConfigLogError(log, "FindTextInEnvironmentVariable: echo failed, %d", status);
        }

        FREE_MEMORY(results);
        FREE_MEMORY(command);
    }

    return status;
}