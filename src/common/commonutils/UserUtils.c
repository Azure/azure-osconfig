// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

//const char* commandTemplate = "sudo cat /etc/users | grep %s";

void FreeUsersList(struct passwd** source, unsigned int size)
{
    unsigned int i = 0;

    for (i = 0; i < size; i++)
    {
        if (NULL != source[i])
        {
            FREE_MEMORY(source[i]->pw_name);
            FREE_MEMORY(source[i]->pw_passwd);
            FREE_MEMORY(source[i]->pw_gecos);
            FREE_MEMORY(source[i]->pw_dir);
            FREE_MEMORY(source[i]->pw_shell);
        }
    }

    FREE_MEMORY(*source);
}

static int AllocateAndCopyPasswdEntry(struct passwd* destination, struct passwd* source, void* log)
{
    int status = 0;
    size_t length = 0;
    
    if ((NULL == destination) || (NULL == source))
    {
        OsConfigLogError(log, "AllocateAndCopyPasswdEntry: invalid arguments");
        return EINVAL;
    }

    if (0 < (length = source->pw_name ? strlen(source->pw_name) : 0))
    {
        if (NULL == (destination->pw_name = malloc(length + 1)))
        {
            OsConfigLogError(log, "AllocateAndCopyPasswdEntry: out of memory copying pw_name '%s'", source->pw_name);
            status = ENOMEM;
        }
        else
        {
            memset(destination->pw_name, 0, length + 1);
            memcopy(destination->pw_name, source->pw_name, length);
        }
    }

    if (0 < (length = source->pw_passwd ? strlen(source->pw_passwd) : 0))
    {
        if (NULL == (destination->pw_passwd = malloc(length + 1)))
        {
            OsConfigLogError(log, "AllocateAndCopyPasswdEntry: out of memory copying pw_passwd '%s'", source->pw_passwd);
            status = ENOMEM;
        }
        else
        {
            memset(destination->pw_passwd, 0, length + 1);
            memcopy(destination->pw_passwd, source->pw_passwd, length);
        }
    }

    destination->pw_uid = source->pw_uid;
    destination->pw_gid = source->pw_gid;

    if (0 < (length = source->pw_gecos ? strlen(source->pw_gecos) : 0))
    {
        if (NULL == (destination->pw_gecos = malloc(length + 1)))
        {
            OsConfigLogError(log, "AllocateAndCopyPasswdEntry: out of memory copying pw_gecos '%s'", source->pw_gecos);
            status = ENOMEM;
        }
        else
        {
            memset(destination->pw_gecos, 0, length + 1);
            memcopy(destination->pw_gecos, source->pw_gecos, length);
        }
    }

    if (0 < (length = source->pw_dir ? strlen(source->pw_dir) : 0))
    {
        if (NULL == (destination->pw_dir = malloc(length + 1)))
        {
            OsConfigLogError(log, "AllocateAndCopyPasswdEntry: out of memory copying pw_dir '%s'", source->pw_dir);
            status = ENOMEM;
        }
        else
        {
            memset(destination->pw_dir, 0, length + 1);
            memcopy(destination->pw_dir, source->pw_dir, length);
        }
    }

    if (0 < (length = source->pw_shell ? strlen(source->pw_shell) : 0))
    {
        if (NULL == (destination->pw_shell = malloc(length + 1)))
        {
            OsConfigLogError(log, "AllocateAndCopyPasswdEntry: out of memory copying pw_shell '%s'", source->pw_shell);
            status = ENOMEM;

        }
        else
        {
            memset(destination->pw_shell, 0, length + 1);
            memcopy(destination->pw_shell, source->pw_shell, length);
        }
    }

    return status;
}

int EnumerateUsers(struct passwd** passwd, unsigned int* size, void* log)
{
    struct passwd* passwdEntry = NULL;
    unsigned int i = 0;

    if ((NULL == passwd) || (NULL == size))
    {
        OsConfigLogError(log, "EnumerateUsers: invalid arguments");
        return EINVAL;
    }

    if (0 == (*size = GetNumberOfLinesInFile("/etc/passwd", log)))
    {
        OsConfigLogError(log, "EnumerateUsers: cannot read /etc/group");
        return NULL;
    }

    if (NULL == (*passwd = malloc(*size * sizeof(passwd))))
    {
        OsConfigLogError(log, "EnumerateUsers: out of memory");
        *size = 0;
        return ENOMEM;
    }

    setpwent();

    for (i = 0; i < *size; i++)
    {
        if (NULL == (passwdEntry = getpwent()))
        {
            // End of list
            break;
        }

        // Temporary
        OsConfigLogInfo(log, "[passwd %d] %s:%s:%d:%d:%s:%s:%s", passwdEntry->pw_name, passwdEntry->pw_passwd, passwdEntry->pw_uid, 
            passwdEntry->pw_gid, passwdEntry->pw_gecos, passwdEntry->pw_dir, passwdEntry->pw_shell);

        if (0 != (status = AllocateAndCopyPasswdEntry(&passwd[i], passwdEntry)))
        {
            OsConfigLogError(log, "EnumerateUsers: failed making copy of passwd entry (%d)", status);
            continue;
        }
    }

    endpwent();

    return status;
}