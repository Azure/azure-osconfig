// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"
#include "UserUtils.h"

#include <shadow.h>

#define MAX_GROUPS_USER_CAN_BE_IN 16
#define NUMBER_OF_SECONDS_IN_A_DAY 86400

static const char* g_root = "root";

static void ResetUserEntry(SIMPLIFIED_USER* target)
{
    if (NULL != target)
    {
        FREE_MEMORY(target->username);
        FREE_MEMORY(target->home);
        FREE_MEMORY(target->shell);

        target->userId = -1;
        target->groupId = -1;
        target->isRoot = false;
        target->isLocked = false;
        target->noLogin = false;
        target->cannotLogin = false;
        target->hasPassword = false;
        target->passwordEncryption = unknown;
        target->lastPasswordChange = 0;
        target->minimumPasswordAge = 0;
        target->maximumPasswordAge = 0;
        target->warningPeriod = 0;
        target->inactivityPeriod = 0;
        target->expirationDate = 0; 
    }
}

void FreeUsersList(SIMPLIFIED_USER** source, unsigned int size)
{
    unsigned int i = 0;

    if (NULL != source)
    {
        for (i = 0; i < size; i++)
        {
            ResetUserEntry(&((*source)[i]));
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

    ResetUserEntry(destination);

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

        destination->isRoot = ((0 == destination->userId) && (0 == destination->groupId)) ? true : false;
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
        ResetUserEntry(destination);
    }

    return status;
}

static char* EncryptionName(int type)
{
    char* name = NULL;
    switch (type)
    {
        case md5:
            name = "MD5";
            break;

        case blowfish:
            name = "Blowfish";
            break;

        case eksBlowfish:
            name = "Eksblowfish";
            break;

        case unknownBlowfish:
            name = "Unknown Blowfish";
            break;

        case sha256:
            name = "SHA-256";
            break;

        case sha512:
            name = "SHA-512";
            break;

        case unknown:
        default:
            name = "Unknown";
    }

    return name;
}

static bool IsNoLoginUser(SIMPLIFIED_USER* user)
{
    const char* noLoginShell[] = { "/usr/sbin/nologin", "/sbin/nologin", "/bin/false" };
    int index = ARRAY_SIZE(noLoginShell);
    bool noLogin = false;

    if (user && user->shell)
    {
        while (index > 0)
        {
            if (0 == strcmp(user->shell, noLoginShell[index - 1]))
            {
                noLogin = true;
                break;
            }

            index--;
        }
    }

    return noLogin;
}

static int CheckIfUserHasPassword(SIMPLIFIED_USER* user, void* log)
{
    struct spwd* shadowEntry = NULL;
    char control = 0;
    int status = 0;

    if ((NULL == user) || (NULL == user->username))
    {
        OsConfigLogError(log, "CheckIfUserHasPassword: invalid argument");
        return EINVAL;
    }

    if (true == (user->noLogin = IsNoLoginUser(user)))
    {
        return 0;
    }

    setspent();

    if (NULL != (shadowEntry = getspnam(user->username)))
    {
        control = shadowEntry->sp_pwdp ? shadowEntry->sp_pwdp[0] : 'n';

        switch (control)
        {
            case '$':
                switch (shadowEntry->sp_pwdp[1])
                {
                    case '1':
                        user->passwordEncryption = md5;
                        break;

                    case '2':
                        switch (shadowEntry->sp_pwdp[2])
                        {
                        case 'a':
                            user->passwordEncryption = blowfish;
                            break;

                        case 'y':
                            user->passwordEncryption = eksBlowfish;
                            break;

                        default:
                            user->passwordEncryption = unknownBlowfish;
                        }
                        break;

                    case '5':
                        user->passwordEncryption = sha256;
                        break;

                    case '6':
                        user->passwordEncryption = sha512;
                        break;

                    default:
                        user->passwordEncryption = unknown;
                }

                user->hasPassword = true;
                user->lastPasswordChange = shadowEntry->sp_lstchg;
                user->minimumPasswordAge = shadowEntry->sp_min;
                user->maximumPasswordAge = shadowEntry->sp_max;
                user->warningPeriod = shadowEntry->sp_warn;
                user->inactivityPeriod = shadowEntry->sp_inact;
                user->expirationDate = shadowEntry->sp_expire;
                break;

            case '!':
                user->isLocked = true;
                user->hasPassword = false;
                break;

            case '*':
                user->cannotLogin = true;
                user->hasPassword = false;
                break;

            case ':':
            default:
                OsConfigLogError(log, "CheckIfUserHasPassword: user '%s' (%u, %u) appears to be missing password ('%c')",
                    user->username, user->userId, user->groupId, control);
                user->hasPassword = false;
        }
    }
    else
    {
        OsConfigLogError(log, "CheckIfUserHasPassword: getspnam(%s) failed (%d)", user->username, errno);
        status = ENOENT;
    }

    endspent();

    return status;
}

int EnumerateUsers(SIMPLIFIED_USER** userList, unsigned int* size, void* log)
{
    const char* passwdFile = "/etc/passwd";

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

    if (0 != (*size = GetNumberOfLinesInFile(passwdFile)))
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
                else if (0 != (status = CheckIfUserHasPassword(&((*userList)[i]), log)))
                {
                    OsConfigLogError(log, "EnumerateUsers: failed checking user's login and password (%d)", status);
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
            OsConfigLogInfo(log, "EnumerateUserGroups(user '%s' (%u)) is in %d groups", user->username, user->groupId, numberOfGroups);
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
                        OsConfigLogInfo(log, "EnumerateUserGroups: user '%s' (%u) is in group '%s' (%u)", 
                            user->username, user->groupId, (*groupList)[i].groupName, (*groupList)[i].groupId);
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

    if (0 != (*size = GetNumberOfLinesInFile(groupFile)))
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
                            OsConfigLogInfo(log, "EnumerateAllGroups(group %d): group name '%s', gid %u, %s", i, 
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
                                OsConfigLogInfo(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: group '%s' (%u) of user '%s' (%u) found in /etc/group",
                                    userList[i].username, userList[i].userId, userGroupList[j].groupName, userGroupList[j].groupId);
                            }

                            found = true;
                            break;
                        }
                    }

                    if (false == found)
                    {
                        OsConfigLogError(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: group '%s' (%u) of user '%s' (%u) not found in /etc/group",
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
                        OsConfigLogError(log, "CheckNoDuplicateUserNamesExist: username '%s' appears more than a single time in /etc/passwd", userList[i].username);
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
                        OsConfigLogError(log, "CheckNoDuplicateGroupsExist: group name '%s' appears more than a single time in /etc/group", groupList[i].groupName);
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

int CheckAllUsersHavePasswordsSet(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].hasPassword)
            {
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user '%s' (%u, %u) appears to have a password set", 
                    userList[i].username, userList[i].userId, userList[i].groupId);
            }
            else if (userList[i].noLogin)
            {
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user '%s' (%u, %u) is no login", 
                    userList[i].username, userList[i].userId, userList[i].groupId);
            }
            else if (userList[i].isLocked)
            {
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user '%s' (%u, %u) is locked", 
                    userList[i].username, userList[i].userId, userList[i].groupId);
            }
            else if (userList[i].cannotLogin)
            {
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user '%s' (%u, %u) cannot login with password", 
                    userList[i].username, userList[i].userId, userList[i].groupId);
            }
            else
            {
                OsConfigLogError(log, "CheckAllUsersHavePasswordsSet: user '%s' (%u, %u) not found to have a password set", 
                    userList[i].username, userList[i].userId, userList[i].groupId);
                status = ENOENT;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: all users who need passwords appear to have passwords set");
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
                OsConfigLogError(log, "CheckNonRootAccountsHaveUniqueUidsGreaterThanZero: user '%s' (%u, %u) fails this check", 
                    userList[i].username, userList[i].userId, userList[i].groupId);
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

static int CheckNoLegacyPlusEntriesInFile(const char* fileName, void* log)
{
    int status = 0;

    if (FileExists(fileName) && CharacterFoundInFile(fileName, '+'))
    {
        OsConfigLogError(log, "CheckNoLegacyPlusEntriesInFile(%s): there are + lines in %s", fileName, fileName);
        status = ENOENT;
    }
    else
    {
        OsConfigLogInfo(log, "CheckNoLegacyPlusEntriesInFile(%s): there are no + lines in %s", fileName, fileName);
    }

    return status;
}

int CheckNoLegacyPlusEntriesInEtcPasswd(void* log)
{
    return CheckNoLegacyPlusEntriesInFile("etc/passwd", log);
}

int CheckNoLegacyPlusEntriesInEtcShadow(void* log)
{
    return CheckNoLegacyPlusEntriesInFile("etc/shadow", log);
}

int CheckNoLegacyPlusEntriesInEtcGroup(void* log)
{
    return CheckNoLegacyPlusEntriesInFile("etc/group", log);
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
                OsConfigLogError(log, "CheckRootIsOnlyUidZeroAccount: root has GID 0");
                status = EACCES;
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckRootIsOnlyUidZeroAccount: only the root group has GID 0");
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
            if (userList[i].noLogin)
            {
                continue;
            }
            else if ((NULL != userList[i].home) && (false == DirectoryExists(userList[i].home)))
            {
                OsConfigLogError(log, "CheckAllUsersHomeDirectoriesExist: user '%s' (%u, %u) home directory '%s' not found or is not a directory", 
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
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

static int CheckHomeDirectoryOwnership(SIMPLIFIED_USER* user, void* log)
{
    struct stat statStruct = {0};
    int status = 0;

    if ((NULL == user) || (NULL == user->home))
    {
        OsConfigLogError(log, "CheckHomeDirectoryOwnership called with an invalid argument");
        return EINVAL;
    }

    if (DirectoryExists(user->home))
    {
        if (0 == (status = stat(user->home, &statStruct)))
        {
            if (((uid_t)user->userId != statStruct.st_uid) || ((gid_t)user->groupId != statStruct.st_gid))
            {
                status = ENOENT;
            }
        }
        else
        {
            OsConfigLogError(log, "CheckHomeDirectoryOwnership: stat('%s') failed with %d", user->home, errno);
        }
    }
    else
    {
        OsConfigLogInfo(log, "CheckHomeDirectoryOwnership: directory '%s' not found, nothing to check", user->home);
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
            if (userList[i].noLogin)
            {
                continue;
            }
            else if (userList[i].home) 
            {
                if (userList[i].cannotLogin && (0 != CheckHomeDirectoryOwnership(&userList[i], log)))
                {
                    OsConfigLogInfo(log, "CheckUsersOwnTheirHomeDirectories: user '%s' (%u, %u) cannot login and their assigned home directory '%s' is owned by root",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                }
                else if (0 == CheckHomeDirectoryOwnership(&userList[i], log))
                {
                    OsConfigLogInfo(log, "CheckUsersOwnTheirHomeDirectories: user '%s' (%u, %u) owns their assigned home directory '%s'",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                }
                else
                {
                    OsConfigLogError(log, "CheckUsersOwnTheirHomeDirectories: user '%s' (%u, %u) does not own their assigned home directory '%s'",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                    status = ENOENT;
                }
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

int CheckRestrictedUserHomeDirectories(unsigned int mode, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((userList[i].noLogin) || (userList[i].isRoot))
            {
                continue;
            }
            else if (DirectoryExists(userList[i].home))
            {
                if (0 == (_status = CheckDirectoryAccess(userList[i].home, userList[i].userId, userList[i].groupId, mode, true, log)))
                {
                    OsConfigLogInfo(log, "CheckRestrictedUserHomeDirectories: user '%s' (%u, %u) has proper access (%u) set for their assigned home directory '%s'",
                        userList[i].username, userList[i].userId, userList[i].groupId, mode, userList[i].home);
                }
                else
                {
                    OsConfigLogError(log, "CheckRestrictedUserHomeDirectories: user '%s' (%u, %u) does not have proper access (%u) set for their assigned home directory '%s'",
                        userList[i].username, userList[i].userId, userList[i].groupId, mode, userList[i].home);
                    status = _status;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckRestrictedUserHomeDirectories: all users who have home directories have restricted access to them");
    }

    return status;
}

int CheckPasswordHashingAlgorithm(unsigned int algorithm, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else
            {
                if (algorithm == userList[i].passwordEncryption)
                {
                    OsConfigLogInfo(log, "CheckPasswordHashingAlgorithm: user '%s' (%u, %u) has a password encrypted with the proper algorithm %s (%d)",
                        userList[i].username, userList[i].userId, userList[i].groupId, EncryptionName(userList[i].passwordEncryption), userList[i].passwordEncryption);
                }
                else
                {
                    OsConfigLogError(log, "CheckRestrictedUserHomeDirectories: user '%s' (%u, %u) has a password encrypted with algorithm %d (%s) instead of %d (%s)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].passwordEncryption, EncryptionName(userList[i].passwordEncryption), 
                        algorithm, EncryptionName(algorithm));
                    status = ENOENT;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckPasswordHashingAlgorithm: all users who have passwords have them encrypted with hashing algorithm %s (%d)",
            EncryptionName(algorithm), algorithm);
    }

    return status;
}

int CheckMinDaysBetweenPasswordChanges(long days, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else
            {
                if (userList[i].minimumPasswordAge >= days)
                {
                    OsConfigLogInfo(log, "CheckMinDaysBetweenPasswordChanges: user '%s' (%u, %u) has a minimum time between password changes of %ld days (requested: %ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].minimumPasswordAge, days);
                }
                else
                {
                    OsConfigLogError(log, "CheckMinDaysBetweenPasswordChanges: user '%s' (%u, %u) minimum time between password changes of %ld days is less than requested %ld days",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].minimumPasswordAge, days);
                    status = ENOENT;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckMinDaysBetweenPasswordChanges: all users who have passwords have correct number of minimum days (%ld) between changes", days);
    }

    return status;
}

int CheckMaxDaysBetweenPasswordChanges(long days, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else
            {
                if (userList[i].maximumPasswordAge <= days)
                {
                    OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: user '%s' (%u, %u) has a maximum time between password changes of %ld days (requested: %ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge, days);
                }
                else
                {
                    OsConfigLogError(log, "CheckMaxDaysBetweenPasswordChanges: user '%s' (%u, %u) maximum time between password changes of %ld days is more than requested %ld days",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge, days);
                    status = ENOENT;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: all users who have passwords have correct number of maximum days (%ld) between changes", days);
    }

    return status;
}

int CheckPasswordExpirationWarning(long days, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else
            {
                if (userList[i].warningPeriod >= days)
                {
                    OsConfigLogInfo(log, "CheckPasswordExpirationWarning: user '%s' (%u, %u) has a password expiration warning time of %ld days (requested: %ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].warningPeriod, days);
                }
                else
                {
                    OsConfigLogError(log, "CheckPasswordExpirationWarning: user '%s' (%u, %u) password expiration warning time is %ld days, less than requested %ld days",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].warningPeriod, days);
                    status = ENOENT;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckPasswordExpirationWarning: all users who have passwords have correct number of maximum days (%ld) between changes", days);
    }

    return status;
}

int CheckUsersRecordedPasswordChangeDates(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    long timer = 0;
    int status = 0;
    long daysCurrent = time(&timer) / NUMBER_OF_SECONDS_IN_A_DAY;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else
            {
                if (userList[i].lastPasswordChange <= daysCurrent)
                {
                    OsConfigLogInfo(log, "CheckUsersRecordedPasswordChangeDates: user '%s' (%u, %u) has %lu days since last password change",
                        userList[i].username, userList[i].userId, userList[i].groupId, daysCurrent - userList[i].lastPasswordChange);
                }
                else
                {
                    OsConfigLogError(log, "CheckUsersRecordedPasswordChangeDates: user '%s' (%u, %u) last recorded password change is in the future (next %ld days)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].lastPasswordChange - daysCurrent);
                    status = ENOENT;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckUsersRecordedPasswordChangeDates: all users who have passwords have dates of last passord change in the past");
    }

    return status;
}

int CheckSystemAccountsAreNonLogin(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((userList[i].isLocked || userList[i].noLogin || userList[i].cannotLogin) && userList[i].hasPassword)
            {
                OsConfigLogError(log, "CheckSystemAccountsAreNonLogin: user '%s' (%u, %u, '%s', '%s') appears system but can login with a password",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home, userList[i].shell);
                status = ENOENT;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckSystemAccountsAreNonLogin: all system accounts are non-login");
    }

    return status;
}