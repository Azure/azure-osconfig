// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"
#include "UserUtils.h"

//const char* commandTemplate = "sudo cat /etc/users | grep %s";

static const char* g_passwdFile = "/etc/passwd";

static void EmptyUserEntry(SIMPLIFIED_USER* target)
{
    if (NULL != target)
    {
        FREE_MEMORY(target->username);
        FREE_MEMORY(target->home);
        FREE_MEMORY(target->shell);

        memset(target, 0, sizeof(SIMPLIFIED_GROUP));
    }
}

void FreeUsersList(SIMPLIFIED_USER** source, unsigned int size)
{
    unsigned int i = 0;

    if (NULL != source)
    {
        for (i = 0; i < size; i++)
        {
            EmptyUserEntry(&((*source)[i]));
        }

        FREE_MEMORY(*source);
    }
}

static int CopyUserEntry(SIMPLIFIED_USER* destination, struct passwd* source, void* log)
{
    int status = 0;
    size_t length = 0;
    
    if ((NULL == destination) || (NULL == source))
    {
        OsConfigLogError(log, "CopyPasswdEntry: invalid arguments");
        return EINVAL;
    }

    EmptyUserEntry(destination);

    if (0 < (length = (source->pw_name ? strlen(source->pw_name) : 0)))
    {
        if (NULL == (destination->username = malloc(length + 1)))
        {
            OsConfigLogError(log, "CopyPasswdEntry: out of memory copying pw_name '%s'", source->pw_name);
            status = ENOMEM;
        }
        else
        {
            memset(destination->username, 0, length + 1);
            memcpy(destination->username, source->pw_name, length);
        }
    }

    if (0 == status)
    {
        destination->userId = source->pw_uid;
        destination->groupId = source->pw_gid;
    }

    if ((0 == status) && (0 < (length = source->pw_dir ? strlen(source->pw_dir) : 0)))
    {
        if (NULL == (destination->home = malloc(length + 1)))
        {
            OsConfigLogError(log, "CopyPasswdEntry: out of memory copying pw_dir '%s'", source->pw_dir);
            status = ENOMEM;
        }
        else
        {
            memset(destination->home, 0, length + 1);
            memcpy(destination->home, source->pw_dir, length);
        }
    }

    if ((0 == status) && (0 < (length = source->pw_shell ? strlen(source->pw_shell) : 0)))
    {
        if (NULL == (destination->shell = malloc(length + 1)))
        {
            OsConfigLogError(log, "CopyPasswdEntry: out of memory copying pw_shell '%s'", source->pw_shell);
            status = ENOMEM;

        }
        else
        {
            memset(destination->shell, 0, length + 1);
            memcpy(destination->shell, source->pw_shell, length);
        }
    }

    if (0 != status)
    {
        EmptyUserEntry(destination);
    }

    return status;
}

int EnumerateUsers(SIMPLIFIED_USER** userList, unsigned int* size, void* log)
{
    struct passwd* userEntry = NULL;
    unsigned int i = 0;
    size_t listSize = 0;
    int status = 0;

    if ((NULL == userList) || (NULL == size))
    {
        OsConfigLogError(log, "EnumerateUsers: invalid arguments");
        return EINVAL;
    }

    *userList = NULL;
    *size = 0;

    if (0 != (*size = GetNumberOfLinesInFile(g_passwdFile, log)))
    {
        listSize = (*size) * sizeof(SIMPLIFIED_USER);
        if (NULL != (*userList = malloc(listSize)))
        {
            memset(*userList, 0, listSize);

            setpwent();

            while ((NULL != (userEntry = getpwent())) && (i < *size))
            {
                if (0 != (status = CopyUserEntry(&((*userList)[i]), userEntry, log)))
                {
                    OsConfigLogError(log, "EnumerateUsers: failed making copy of user entry (%d)", status);
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
        OsConfigLogError(log, "EnumerateUsers: cannot read %s", g_passwdFile);
        status = EPERM;
    }

    
    if (0 != status)
    {
        OsConfigLogError(log, "EnumerateUsers failed with %d", status);
    }
    else if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "EnumerateUsers: %u users found", *size);

        for (i = 0; i < *size; i++)
        {
            OsConfigLogInfo(log, "EnumerateUsers(user %u): name '%s', uid %d, gid %d, home '%s', shell '%s'", i, 
                (*userList)[i].username, (*userList)[i].userId, (*userList)[i].groupId, (*userList)[i].home, (*userList)[i].shell);
        }
    }

    return status;
}

void FreeGroupList(SIMPLIFIED_GROUP** groupList, unsigned int size)
{
    unsigned int i = 0;

    if (NULL != groupList)
    {
        for (i = 0; i < size; i++)
        {
            FREE_MEMORY(((*groupList)[i]).groupName);
        }

        FREE_MEMORY(*groupList);
    }
}

#define MAX_GROUPS_USER_CAN_BE_IN 16

int EnumerateUserGroups(SIMPLIFIED_USER* user, SIMPLIFIED_GROUP** groupList, unsigned int* size, void* log)
{
    gid_t groupIds[MAX_GROUPS_USER_CAN_BE_IN] = {0};
    int numberOfGroups = ARRAY_SIZE(groupIds);
    struct group* groupEntry = NULL;
    size_t groupNameLength = 0;
    int i = 0;
    int status = 0;

    if ((NULL == user) || (NULL == groupList) || (NULL == size))
    {
        OsConfigLogError(log, "EnumerateUserGroups: invalid arguments");
        return EINVAL;
    }

    *groupList = NULL;
    *size = 0;

    if (-1 == (numberOfGroups = getgrouplist(user->username, user->groupId, &groupIds[0], &numberOfGroups)))
    {
        OsConfigLogError(log, "EnumerateUserGroups: getgrouplist failed");
        status = ENOENT;
    }
    else if (NULL == (*groupList = malloc(sizeof(SIMPLIFIED_GROUP) * numberOfGroups)))
    {
        OsConfigLogError(log, "EnumerateUserGroups: out of memory");
        status = ENOMEM;
    }
    else
    {
        *size = numberOfGroups;

        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(log, "EnumerateUserGroups(user '%s' (uid: %u)) is in %d groups", user->username, user->groupId, numberOfGroups);
        }

        for (i = 0; i < numberOfGroups; i++)
        {
            if (NULL == (groupEntry = getgrgid(groupIds[i])))
            {
                OsConfigLogError(log, "EnumerateUserGroups: getgrgid(%u) failed", (unsigned int)groupIds[i]);
                status = ENOENT;
                break;
            }

            (*groupList)[i].groupId = groupEntry->gr_gid;
            (*groupList)[i].groupName = NULL;
            (*groupList)[i].hasUsers = true;

            if (0 < (groupNameLength = (groupEntry->gr_name ? strlen(groupEntry->gr_name) : 0)))
            {
                if (NULL != ((*groupList)[i].groupName = malloc(groupNameLength + 1)))
                {
                    memset((*groupList)[i].groupName, 0, groupNameLength + 1);
                    memcpy((*groupList)[i].groupName, groupEntry->gr_name, groupNameLength);

                    if (IsFullLoggingEnabled())
                    {
                        OsConfigLogInfo(log, "EnumerateUserGroups(user '%s' (uid: %u), group %d): group name '%s', gid %d", 
                           user->username, user->groupId, i, (*groupList)[i].groupName, (*groupList)[i].groupId);
                    }
                }
                else
                {
                    OsConfigLogError(log, "EnumerateUserGroups: out of memory (3)");
                    status = ENOMEM;
                    break;
                }
            }
        }
    }

    return status;
}

int EnumerateAllGroups(SIMPLIFIED_GROUP** groupList, unsigned int* size, void* log)
{
    const char* groupFile = "/etc/group";
    struct group* groupEntry = NULL;
    size_t groupNameLength = 0;
    size_t listSize = 0;
    unsigned int i = 0;
    int status = 0;

    if ((NULL == groupList) || (NULL == size))
    {
        OsConfigLogError(log, "EnumerateAllGroups: invalid arguments");
        return EINVAL;
    }

    *groupList = NULL;
    *size = 0;

    if (0 != (*size = GetNumberOfLinesInFile(groupFile, log)))
    {
        listSize = (*size) * sizeof(SIMPLIFIED_GROUP);
        if (NULL != (*groupList = malloc(listSize)))
        {
            memset(*groupList, 0, listSize);

            setgrent();

            while ((NULL != (groupEntry = getgrent())) && (i < *size))
            {
                (*groupList)[i].groupId = groupEntry->gr_gid;
                (*groupList)[i].groupName = NULL;
                (*groupList)[i].hasUsers = ((NULL != groupEntry->gr_mem) && (NULL != *(groupEntry->gr_mem)) && (0 != *(groupEntry->gr_mem)[0])) ? true : false;

                if (0 < (groupNameLength = (groupEntry->gr_name ? strlen(groupEntry->gr_name) : 0)))
                {
                    if (NULL != ((*groupList)[i].groupName = malloc(groupNameLength + 1)))
                    {
                        memset((*groupList)[i].groupName, 0, groupNameLength + 1);
                        memcpy((*groupList)[i].groupName, groupEntry->gr_name, groupNameLength);

                        if (IsFullLoggingEnabled())
                        {
                            OsConfigLogInfo(log, "EnumerateAllGroups(group %d): group name '%s', gid %d, %s", i, 
                                (*groupList)[i].groupName, (*groupList)[i].groupId, (*groupList)[i].hasUsers ? "has users" : "empty");
                        }
                    }
                    else
                    {
                        OsConfigLogError(log, "EnumerateAllGroups: out of memory (2)");
                        status = ENOMEM;
                        break;
                    }
                }
             
                i += 1;
            }

            endgrent();

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(log, "EnumerateAllGroups: found %u groups (expected %u)", i, *size);
            }

            *size = i;
        }
        else
        {
            OsConfigLogError(log, "EnumerateAllGroups: out of memory (1)");
            status = ENOMEM;
        }
    }
    else
    {
        OsConfigLogError(log, "EnumerateGroups: cannot read %s", groupFile);
        status = EPERM;
    }

    return status;
}