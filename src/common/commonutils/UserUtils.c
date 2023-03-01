// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"
#include "UserUtils.h"

//const char* commandTemplate = "sudo cat /etc/users | grep %s";

static const char* g_root = "root";
static const char* g_passwdFile = "/etc/passwd";
static const char* g_noLoginShell = "/usr/sbin/nologin";

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

int CheckAllEtcPasswdGroupsExistInEtcGroup(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    struct SIMPLIFIED_GROUP* userGroupList = NULL;
    unsigned int userGroupListSize = 0;
    struct SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0, j = 0, k = 0;
    bool found = false;
    int status = 0;

    if ((0 == (status = EnumerateUsers(&userList, &userListSize, log))) &&
        (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, log))))
    {
        for (i = 0; (i < userListSize) && (0 == status); i++)
        {
            if (0 == (status = EnumerateUserGroups(&userList[i], &userGroupList, &userGroupListSize, log)))
            {
                for (j = 0; (j < userGroupListSize) && (0 == status); j++)
                {
                    found = false;

                    for (k = 0; (k < groupListSize) && (0 == status); k++)
                    {
                        if (userGroupList[j].groupId == groupList[k].groupId)
                        {
                            if (IsFullLoggingEnabled())
                            {
                                OsConfigLogInfo(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: group %s (%u) of user %s (%u) found in /etc/group",
                                    userList[i].username, userList[i].userId, userGroupList[j].groupName, userGroupList[j].groupId);
                            }

                            found = true;
                            break;
                        }
                    }

                    if (false == found)
                    {
                        OsConfigLogError(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: group %s (%u) of user %s (%u) not found in /etc/group",
                            userList[i].username, userList[i].userId, userGroupList[j].groupName, userGroupList[j].groupId);
                        status = ENOENT;
                        break;
                    }
                }

                FreeGroupList(&userGroupList, userGroupListSize);
            }
        }
    }

    FreeUsersList(&userList, userListSize);
    FreeGroupList(&groupList, groupListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: all groups in /etc/passwd exist in /etc/group");
    }

    return status;
}

int CheckNoDuplicateUidsExist(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; (i < userListSize) && (0 == status); i++)
        {
            hits = 0;

            for (j = 0; (j < userListSize) && (0 == status); j++)
            {
                if (userList[i].userId == userList[j].userId)
                {
                    hits += 1;

                    if (hits > 1)
                    {
                        OsConfigLogError(log, "CheckNoDuplicateUidsExist: UID %u appears more than a single time in /etc/passwd", userList[i].userId);
                        status = EEXIST;
                        break;
                    }
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckNoDuplicateUidsExist: no duplicate UIDs exist in /etc/passwd");
    }

    return status;
}

int CheckNoDuplicateGidsExist(void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, log)))
    {
        for (i = 0; (i < groupListSize) && (0 == status); i++)
        {
            hits = 0;

            for (j = 0; (j < groupListSize) && (0 == status); j++)
            {
                if (groupList[i].groupId == groupList[j].groupId)
                {
                    hits += 1;

                    if (hits > 1)
                    {
                        OsConfigLogError(log, "CheckNoDuplicateGidsExist: GID %u appears more than a single time in /etc/group", groupList[i].groupId);
                        status = EEXIST;
                        break;
                    }
                }
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckNoDuplicateGidsExist: no duplicate GIDs exist in /etc/group");
    }

    return status;
}

int CheckNoDuplicateUserNamesExist(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; (i < userListSize) && (0 == status); i++)
        {
            hits = 0;

            for (j = 0; (j < userListSize) && (0 == status); j++)
            {
                if (0 == strcmp(userList[i].username, userList[j].username))
                {
                    hits += 1;

                    if (hits > 1)
                    {
                        OsConfigLogError(log, "CheckNoDuplicateUserNamesExist: username %s appears more than a single time in /etc/passwd", userList[i].username);
                        status = EEXIST;
                        break;
                    }
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckNoDuplicateUserNamesExist: no duplicate usernames exist in /etc/passwd");
    }

    return status;
}

int CheckNoDuplicateGroupsExist(void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, log)))
    {
        for (i = 0; (i < groupListSize) && (0 == status); i++)
        {
            hits = 0;

            for (j = 0; (j < groupListSize) && (0 == status); j++)
            {
                if (0 == strcmp(groupList[i].groupName, groupList[j].groupName))
                {
                    hits += 1;

                    if (hits > 1)
                    {
                        OsConfigLogError(log, "CheckNoDuplicateGroupsExist: group name %s appears more than a single time in /etc/group", groupList[i].groupName);
                        status = EEXIST;
                        break;
                    }
                }
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckNoDuplicateGroupsExist: no duplicate group names exist in /etc/group");
    }

    return status;
}

int CheckShadowGroupIsEmpty(void* log)
{
    const char* shadow = "shadow";
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0;
    bool found = false;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, log)))
    {
        for (i = 0; i < groupListSize; i++)
        {
            if ((0 == strcmp(groupList[i].groupName, shadow)) && (true == groupList[i].hasUsers))
            {
                OsConfigLogError(log, "CheckShadowGroupIsEmpty: group shadow (%u) is not empty", groupList[i].groupId);
                status = ENOENT;
                break;
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckShadowGroupIsEmpty: shadow group is %s", found ? "empty" : "not found");
    }

    return status;
}

int CheckRootGroupExists(void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0;
    bool found = false;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, log)))
    {
        for (i = 0; i < groupListSize; i++)
        {
            if ((0 == strcmp(groupList[i].groupName, g_root)) && (0 == groupList[i].groupId))
            {
                OsConfigLogInfo(log, "CheckRootGroupExists: root group exists with GID 0");
                found = true;
                break;
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (false == found)
    {
        OsConfigLogError(log, "CheckRootGroupExists: root group with GID 0 not found");
        status = ENOENT;
    }

    return status;
}

#define MAXIMUM_LINE_LENGTH 1024

int CheckUserHasPassword(SIMPLIFIED_USER* user, void* log)
{
    char* commandTemplate = "cat /etc/shadow | grep %s";

    char command[MAXIMUM_LINE_LENGTH] = {0};
    char* textResult = NULL;
    size_t offset = 0;
    char control = 0;
    int status = 0;

    if ((NULL == user) || (NULL == user->username))
    {
        OsConfigLogError(log, "CheckUserHasPassword: invalid argument");
        return EINVAL;
    }

    if ((NULL != user->shell) && (0 == strcmp(user->shell, g_noLoginShell)))
    {
        OsConfigLogInfo(log, "CheckUserHasPassword: user '%s' (%d) is set to not login", user->username, user->userId);
    }
    else
    {
        // Available APIs that return passwd structures from files such as /etc/shadow (fgetpwent and fgetpwent_r)
        // appear to have a bug and cannot always return the shadow entries for all users so instead we do this:

        snprintf(command, sizeof(command), commandTemplate, user->username);
        
        if (0 == (status = ExecuteCommand(NULL, command, true, false, 0, 0, &textResult, NULL, log)))
        {
            offset = strlen(user->username) + 1;

            if (NULL != textResult)
            {
                control = textResult[offset];
                
                switch (control)
                {
                    case '$':
                        OsConfigLogInfo(log, "CheckUserHasPassword: user '%s' (%d) appears to have a password set", user->username, user->userId);
                        break;

                    case '!':
                        OsConfigLogInfo(log, "CheckUserHasPassword: user '%s' (%d) account is locked ('!')", user->username, user->userId);
                        break;

                    case '*':
                        OsConfigLogInfo(log, "CheckUserHasPassword: user '%s' (%d) cannot login with password ('*')", user->username, user->userId);
                        break;

                    case ':':
                    default:
                        OsConfigLogError(log, "CheckUserHasPassword: user '%s' (%d) not found to have a password set ('%c')", user->username, user->userId, control);
                        status = ENOENT;
                }
            }
            else
            {
                OsConfigLogError(log, "CheckUserHasPassword: ExecuteCommand(%s) returned no data, cannot check if user '%s' (%d) has a password set",
                    command, user->username, user->userId);
                status = ENOENT;    
            } 

        }
        else
        {
            OsConfigLogError(log, "CheckUserHasPassword: ExecuteCommand(%s) failed with %d, cannot check if user '%s' (%d) has a password set",
                command, status, user->username, user->userId);
        }

        FREE_MEMORY(textResult);
    }

    return status;
} 

int CheckAllUsersHavePasswordsSet(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        // Parse the full user list to log all who do not have passwords
        for (i = 0; i < userListSize; i++)
        {
            if ((0 != (_status = CheckUserHasPassword(&userList[i], log))) && (0 == status))
            {
                status = _status;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: all users who need passwords appear to have passwords set");
    }
    else
    {
        OsConfigLogError(log, "CheckAllUsersHavePasswordsSet: not all users who need passwords appear to have passwords set");
    }

    return status;
}

int CheckNonRootAccountsHaveUniqueUidsGreaterThanZero(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (strcmp(userList[i].username, g_root) && (0 == userList[i].userId))
            {
                OsConfigLogError(log, "CheckNonRootAccountsHaveUniqueUidsGreaterThanZero: user '%s' (%d) fails this check", userList[i].username, userList[i].userId);
                status = EACCES;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckNonRootAccountsHaveUniqueUidsGreaterThanZero: all users who are not root have UIDs greater than 0");
    }

    return status;
}

int CheckNoLegacyPlusEntriesInEtcPasswd(void* log)
{
    char* command = "cat /etc/passwd | grep + ^ii";
    int status = 0;

    if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogInfo(log, "CheckNoLegacyPlusEntriesInEtcPasswd: there are no '+' entries in /etc/passwd");
    }
    else
    {
        OsConfigLogError(log, "CheckNoLegacyPlusEntriesInEtcPasswd: there are '+' entries in /etc/passwd");
    }

    return status;
}

int CheckNoLegacyPlusEntriesInEtcShadow(void* log)
{
    char* command = "cat /etc/shadow | grep + ^ii";
    int status = 0;

    if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogInfo(log, "CheckNoLegacyPlusEntriesInEtcPasswd: there are no '+' entries in /etc/shadow");
    }
    else
    {
        OsConfigLogError(log, "CheckNoLegacyPlusEntriesInEtcPasswd: there are '+' entries in /etc/shadow");
    }

    return status;
}

int CheckNoLegacyPlusEntriesInEtcGroup(void* log)
{
    char* command = "cat /etc/user | grep + ^ii";
    int status = 0;

    if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogInfo(log, "CheckNoLegacyPlusEntriesInEtcPasswd: there are no '+' entries in /etc/user");
    }
    else
    {
        OsConfigLogError(log, "CheckNoLegacyPlusEntriesInEtcPasswd: there are '+' entries in /etc/user");
    }

    return status;
}

int CheckDefaultRootAccountGroupIsGidZero(void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, log)))
    {
        for (i = 0; i < groupListSize; i++)
        {
            if ((0 == strcmp(groupList[i].groupName, g_root)) && groupList[i].groupId)
            {
                OsConfigLogError(log, "CheckDefaultRootAccountGroupIsGidZero: group '%s' is GID %u", groupList[i].groupName, groupList[i].groupId);
                status = EACCES;
                break;
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckDefaultRootAccountGroupIsGidZero: default root group is GID 0");
    }

    return status;
}


int CheckRootIsOnlyUidZeroAccount(void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, log)))
    {
        for (i = 0; i < groupListSize; i++)
        {
            if (strcmp(groupList[i].groupName, g_root) && (0 == groupList[i].groupId))
            {
                OsConfigLogError(log, "CheckRootIsOnlyUidZeroAccount: group '%s' has GID 0", groupList[i].groupName);
                status = EACCES;
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (0 == status)
    {
        OsConfigLogError(log, "CheckRootIsOnlyUidZeroAccount: only the root group has GID 0");
    }

    return status;

}

int CheckAllUsersHomeDirectoriesExist(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].shell && strcmp(userList[i].shell, g_noLoginShell))
            {
                continue;
            }
            else if ((NULL != userList[i].home) && (false == DirectoryExists(userList[i].home)))
            {
                OsConfigLogError(log, "CheckAllUsersHomeDirectoriesExist: user '%s' (%d) home directory '%s' not found or is not a directory", 
                    userList[i].username, userList[i].userId, userList[i].home);
                status = ENOENT;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckAllUsersHomeDirectoriesExist: all users who can login have home directories that exist");
    }

    return status;
}

int CheckUsersOwnTheirHomeDirectories(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].shell && strcmp(userList[i].shell, g_noLoginShell))
            {
                continue;
            }
            else if ((NULL != userList[i].home) && (0 == (status = CheckDirectoryOwnership(userList[i].home, userList[i].userId, log))))
            {
                OsConfigLogError(log, "CheckUsersOwnTheirHomeDirectories: user '%s' (%d) does not own their assigned home directory '%s'",
                    userList[i].username, userList[i].userId, userList[i].home);
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckUsersOwnTheirHomeDirectories: all users who can login own their home directories");
    }

    return status;
}