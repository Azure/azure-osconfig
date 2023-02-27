// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

//const char* commandTemplate = "sudo cat /etc/users | grep %s";

static void EmptyPasswd(struct passwd* target)
{
    if (NULL != target)
    {
        FREE_MEMORY(target->pw_name);
        FREE_MEMORY(target->pw_passwd);
        FREE_MEMORY(target->pw_gecos);
        FREE_MEMORY(target->pw_dir);
        FREE_MEMORY(target->pw_shell);

        memset(target, 0, sizeof(struct passwd));
    }
}

void FreeUsersList(struct passwd** source, unsigned int size)
{
    unsigned int i = 0;

    if (NULL != source)
    {
        for (i = 0; i < size; i++)
        {
            EmptyPasswd(&((*source)[i]));
        }

        FREE_MEMORY(*source);
    }
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

    EmptyPasswd(destination);

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

    if ((0 == status) && (0 < (length = source->pw_passwd ? strlen(source->pw_passwd) : 0)))
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

    if (0 == status)
    {
        destination->pw_uid = source->pw_uid;
        destination->pw_gid = source->pw_gid;
    }

    if ((0 == status) && (0 < (length = source->pw_gecos ? strlen(source->pw_gecos) : 0)))
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

    if ((0 == status) && (0 < (length = source->pw_dir ? strlen(source->pw_dir) : 0)))
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

    if ((0 == status) && (0 < (length = source->pw_shell ? strlen(source->pw_shell) : 0)))
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

    if (0 != status)
    {
        EmptyPasswd(destination);
    }

    return status;
}

int EnumerateUsers(struct passwd** passwdList, unsigned int* size, void* log)
{
    const char* passwdFile = "/etc/passwd";
    struct passwd* passwdEntry = NULL;
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

    if (0 != (*size = GetNumberOfLinesInFile(passwdFile, log)))
    {
        listSize = (*size) * sizeof(struct passwd);
        if (NULL != (*passwdList = malloc(listSize)))
        {
            memset(*passwdList, 0, listSize);

            setpwent();

            while ((NULL != (passwdEntry = getpwent())) && (i < *size))
            {
                if (0 != (status = CopyPasswdEntry(&((*passwdList)[i]), passwdEntry, log)))
                {
                    OsConfigLogError(log, "EnumerateUsers: failed making copy of passwd entry (%d)", status);
                    break;
                }

                i += 1;
            }
            endpwent();
        }
        else
        {
            OsConfigLogError(log, "EnumerateUsers: out of memory");
            *size = 0;
            status = ENOMEM;
        }
    }
    else
    {
        OsConfigLogError(log, "EnumerateUsers: cannot read %s", passwdFile);
        status = EPERM;
    }

    
    for (i = 0; i < *size; i++)
    {
        OsConfigLogInfo(log, "EnumerateUsers(user %u): name '%s', uid %d, gid %d, user info '%s', home dir '%s', shell '%s'", i, 
            (*passwdList)[i].pw_name, (*passwdList)[i].pw_uid, (*passwdList)[i].pw_gid, (*passwdList)[i].pw_gecos, (*passwdList)[i].pw_dir, (*passwdList)[i].pw_shell);
    }
    
    return status;
}