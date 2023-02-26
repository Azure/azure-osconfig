// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

//const char* commandTemplate = "sudo cat /etc/users | grep %s";

void FreeUsersList(struct passwd** source, unsigned int size)
{
    struct passwd* entry = NULL;
    unsigned int i = 0;

    printf("############## Free %u entries from %p\n", size, *source);

    if (NULL != source)
    {
        for (i = 0; i < size; i++)
        {
            entry = *source + (i * sizeof(struct passwd));
            if (NULL != entry)
            {
                FREE_MEMORY(entry->pw_name);
                FREE_MEMORY(entry->pw_passwd);
                FREE_MEMORY(entry->pw_gecos);
                FREE_MEMORY(entry->pw_dir);
                FREE_MEMORY(entry->pw_shell);
            }

            printf("############## Freed %u entry from %p\n", i, entry);
        }
    }

    FREE_MEMORY(source);
}

static int CopyPasswdEntry(struct passwd* destination, struct passwd* source, void* log)
{
    int status = 0;
    size_t length = 0;
    
    if ((NULL == destination) || (NULL == source))
    {
        OsConfigLogError(log, "CopyPasswdEntry: invalid arguments");
        return EINVAL;
    }

    if (0 < (length = (source->pw_name ? strlen(source->pw_name) : 0)))
    {
        if (NULL == (destination->pw_name = malloc(length + 1)))
        {
            OsConfigLogError(log, "CopyPasswdEntry: out of memory copying pw_name '%s'", source->pw_name);
            status = ENOMEM;
        }
        else
        {
            memset(destination->pw_name, 0, length + 1);
            memcpy(destination->pw_name, source->pw_name, length);
        }
    }
    else
    {
        destination->pw_name = NULL;    
    }

    if (0 < (length = source->pw_passwd ? strlen(source->pw_passwd) : 0))
    {
        if (NULL == (destination->pw_passwd = malloc(length + 1)))
        {
            OsConfigLogError(log, "CopyPasswdEntry: out of memory copying pw_passwd '%s'", source->pw_passwd);
            status = ENOMEM;
        }
        else
        {
            memset(destination->pw_passwd, 0, length + 1);
            memcpy(destination->pw_passwd, source->pw_passwd, length);
        }
    }
    else
    {
        destination->pw_passwd = NULL;    
    }

    destination->pw_uid = source->pw_uid;
    destination->pw_gid = source->pw_gid;

    if (0 < (length = source->pw_gecos ? strlen(source->pw_gecos) : 0))
    {
        if (NULL == (destination->pw_gecos = malloc(length + 1)))
        {
            OsConfigLogError(log, "CopyPasswdEntry: out of memory copying pw_gecos '%s'", source->pw_gecos);
            status = ENOMEM;
        }
        else
        {
            memset(destination->pw_gecos, 0, length + 1);
            memcpy(destination->pw_gecos, source->pw_gecos, length);
        }
    }
    else
    {
        destination->pw_gecos = NULL;    
    }

    if (0 < (length = source->pw_dir ? strlen(source->pw_dir) : 0))
    {
        if (NULL == (destination->pw_dir = malloc(length + 1)))
        {
            OsConfigLogError(log, "CopyPasswdEntry: out of memory copying pw_dir '%s'", source->pw_dir);
            status = ENOMEM;
        }
        else
        {
            memset(destination->pw_dir, 0, length + 1);
            memcpy(destination->pw_dir, source->pw_dir, length);
        }
    }
    else
    {
        destination->pw_dir = NULL;    
    }

    if (0 < (length = source->pw_shell ? strlen(source->pw_shell) : 0))
    {
        if (NULL == (destination->pw_shell = malloc(length + 1)))
        {
            OsConfigLogError(log, "CopyPasswdEntry: out of memory copying pw_shell '%s'", source->pw_shell);
            status = ENOMEM;

        }
        else
        {
            memset(destination->pw_shell, 0, length + 1);
            memcpy(destination->pw_shell, source->pw_shell, length);
        }
    }
    else
    {
        destination->pw_shell = NULL;    
    }

    OsConfigLogInfo(log, "Copied: %s:%s:%d:%d:%s:%s:%s", destination->pw_name, destination->pw_passwd, 
       destination->pw_uid, destination->pw_gid, destination->pw_gecos, destination->pw_dir, destination->pw_shell);

    return status;
}

int EnumerateUsers(struct passwd** passwdList, unsigned int* size, void* log)
{
    const char* passwdFile = "/etc/passwd";
    struct passwd* passwdEntry = NULL;
    struct passwd* nextEntry = NULL;
    unsigned int numEntries = 0;
    unsigned int i = 0;
    size_t listSize = 0;
    int status = 0;

    if ((NULL == passwdList) || (NULL == size))
    {
        OsConfigLogError(log, "EnumerateUsers: invalid arguments");
        return EINVAL;
    }

    *passwdList = NULL;
    *size = 0;

    if (0 == (numEntries = GetNumberOfLinesInFile(passwdFile, log)))
    {
        OsConfigLogError(log, "EnumerateUsers: cannot read %s", passwdFile);
        return EPERM;
    }

    OsConfigLogInfo(log, "EnumerateUsers: %u entries in %s", numEntries, passwdFile);
    listSize = numEntries * sizeof(struct passwd);
    OsConfigLogInfo(log, "EnumerateUsers: %u entries, %u entry size, total list size %u", numEntries, (unsigned int)sizeof(struct passwd), (unsigned int)listSize);

    if (NULL == (*passwdList = malloc(listSize)))
    {
        OsConfigLogError(log, "EnumerateUsers: out of memory");
        *size = 0;
        return ENOMEM;
    }

    memset(*passwdList, 0, listSize);

    nextEntry = *passwdList;
    for (i = 0; i < numEntries; i++)
    {
        OsConfigLogInfo(log, "### [%u]: %s:%s:%d:%d:%s:%s:%s", i, nextEntry->pw_name, nextEntry->pw_passwd, 
            nextEntry->pw_uid, nextEntry->pw_gid, nextEntry->pw_gecos, nextEntry->pw_dir, nextEntry->pw_shell);
        nextEntry += sizeof(struct passwd);
    }

    setpwent();

    i = 0;

    while ((NULL != (passwdEntry = getpwent())) && (i < *size))
    {
        nextEntry = (struct passwd*)((*passwdList) + (i * sizeof(struct passwd)));
        if (0 != (status = CopyPasswdEntry(nextEntry, passwdEntry, log)))
        {
            OsConfigLogError(log, "EnumerateUsers: failed making copy of passwd entry (%d)", status);
            break;
        }

        i += 1;
    }

    endpwent();

    for (i = 0; i < *size; i++)
    {
        nextEntry = (struct passwd*)((*passwdList) + (i * sizeof(struct passwd)));
        OsConfigLogInfo(log, "Listed entry %u: %s:%s:%d:%d:%s:%s:%s", i, nextEntry->pw_name, nextEntry->pw_passwd, 
            nextEntry->pw_uid, nextEntry->pw_gid, nextEntry->pw_gecos, nextEntry->pw_dir, nextEntry->pw_shell);
    }

    return status;
}