// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"
#include "UserUtils.h"

#include <shadow.h>

#define MAX_GROUPS_USER_CAN_BE_IN 32
#define NUMBER_OF_SECONDS_IN_A_DAY 86400

static const char* g_root = "root";
static const char* g_shadow = "shadow";
static const char* g_etcShadow = "/etc/shadow";
static const char* g_etcPasswd = "/etc/passwd";
static const char* g_etcPasswdDash = "/etc/passwd-";
static const char* g_etcShadowDash = "/etc/shadow-";

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
        OsConfigLogError(log, "CopyUserEntry: invalid arguments");
        return EINVAL;
    }

    ResetUserEntry(destination);

    if (0 < (length = (source->pw_name ? strlen(source->pw_name) : 0)))
    {
        if (NULL == (destination->username = malloc(length + 1)))
        {
            OsConfigLogError(log, "CopyUserEntry: out of memory copying pw_name '%s'", source->pw_name);
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
            OsConfigLogError(log, "CopyUserEntry: out of memory copying pw_dir '%s'", source->pw_dir);
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
            OsConfigLogError(log, "CopyUserEntry: out of memory copying pw_shell '%s'", source->pw_shell);
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
            name = "blowfish";
            break;

        case eksBlowfish:
            name = "eksblowfish";
            break;

        case unknownBlowfish:
            name = "unknown blowFish";
            break;

        case sha256:
            name = "SHA256";
            break;

        case sha512:
            name = "SHA512";
            break;

        case unknown:
        default:
            name = "unknown default";
    }

    return name;
}

static bool IsNoLoginUser(SIMPLIFIED_USER* user)
{
    const char* noLoginShell[] = {"/usr/sbin/nologin", "/sbin/nologin", "/bin/false", "/bin/true", "/usr/bin/true", "/usr/bin/false", "/dev/null", ""};

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
                user->hasPassword = false;
                user->isLocked = true;
                break;

            case '*':
                user->hasPassword = false;
                user->cannotLogin = true;
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
        OsConfigLogError(log, "CheckIfUserHasPassword: getspnam('%s') failed (%d)", user->username, errno);
        status = ENOENT;
    }

    endspent();

    return status;
}

int EnumerateUsers(SIMPLIFIED_USER** userList, unsigned int* size, char** reason, void* log)
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
        OsConfigCaptureReason(reason, "Failed to enumerate users (%d). User database may be corrupt. Automatic remediation is not possible", status);
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

int EnumerateUserGroups(SIMPLIFIED_USER* user, SIMPLIFIED_GROUP** groupList, unsigned int* size, char** reason, void* log)
{
    gid_t* groupIds = NULL;
    int numberOfGroups = MAX_GROUPS_USER_CAN_BE_IN;
    struct group* groupEntry = NULL;
    size_t groupNameLength = 0;
    int i = 0;
    int getGroupListResult = 0;
    int status = 0;

    if ((NULL == user) || (NULL == groupList) || (NULL == size))
    {
        OsConfigLogError(log, "EnumerateUserGroups: invalid arguments");
        return EINVAL;
    }
    else if (NULL == user->username)
    {
        OsConfigLogError(log, "EnumerateUserGroups: unable to enumerate groups for user without name");
        return ENOENT;
    }

    *groupList = NULL;
    *size = 0;

    if (NULL == (groupIds = malloc(numberOfGroups * sizeof(gid_t))))
    {
        OsConfigLogError(log, "EnumerateUserGroups: out of memory allocating list of %d group identifiers", numberOfGroups);
        numberOfGroups = 0;
        status = ENOMEM;
    }
    else if (-1 == (getGroupListResult = getgrouplist(user->username, user->groupId, groupIds, &numberOfGroups)))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(log, "EnumerateUserGroups: first call to getgrouplist for user '%s' (%u) returned %d and %d",
                user->username, user->groupId, getGroupListResult, numberOfGroups);
        }

        FREE_MEMORY(groupIds);

        if (0 < numberOfGroups)
        {
            if (NULL != (groupIds = malloc(numberOfGroups * sizeof(gid_t))))
            {
                getGroupListResult = getgrouplist(user->username, user->groupId, groupIds, &numberOfGroups);

                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(log, "EnumerateUserGroups: second call to getgrouplist for user '%s' (%u) returned %d and %d",
                        user->username, user->groupId, getGroupListResult, numberOfGroups);
                }
            }
            else
            {
                OsConfigLogError(log, "EnumerateUserGroups: out of memory allocating list of %d group identifiers", numberOfGroups);
                numberOfGroups = 0;
                status = ENOMEM;
            }
        }
        else
        {
            OsConfigLogError(log, "EnumerateUserGroups: first call to getgrouplist for user '%s' (%u) failed with -1 and returned %d groups",
                user->username, user->groupId, numberOfGroups);
            status = ENOENT;
        }
    }

    if ((0 == status) && (0 < numberOfGroups))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(log, "EnumerateUserGroups: user '%s' (%u) is in %d group%s", user->username, user->groupId, numberOfGroups, (1 == numberOfGroups) ? "" : "s");
        }

        if (NULL == (*groupList = malloc(sizeof(SIMPLIFIED_GROUP) * numberOfGroups)))
        {
            OsConfigLogError(log, "EnumerateUserGroups: out of memory");
            status = ENOMEM;
        }
        else
        {
            *size = numberOfGroups;

            for (i = 0; i < numberOfGroups; i++)
            {
                if (NULL == (groupEntry = getgrgid(groupIds[i])))
                {
                    OsConfigLogError(log, "EnumerateUserGroups: getgrgid(for gid: %u) failed", (unsigned int)groupIds[i]);
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
    }

    if (status)
    {
        OsConfigCaptureReason(reason, "Failed to enumerate groups for users (%d). User database may be corrupt. Automatic remediation is not possible", status);
    }

    FREE_MEMORY(groupIds);

    return status;
}

int EnumerateAllGroups(SIMPLIFIED_GROUP** groupList, unsigned int* size, char** reason, void* log)
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

    if (status)
    {
        OsConfigCaptureReason(reason, "Failed to enumerate user groups (%d). User group database may be corrupt. Automatic remediation is not possible", status);
    }

    return status;
}

int CheckAllEtcPasswdGroupsExistInEtcGroup(char** reason, void* log)
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

    if ((0 == (status = EnumerateUsers(&userList, &userListSize, reason, log))) &&
        (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, reason, log))))
    {
        for (i = 0; (i < userListSize) && (0 == status); i++)
        {
            if (0 == (status = EnumerateUserGroups(&userList[i], &userGroupList, &userGroupListSize, reason, log)))
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
                                OsConfigLogInfo(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: group '%s' (%u) of user '%s' (%u) found in '/etc/group'",
                                    userList[i].username, userList[i].userId, userGroupList[j].groupName, userGroupList[j].groupId);
                            }

                            found = true;
                            break;
                        }
                    }

                    if (false == found)
                    {
                        OsConfigLogError(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: group '%s' (%u) of user '%s' (%u) not found in '/etc/group'",
                            userList[i].username, userList[i].userId, userGroupList[j].groupName, userGroupList[j].groupId);
                        OsConfigCaptureReason(reason, "Group '%s' (%u) of user '%s' (%u) not found in '/etc/group'",
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
        OsConfigLogInfo(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: all groups in '/etc/passwd' exist in '/etc/group'");
        OsConfigCaptureSuccessReason(reason, "All user groups in '/etc/passwd' exist in '/etc/group'");
    }

    return status;
}

int SetAllEtcPasswdGroupsToExistInEtcGroup(void* log)
{
    const char* commandTemplate = "gpasswd -d %u %u";
    char* command = NULL;
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    struct SIMPLIFIED_GROUP* userGroupList = NULL;
    unsigned int userGroupListSize = 0;
    struct SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0, j = 0, k = 0;
    bool found = false;
    int status = 0, _status = 0;

    if ((0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log))) &&
        (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, NULL, log))))
    {
        for (i = 0; (i < userListSize) && (0 == status); i++)
        {
            if (0 == (status = EnumerateUserGroups(&userList[i], &userGroupList, &userGroupListSize, NULL, log)))
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
                                OsConfigLogInfo(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: group '%s' (%u) of user '%s' (%u) found in '/etc/group'",
                                    userGroupList[j].groupName, userGroupList[j].groupId, userList[i].username, userList[i].userId);
                            }

                            found = true;
                            break;
                        }
                    }

                    if (false == found)
                    {
                        OsConfigLogError(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: group '%s' (%u) of user '%s' (%u) not found in '/etc/group'",
                            userGroupList[j].groupName, userGroupList[j].groupId, userList[i].username, userList[i].userId);

                        if (NULL != (command = FormatAllocateString(commandTemplate, userList[i].userId, userGroupList[j].groupId)))
                        {
                            if (0 == (_status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
                            {
                                OsConfigLogError(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: user '%s' (%u) was removed from group '%s' (%u)",
                                    userList[i].username, userList[i].userId, userGroupList[j].groupName, userGroupList[j].groupId);
                            }
                            else
                            {
                                OsConfigLogError(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: 'gpasswd -d %u %u' failed with %d",
                                    userList[i].userId, userGroupList[j].groupId, _status);
                            }

                            FREE_MEMORY(command);
                        }
                        else
                        {
                            OsConfigLogError(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: out of memory");
                            _status = ENOMEM;
                        }
                    }

                    if (0 == status)
                    {
                        status = _status;
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
        OsConfigLogInfo(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: all groups in '/etc/passwd' now exist in '/etc/group'");
    }

    return status;
}

int CheckNoDuplicateUidsExist(char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
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
                        OsConfigLogError(log, "CheckNoDuplicateUidsExist: uid %u appears more than a single time in '/etc/passwd'", userList[i].userId);
                        OsConfigCaptureReason(reason, "Uid %u appears more than a single time in '/etc/passwd'", userList[i].userId);
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
        OsConfigLogInfo(log, "CheckNoDuplicateUidsExist: no duplicate uids exist in /etc/passwd");
        OsConfigCaptureSuccessReason(reason, "No duplicate uids exist in '/etc/passwd'");
    }

    return status;
}

static int RemoveUser(SIMPLIFIED_USER* user, void* log)
{
    const char* commandTemplate = "userdel -f -r %s";
    char* command = NULL;
    int status = 0, _status = 0;

    if (NULL == user)
    {
        OsConfigLogError(log, "RemoveUser: invalid argument");
        return EINVAL;
    }
    else if (0 == user->userId)
    {
        OsConfigLogError(log, "RemoveUser: cannot remove user with uid 0 ('%s' %u, %u)", user->username, user->userId, user->groupId);
        return EPERM;
    }

    if (NULL != (command = FormatAllocateString(commandTemplate, user->username)))
    {
        if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogInfo(log, "RemoveUser: removed user '%s' (%u, %u, '%s')", user->username, user->userId, user->groupId, user->home);

            if (DirectoryExists(user->home))
            {
                OsConfigLogError(log, "RemoveUser: home directory of user '%s' remains ('%s') and needs to be manually deleted", user->username, user->home);
            }
            else
            {
                OsConfigLogInfo(log, "RemoveUser: home directory of user '%s' successfully removed ('%s')", user->username, user->home);
            }
        }
        else
        {
            OsConfigLogError(log, "RemoveUser: failed to remove user '%s' (%u, %u) (%d)", user->username, user->userId, user->groupId, _status);
        }

        FREE_MEMORY(command);
    }
    else
    {
        OsConfigLogError(log, "RemoveUser: out of memory");
        status = ENOMEM;
    }

    return status;
}

int SetNoDuplicateUids(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            hits = 0;

            for (j = 0; j < userListSize; j++)
            {
                if (userList[i].userId == userList[j].userId)
                {
                    hits += 1;
                }
            }

            if (hits > 1)
            {
                OsConfigLogError(log, "SetNoDuplicateUids: user '%s' (%u) appears more than a single time in '/etc/passwd', deleting this user account",
                    userList[i].username, userList[i].userId);

                if ((0 != (_status = RemoveUser(&(userList[i]), log))) && (0 == status))
                {
                    status = _status;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetNoDuplicateUids: no duplicate uids exist in /etc/passwd");
    }

    return status;
}

int CheckNoDuplicateGidsExist(char** reason, void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, reason, log)))
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
                        OsConfigLogError(log, "CheckNoDuplicateGidsExist: gid %u appears more than a single time in '/etc/group'", groupList[i].groupId);
                        OsConfigCaptureReason(reason, "Gid %u appears more than a single time in '/etc/group'", groupList[i].groupId);
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
        OsConfigLogInfo(log, "CheckNoDuplicateGidsExist: no duplicate gids exist in '/etc/group'");
        OsConfigCaptureSuccessReason(reason, "No duplicate gids (group ids) exist in '/etc/group'");
    }

    return status;
}

static int RemoveGroup(SIMPLIFIED_GROUP* group, void* log)
{
    const char* commandTemplate = "groupdel -f %s";
    char* command = NULL;
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    unsigned int i = 0;
    int status = 0;

    if (NULL == group)
    {
        OsConfigLogError(log, "RemoveGroup: invalid argument");
        return EINVAL;
    }
    else if (0 == strcmp(g_root, group->groupName))
    {
        OsConfigLogError(log, "RemoveGroup: cannot remove root group");
        return EPERM;
    }

    if (group->hasUsers)
    {
        OsConfigLogInfo(log, "RemoveGroup: attempting to delete a group that has users ('%s', %u)", group->groupName, group->groupId);

        // Check if this group is the primary group of any existing user
        if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
        {
            for (i = 0; i < userListSize; i++)
            {
                if (userList[i].groupId == group->groupId)
                {
                    OsConfigLogError(log, "RemoveGroup: group '%s' (%u) is primary group of user '%s' (%u), try first to delete this user account",
                        group->groupName, group->groupId, userList[i].username, userList[i].userId);
                    RemoveUser(&(userList[i]), log);
                }
            }
        }

        FreeUsersList(&userList, userListSize);
    }

    if (NULL != (command = FormatAllocateString(commandTemplate, group->groupName)))
    {
        if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogInfo(log, "RemoveGroup: removed group '%s' (%u)", group->groupName, group->groupId);
        }
        else
        {
            OsConfigLogError(log, "RemoveGroup: failed to remove group '%s' (%u) (%d)", group->groupName, group->groupId, status);
        }

        FREE_MEMORY(command);
    }
    else
    {
        OsConfigLogError(log, "RemoveGroup: out of memory");
        status = ENOMEM;
    }

    return status;
}

int SetNoDuplicateGids(void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, NULL, log)))
    {
        for (i = 0; i < groupListSize; i++)
        {
            hits = 0;

            for (j = 0; j < groupListSize; j++)
            {
                if (groupList[i].groupId == groupList[j].groupId)
                {
                    hits += 1;
                }
            }

            if (hits > 1)
            {
                OsConfigLogError(log, "SetNoDuplicateGids: gid %u appears more than a single time in '/etc/group'", groupList[i].groupId);
                if ((0 != (_status = RemoveGroup(&(groupList[i]), log))) && (0 == status))
                {
                    status = _status;
                }
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetNoDuplicateGids: no duplicate gids exist in '/etc/group'");
    }

    return status;
}

int CheckNoDuplicateUserNamesExist(char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; (i < userListSize) && (0 == status); i++)
        {
            hits = 0;

            for (j = 0; (j < userListSize) && (0 == status); j++)
            {
                if (userList[i].username && userList[j].username && (0 == strcmp(userList[i].username, userList[j].username)))
                {
                    hits += 1;

                    if (hits > 1)
                    {
                        OsConfigLogError(log, "CheckNoDuplicateUserNamesExist: username '%s' appears more than a single time in '/etc/passwd'", userList[i].username);
                        OsConfigCaptureReason(reason, "Username '%s' appears more than a single time in '/etc/passwd'", userList[i].username);
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
        OsConfigLogInfo(log, "CheckNoDuplicateUserNamesExist: no duplicate usernames exist in '/etc/passwd'");
        OsConfigCaptureSuccessReason(reason, "No duplicate usernames exist in '/etc/passwd'");
    }

    return status;
}

int SetNoDuplicateUserNames(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            hits = 0;

            for (j = 0; j < userListSize; j++)
            {
                if (userList[i].username && userList[j].username && (0 == strcmp(userList[i].username, userList[j].username)))
                {
                    hits += 1;
                }
            }

            if (hits > 1)
            {
                OsConfigLogError(log, "SetNoDuplicateUserNames: username '%s' appears more than a single time in '/etc/passwd'", userList[i].username);

                if ((0 != (_status = RemoveUser(&(userList[i]), log))) && (0 == status))
                {
                    status = _status;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetNoDuplicateUserNames: no duplicate usernames exist in '/etc/passwd'");
    }

    return status;
}

int CheckNoDuplicateGroupNamesExist(char** reason, void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, reason, log)))
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
                        OsConfigLogError(log, "CheckNoDuplicateGroupNamesExist: group name '%s' appears more than a single time in '/etc/group'", groupList[i].groupName);
                        OsConfigCaptureReason(reason, "Group name '%s' appears more than a single time in '/etc/group'", groupList[i].groupName);
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
        OsConfigLogInfo(log, "CheckNoDuplicateGroupNamesExist: no duplicate group names exist in '/etc/group'");
        OsConfigCaptureSuccessReason(reason, "No duplicate group names exist in '/etc/group'");
    }

    return status;
}

int SetNoDuplicateGroupNames(void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0, j = 0;
    unsigned int hits = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, NULL, log)))
    {
        for (i = 0; i < groupListSize; i++)
        {
            hits = 0;

            for (j = 0; j < groupListSize; j++)
            {
                if (groupList[i].groupId == groupList[j].groupId)
                {
                    hits += 1;
                }
            }

            if (hits > 1)
            {
                OsConfigLogError(log, "SetNoDuplicateGroupNames: group name '%s' appears more than a single time in '/etc/group'", groupList[i].groupName);
                if ((0 != (_status = RemoveGroup(&(groupList[i]), log))) && (0 == status))
                {
                    status = _status;
                }
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetNoDuplicateGroupNames: no duplicate group names exist in '/etc/group'");
    }

    return status;
}

int CheckShadowGroupIsEmpty(char** reason, void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0;
    bool found = false;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, reason, log)))
    {
        for (i = 0; i < groupListSize; i++)
        {
            if ((0 == strcmp(groupList[i].groupName, g_shadow)) && (true == groupList[i].hasUsers))
            {
                OsConfigLogError(log, "CheckShadowGroupIsEmpty: group 'shadow' (%u) is not empty", groupList[i].groupId);
                OsConfigCaptureReason(reason, "Group 'shadow' is not empty: %u", groupList[i].groupId);
                status = ENOENT;
                break;
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckShadowGroupIsEmpty: shadow group is %s", found ? "empty" : "not found");
        OsConfigCaptureSuccessReason(reason, "The 'shadow' group is %s", found ? "empty" : "not found");
    }

    return status;
}

int SetShadowGroupEmpty(void* log)
{
    const char* commandTemplate = "gpasswd -d %s %s";
    char* command = NULL;
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    struct SIMPLIFIED_GROUP* userGroupList = NULL;
    unsigned int userGroupListSize = 0;
    unsigned int i = 0, j = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (0 == (status = EnumerateUserGroups(&userList[i], &userGroupList, &userGroupListSize, NULL, log)))
            {
                for (j = 0; j < userGroupListSize; j++)
                {
                    if (0 == strcmp(userGroupList[j].groupName, g_shadow))
                    {
                        OsConfigLogInfo(log, "SetShadowGroupEmpty: user '%s' (%u) is a member of group '%s' (%u)",
                            userList[i].username, userList[i].userId, g_shadow, userGroupList[j].groupId);

                        if (NULL != (command = FormatAllocateString(commandTemplate, userList[i].username, g_shadow)))
                        {
                            if (0 == (_status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
                            {
                                OsConfigLogError(log, "SetShadowGroupEmpty: user '%s' (%u) was removed from group '%s' (%u)",
                                    userList[i].username, userList[i].userId, userGroupList[j].groupName, userGroupList[j].groupId);
                            }
                            else
                            {
                                OsConfigLogError(log, "SetShadowGroupEmpty: 'gpasswd -d %s %s' failed with %d", userList[i].username, g_shadow, _status);
                            }

                            FREE_MEMORY(command);
                        }
                        else
                        {
                            OsConfigLogError(log, "SetShadowGroupEmpty: out of memory");
                            _status = ENOMEM;
                        }

                        if (_status && (0 == status))
                        {
                            status = _status;
                        }
                    }
                }

                FreeGroupList(&userGroupList, userGroupListSize);
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetShadowGroupEmpty: the 'shadow' group is empty");
    }

    return status;
}

int CheckRootGroupExists(char** reason, void* log)
{
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0;
    bool found = false;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, reason, log)))
    {
        for (i = 0; i < groupListSize; i++)
        {
            if ((0 == strcmp(groupList[i].groupName, g_root)) && (0 == groupList[i].groupId))
            {
                OsConfigLogInfo(log, "CheckRootGroupExists: root group exists with gid 0");
                OsConfigCaptureSuccessReason(reason, "Root group exists with gid 0");
                found = true;
                break;
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (false == found)
    {
        OsConfigLogError(log, "CheckRootGroupExists: root group with gid 0 not found");
        OsConfigCaptureReason(reason, "Root group with gid 0 not found");
        status = ENOENT;
    }

    return status;
}

int RepairRootGroup(void* log)
{
    const char* etcGroup = "/etc/group";
    const char* rootLine = "root:x:0:\n";
    const char* tempFileName = "/etc/~group";
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int groupListSize = 0;
    unsigned int i = 0;
    bool found = false;
    char* original = NULL;
    int status = 0;

    if (0 == (status = EnumerateAllGroups(&groupList, &groupListSize, NULL, log)))
    {
        for (i = 0; i < groupListSize; i++)
        {
            if ((0 == strcmp(groupList[i].groupName, g_root)) && (0 == groupList[i].groupId))
            {
                OsConfigLogInfo(log, "CheckRootGroupExists: root group exists with gid 0");
                found = true;
                break;
            }
        }
    }

    FreeGroupList(&groupList, groupListSize);

    if (false == found)
    {
        // Load content of /etc/group
        if (NULL != (original = LoadStringFromFile(etcGroup, false, log)))
        {
            // Save content loaded from /etc/group to temporary file
            if (SavePayloadToFile(tempFileName, rootLine, strlen(rootLine), log))
            {
                // Delete from temporary file any lines containing "root"
                if (0 == (status = ReplaceMarkedLinesInFile(tempFileName, g_root, NULL, '#', false, log)))
                {
                    // Free the previously loaded content, we'll reload
                    FREE_MEMORY(original);

                    // Load the fixed content of temporary file
                    if (NULL != (original = LoadStringFromFile(tempFileName, false, log)))
                    {
                        // Delete the previously created temporary file, we'll recreate
                        remove(tempFileName);

                        // Save correct root line to the recreated temporary file
                        if (SavePayloadToFile(tempFileName, rootLine, strlen(rootLine), log))
                        {
                            // Append to temporary file the cleaned content
                            if (AppendPayloadToFile(tempFileName, original, strlen(original), log))
                            {
                                // In a single atomic operation move edited contents from temporary file to /etc/group
                                if (0 != (status = RenameFileWithOwnerAndAccess(tempFileName, etcGroup, log)))
                                {
                                    OsConfigLogError(log, "RepairRootGroup:  RenameFileWithOwnerAndAccess('%s' to '%s') failed with %d",
                                        tempFileName, etcGroup, status);
                                }
                            }
                            else
                            {
                                OsConfigLogError(log, "RepairRootGroup: failed appending to to temp file '%s", tempFileName);
                                status = ENOENT;
                            }

                            remove(tempFileName);
                        }
                    }
                    else
                    {
                        OsConfigLogError(log, "RepairRootGroup: failed reading '%s", tempFileName);
                        status = EACCES;
                    }
                }
                else
                {
                    OsConfigLogError(log, "RepairRootGroup: failed removing potentially corrupted root entries from '%s' ", etcGroup);
                }
            }
            else
            {
                OsConfigLogError(log, "RepairRootGroup: failed saving to temp file '%s", tempFileName);
                status = EPERM;
            }

            FREE_MEMORY(original);
        }
        else
        {
            OsConfigLogError(log, "RepairRootGroup: failed reading '%s", etcGroup);
            status = EACCES;
        }
    }

    if (0 == status)
    {
        OsConfigLogInfo(log, "RepairRootGroup: root group exists with gid 0");
    }

    return status;
}

int CheckAllUsersHavePasswordsSet(char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
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
                OsConfigCaptureReason(reason, "User '%s' (%u, %u) not found to have a password set",
                    userList[i].username, userList[i].userId, userList[i].groupId);
                status = ENOENT;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: all users who need passwords appear to have passwords set");
        OsConfigCaptureSuccessReason(reason, "All users who need passwords appear to have passwords set");
    }

    return status;
}

int RemoveUsersWithoutPasswords(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].hasPassword)
            {
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user '%s' (%u, %u) appears to have a password set",
                    userList[i].username, userList[i].userId, userList[i].groupId);
            }
            else if (userList[i].noLogin)
            {
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user '%s' (%u, %u) is no login",
                    userList[i].username, userList[i].userId, userList[i].groupId);
            }
            else if (userList[i].isLocked)
            {
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user '%s' (%u, %u) is locked",
                    userList[i].username, userList[i].userId, userList[i].groupId);
            }
            else if (userList[i].cannotLogin)
            {
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user '%s' (%u, %u) cannot login with password",
                    userList[i].username, userList[i].userId, userList[i].groupId);
            }
            else
            {
                OsConfigLogError(log, "RemoveUsersWithoutPasswords: user '%s' (%u, %u) can login and has no password set",
                    userList[i].username, userList[i].userId, userList[i].groupId);

                if (0 == userList[i].userId)
                {
                    OsConfigLogError(log, "RemoveUsersWithoutPasswords: the root account's password must be manually fixed");
                    status = EPERM;
                }
                else if ((0 != (_status = RemoveUser(&(userList[i]), log))) && (0 == status))
                {
                    status = _status;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: all users who need passwords have passwords set");
    }

    return status;
}

int CheckRootIsOnlyUidZeroAccount(char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (((NULL == userList[i].username) || (0 != strcmp(userList[i].username, g_root))) && (0 == userList[i].userId))
            {
                OsConfigLogError(log, "CheckRootIsOnlyUidZeroAccount: user '%s' (%u, %u) is not root but has uid 0",
                    userList[i].username, userList[i].userId, userList[i].groupId);
                OsConfigCaptureReason(reason, "User '%s' (%u, %u) is not root but has uid 0",
                    userList[i].username, userList[i].userId, userList[i].groupId);
                status = EACCES;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckRootIsOnlyUidZeroAccount: all users who are not root have uids (user ids) greater than 0");
        OsConfigCaptureSuccessReason(reason, "All users who are not root have uids (user ids) greater than 0");
    }

    return status;
}

int SetRootIsOnlyUidZeroAccount(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (((NULL == userList[i].username) || (0 != strcmp(userList[i].username, g_root))) && (0 == userList[i].userId))
            {
                OsConfigLogError(log, "SetRootIsOnlyUidZeroAccount: user '%s' (%u, %u) is not root but has uid 0",
                    userList[i].username, userList[i].userId, userList[i].groupId);

                if ((0 != (_status = RemoveUser(&(userList[i]), log))) && (0 == status))
                {
                    status = _status;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetRootIsOnlyUidZeroAccount: all users who are not root have uids (user ids) greater than 0");
    }

    return status;
}

int CheckDefaultRootAccountGroupIsGidZero(char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0;
    unsigned int i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((0 == strcmp(userList[i].username, g_root)) && (0 == userList[i].userId) && (0 != userList[i].groupId))
            {
                OsConfigLogError(log, "CheckDefaultRootAccountuserIsGidZero: root user '%s' (%u) has default gid %u instead of gid 0",
                    userList[i].username, userList[i].userId, userList[i].groupId);
                OsConfigCaptureReason(reason, "Root user '%s' (%u) has default gid %u instead of gid 0",
                    userList[i].username, userList[i].userId, userList[i].groupId);
                status = EPERM;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckDefaultRootAccountGroupIsGidZero: default root group is gid 0");
        OsConfigCaptureSuccessReason(reason, "Default root group is gid 0");
    }

    return status;
}

int SetDefaultRootAccountGroupIsGidZero(void* log)
{
    int status = 0;

    if (0 != (status = CheckDefaultRootAccountGroupIsGidZero(NULL, log)))
    {
        status = RepairRootGroup(log);
    }

    return status;
}

int CheckAllUsersHomeDirectoriesExist(char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].noLogin || userList[i].cannotLogin || userList[i].isLocked)
            {
                continue;
            }
            else if ((NULL != userList[i].home) && (false == DirectoryExists(userList[i].home)))
            {
                OsConfigLogError(log, "CheckAllUsersHomeDirectoriesExist: user '%s' (%u, %u) home directory '%s' not found or is not a directory",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                OsConfigCaptureReason(reason, "User '%s' (%u, %u) home directory '%s' not found or is not a directory",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                status = ENOENT;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckAllUsersHomeDirectoriesExist: all users who can login have home directories that exist");
        OsConfigCaptureSuccessReason(reason, "All users who can login have home directories that exist");
    }

    return status;
}

int SetUserHomeDirectories(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    unsigned int defaultHomeDirAccess = 750;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].noLogin || userList[i].cannotLogin || userList[i].isLocked)
            {
                continue;
            }
            else if (NULL != userList[i].home)
            {
                // If the home directory does not exist, create it
                if (false == DirectoryExists(userList[i].home))
                {
                    OsConfigLogError(log, "SetUserHomeDirectories: user '%s' (%u, %u) home directory '%s' not found",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);

                    if (0 == (_status = mkdir(userList[i].home, defaultHomeDirAccess)))
                    {
                        OsConfigLogInfo(log, "SetUserHomeDirectories: user '%s' (%u, %u) has now home directory '%s'",
                            userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                    }
                    else
                    {
                        _status = (0 == errno) ? EACCES : errno;
                        OsConfigLogError(log, "SetUserHomeDirectories: cannot create home directory '%s' for user '%s' (%u, %u) (%d)",
                            userList[i].home, userList[i].username, userList[i].userId, userList[i].groupId, _status);
                    }
                }

                // If the home directory does not have correct ownership and access, correct this
                if (true == DirectoryExists(userList[i].home))
                {
                    if (0 != (_status = SetDirectoryAccess(userList[i].home, userList[i].userId, userList[i].groupId, defaultHomeDirAccess, log)))
                    {
                        OsConfigLogError(log, "SetUserHomeDirectories: failed to set access and ownership for home directory '%s' of user '%s' (%u, %u) (%d)",
                            userList[i].home, userList[i].username, userList[i].userId, userList[i].groupId, _status);
                    }
                }

                if (_status && (0 != status))
                {
                    status = _status;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetUserHomeDirectories: all users who can login have home directories that exist, have correct ownership, and access");
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
        OsConfigLogInfo(log, "CheckHomeDirectoryOwnership: directory '%s' is not found, nothing to check", user->home);
    }

    return status;
}

int CheckUsersOwnTheirHomeDirectories(char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].noLogin || userList[i].cannotLogin || userList[i].isLocked)
            {
                continue;
            }
            else if (DirectoryExists(userList[i].home))
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
                    OsConfigCaptureReason(reason, "User '%s' (%u, %u) does not own their assigned home directory '%s'",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                    status = ENOENT;
                }
            }
            else
            {
                OsConfigLogError(log, "CheckUsersOwnTheirHomeDirectories: user '%s' (%u, %u) assigned home directory '%s' does not exist",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                OsConfigCaptureReason(reason, "User '%s' (%u, %u) assigned home directory '%s' does not exist",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                status = ENOENT;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckUsersOwnTheirHomeDirectories: all users who can login own their home directories");
        OsConfigCaptureSuccessReason(reason, "All users who can login own their home directories");
    }

    return status;
}

int CheckRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0, j = 0;
    bool oneGoodMode = false;
    int status = 0;

    if ((NULL == modes) || (0 == numberOfModes))
    {
        OsConfigLogError(log, "CheckRestrictedUserHomeDirectories: invalid arguments (%p, %u)", modes, numberOfModes);
        return EINVAL;
    }

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].noLogin || userList[i].cannotLogin || userList[i].isLocked)
            {
                continue;
            }
            else if (DirectoryExists(userList[i].home))
            {
                oneGoodMode = false;

                for (j = 0; j < numberOfModes; j++)
                {
                    if (0 == CheckDirectoryAccess(userList[i].home, userList[i].userId, userList[i].groupId, modes[j], true, NULL, log))
                    {
                        OsConfigLogInfo(log, "CheckRestrictedUserHomeDirectories: user '%s' (%u, %u) has proper restricted access (%u) for their assigned home directory '%s'",
                            userList[i].username, userList[i].userId, userList[i].groupId, modes[j], userList[i].home);
                        oneGoodMode = true;
                        break;
                    }
                }

                if (false == oneGoodMode)
                {
                    OsConfigLogError(log, "CheckRestrictedUserHomeDirectories: user '%s' (%u, %u) does not have proper restricted access for their assigned home directory '%s'",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                    OsConfigCaptureReason(reason, "User '%s' (%u, %u) does not have proper restricted access for their assigned home directory '%s'",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);

                    if (0 == status)
                    {
                        status = ENOENT;
                    }
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckRestrictedUserHomeDirectories: all users who can login and have home directories have restricted access to them");
        OsConfigCaptureSuccessReason(reason, "All users who can login and have home directories have restricted access to them");
    }

    return status;
}

int SetRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, unsigned int modeForRoot, unsigned int modeForOthers, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0, j = 0;
    bool oneGoodMode = false;
    int status = 0, _status = 0;

    if ((NULL == modes) || (0 == numberOfModes))
    {
        OsConfigLogError(log, "SetRestrictedUserHomeDirectories: invalid arguments (%p, %u)", modes, numberOfModes);
        return EINVAL;
    }

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].noLogin || userList[i].cannotLogin || userList[i].isLocked)
            {
                continue;
            }
            else if (DirectoryExists(userList[i].home))
            {
                oneGoodMode = false;

                for (j = 0; j < numberOfModes; j++)
                {
                    if (0 == CheckDirectoryAccess(userList[i].home, userList[i].userId, userList[i].groupId, modes[j], true, NULL, log))
                    {
                        OsConfigLogInfo(log, "SetRestrictedUserHomeDirectories: user '%s' (%u, %u) already has proper restricted access (%u) for their assigned home directory '%s'",
                            userList[i].username, userList[i].userId, userList[i].groupId, modes[j], userList[i].home);
                        oneGoodMode = true;
                        break;
                    }
                }

                if (false == oneGoodMode)
                {
                    if (0 == (_status = SetDirectoryAccess(userList[i].home, userList[i].userId, userList[i].groupId, userList[i].isRoot ? modeForRoot : modeForOthers, log)))
                    {
                        OsConfigLogInfo(log, "SetRestrictedUserHomeDirectories: user '%s' (%u, %u) has now proper restricted access (%u) for their assigned home directory '%s'",
                            userList[i].username, userList[i].userId, userList[i].groupId, userList[i].isRoot ? modeForRoot : modeForOthers, userList[i].home);
                    }
                    else
                    {
                        OsConfigLogError(log, "SetRestrictedUserHomeDirectories: failed to set restricted access (%u) for user '%s' (%u, %u) assigned home directory '%s' (%d)",
                            userList[i].isRoot ? modeForRoot : modeForOthers, userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home, _status);

                        if (0 == status)
                        {
                            status = _status;
                        }
                    }
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetRestrictedUserHomeDirectories: all users who can login have proper restricted access for their home directories");
    }

    return status;
}

int CheckPasswordHashingAlgorithm(unsigned int algorithm, char** reason, void* log)
{
    const char* command = "cat /etc/login.defs | grep ENCRYPT_METHOD | grep ^[^#]";
    char* encryption = EncryptionName(algorithm);
    char* textResult = NULL;
    int status = 0;

    if ((0 == (status = ExecuteCommand(NULL, command, true, false, 0, 0, &textResult, NULL, log))) && (NULL != textResult))
    {
        RemovePrefixBlanks(textResult);
        RemovePrefixUpTo(textResult, ' ');
        RemovePrefixBlanks(textResult);
        RemoveTrailingBlanks(textResult);

        if (0 == strcmp(textResult, encryption))
        {
            OsConfigLogInfo(log, "CheckPasswordHashingAlgorithm: the correct user password encryption algorithm '%s' (%d) is currently set in '/etc/login.defs'", encryption, algorithm);
            OsConfigCaptureSuccessReason(reason, "The correct user password encryption algorithm '%s' (%d) is currently set in '/etc/login.defs'", encryption, algorithm);
        }
        else
        {
            OsConfigLogError(log, "CheckPasswordHashingAlgorithm: the user password encryption algorithm currently set in '/etc/login.defs' to '%s' is different from the required '%s' (%d) ",
                textResult, encryption, algorithm);
            OsConfigCaptureReason(reason, "The user password encryption algorithm currently set in '/etc/login.defs' to '%s' is different from the required '%s' (%d) ",
                textResult, encryption, algorithm);
        }

        FREE_MEMORY(textResult);
    }
    else
    {
        if (0 == status)
        {
            status = ENOENT;
        }

        OsConfigLogError(log, "CheckPasswordHashingAlgorithm: failed to read 'ENCRYPT_METHOD' from '/etc/login.defs' (%d)", status);
        OsConfigCaptureReason(reason, "Failed to read 'ENCRYPT_METHOD' from '/etc/login.defs' (%d)", status);
    }

    return status;
}

int SetPasswordHashingAlgorithm(unsigned int algorithm, void* log)
{
    const char* encryptMethod = "ENCRYPT_METHOD";
    char* encryption = EncryptionName(algorithm);
    int status = 0;

    if ((md5 != algorithm) && (sha256 != algorithm) && (sha512 != algorithm))
    {
        OsConfigLogError(log, "SetPasswordHashingAlgorithm: unsupported algorithm argument (%u, not: %u, %u, or %u)", algorithm, md5, sha256, sha512);
        return EINVAL;
    }

    if (0 == CheckPasswordHashingAlgorithm(algorithm, NULL, log))
    {
        if (0 == (status = SetEtcLoginDefValue(encryptMethod, encryption, log)))
        {
            OsConfigLogInfo(log, "SetPasswordHashingAlgorithm: successfully set 'ENCRYPT_METHOD' to '%s' in '/etc/login.defs'", encryption);
        }
        else
        {
            OsConfigLogError(log, "SetPasswordHashingAlgorithm: failed to set 'ENCRYPT_METHOD' to '%s' in '/etc/login.defs' (%d)", encryption, status);
        }
    }

    return status;
}

int CheckMinDaysBetweenPasswordChanges(long days, char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;
    long etcLoginDefsDays = GetPassMinDays(log);

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
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
                    OsConfigCaptureSuccessReason(reason, "User '%s' (%u, %u) has a minimum time between password changes of %ld days (requested: %ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].minimumPasswordAge, days);
                }
                else
                {
                    OsConfigLogError(log, "CheckMinDaysBetweenPasswordChanges: user '%s' (%u, %u) minimum time between password changes of %ld days is less than requested %ld days",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].minimumPasswordAge, days);
                    OsConfigCaptureReason(reason, "User '%s' (%u, %u) minimum time between password changes of %ld days is less than requested %ld days",
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
        OsConfigCaptureSuccessReason(reason, "All users who have passwords have correct number of minimum days (%ld) between changes", days);
    }

    if (-1 == etcLoginDefsDays)
    {
        OsConfigLogError(log, "CheckMinDaysBetweenPasswordChanges: there is no configured PASS_MIN_DAYS in /etc/login.defs");
        OsConfigCaptureReason(reason, "There is no configured 'PASS_MIN_DAYS' in '/etc/login.defs'");
        status = ENOENT;
    }
    else if (0 == etcLoginDefsDays)
    {
        OsConfigLogError(log, "CheckMinDaysBetweenPasswordChanges: PASS_MIN_DAYS is configured to default 0 in /etc/login.defs meaning disabled restriction");
        OsConfigCaptureReason(reason, "'PASS_MIN_DAYS' is configured to default 0 in '/etc/login.defs' meaning disabled restriction");
        status = ENOENT;
    }
    else if (etcLoginDefsDays < days)
    {
        OsConfigLogError(log, "CheckMinDaysBetweenPasswordChanges: configured PASS_MIN_DAYS in /etc/login.defs %ld days is less than requested %ld days", etcLoginDefsDays, days);
        OsConfigCaptureReason(reason, "Configured 'PASS_MIN_DAYS' in '/etc/login.defs' of %ld days is less than requested %ld days", etcLoginDefsDays, days);
        status = ENOENT;
    }
    else
    {
        OsConfigCaptureSuccessReason(reason, "'PASS_MIN_DAYS' is set to %ld days in '/etc/login.defs' (requested: %ld)", etcLoginDefsDays, days);
    }

    return status;
}

int SetMinDaysBetweenPasswordChanges(long days, void* log)
{
    const char* commandTemplate = "chage -m %ld %s";
    char* command = NULL;
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else if (userList[i].minimumPasswordAge < days)
            {
                OsConfigLogInfo(log, "SetMinDaysBetweenPasswordChanges: user '%s' (%u, %u) minimum time between password changes of %ld days is less than requested %ld days",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].minimumPasswordAge, days);

                if (NULL == (command = FormatAllocateString(commandTemplate, days, userList[i].username)))
                {
                    OsConfigLogError(log, "SetMinDaysBetweenPasswordChanges: cannot allocate memory");
                    status = ENOMEM;
                    break;
                }
                else
                {
                    if (0 == (_status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
                    {
                        userList[i].minimumPasswordAge = days;
                        OsConfigLogInfo(log, "SetMinDaysBetweenPasswordChanges: user '%s' (%u, %u) minimum time between password changes is now set to %ld days",
                            userList[i].username, userList[i].userId, userList[i].groupId, days);
                    }

                    FREE_MEMORY(command);

                    if (_status && (0 == status))
                    {
                        status = _status;
                    }
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetMinDaysBetweenPasswordChanges: all users who have passwords have correct number of minimum days (%ld) between changes", days);
    }

    if (0 == (_status = SetPassMinDays(days, log)))
    {
        OsConfigLogInfo(log, "SetMinDaysBetweenPasswordChanges: 'PASS_MIN_DAYS' is set to %ld days in '/etc/login.defs'", days);
    }
    else
    {
        OsConfigLogError(log, "SetMinDaysBetweenPasswordChanges: failed to set 'PASS_MIN_DAYS' to %ld days in '/etc/login.defs' (%d)", days, _status);
    }

    if (_status && (0 == status))
    {
        status = _status;
    }

    return status;
}

int CheckMaxDaysBetweenPasswordChanges(long days, char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;
    long etcLoginDefsDays = GetPassMaxDays(log);

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else
            {
                if (userList[i].maximumPasswordAge < 0)
                {
                    OsConfigLogError(log, "CheckMaxDaysBetweenPasswordChanges: user '%s' (%u, %u) has unlimited time between password changes of %ld days (requested: %ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge, days);
                    OsConfigCaptureReason(reason, "User '%s' (%u, %u) has unlimited time between password changes of %ld days(requested: %ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge, days);
                    status = ENOENT;
                }
                else if (userList[i].maximumPasswordAge <= days)
                {
                    OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: user '%s' (%u, %u) has a maximum time between password changes of %ld days (requested: %ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge, days);
                    OsConfigCaptureSuccessReason(reason, "User '%s' (%u, %u) has a maximum time between password changes of %ld days(requested: %ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge, days);
                }
                else
                {
                    OsConfigLogError(log, "CheckMaxDaysBetweenPasswordChanges: user '%s' (%u, %u) maximum time between password changes of %ld days is more than requested %ld days",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge, days);
                    OsConfigCaptureReason(reason, "User '%s' (%u, %u) maximum time between password changes of %ld days is more than requested %ld days",
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
        OsConfigCaptureSuccessReason(reason, "All users who have passwords have correct number of maximum days (%ld) between changes", days);
    }

    if (-1 == etcLoginDefsDays)
    {
        OsConfigLogError(log, "CheckMaxDaysBetweenPasswordChanges: there is no configured PASS_MAX_DAYS in /etc/login.defs");
        OsConfigCaptureReason(reason, "'PASS_MAX_DAYS' is not configured in '/etc/login.defs'");
        status = ENOENT;
    }
    else if (etcLoginDefsDays > days)
    {
        OsConfigLogError(log, "CheckMaxDaysBetweenPasswordChanges: configured PASS_MAX_DAYS in /etc/login.defs %ld days is more than requested %ld days", etcLoginDefsDays, days);
        OsConfigCaptureReason(reason, "Configured 'PASS_MAX_DAYS' in '/etc/login.defs' of %ld days is more than requested %ld days", etcLoginDefsDays, days);
        status = ENOENT;
    }
    else
    {
        OsConfigCaptureSuccessReason(reason, "'PASS_MAX_DAYS' is set to %ld days in '/etc/login.defs' (requested: %ld)", etcLoginDefsDays, days);
    }

    return status;
}

int SetMaxDaysBetweenPasswordChanges(long days, void* log)
{
    const char* commandTemplate = "chage -M %ld %s";
    char* command = NULL;
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else if ((userList[i].maximumPasswordAge > days) || (userList[i].maximumPasswordAge < 0))
            {
                OsConfigLogInfo(log, "SetMaxDaysBetweenPasswordChanges: user '%s' (%u, %u) has maximum time between password changes of %ld days while requested is %ld days",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge, days);

                if (NULL == (command = FormatAllocateString(commandTemplate, days, userList[i].username)))
                {
                    OsConfigLogError(log, "SetMaxDaysBetweenPasswordChanges: cannot allocate memory");
                    status = ENOMEM;
                    break;
                }
                else
                {
                    if (0 == (_status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
                    {
                        userList[i].maximumPasswordAge = days;
                        OsConfigLogInfo(log, "SetMaxDaysBetweenPasswordChanges: user '%s' (%u, %u) maximum time between password changes is now set to %ld days",
                            userList[i].username, userList[i].userId, userList[i].groupId, days);
                    }

                    FREE_MEMORY(command);

                    if (0 == status)
                    {
                        status = _status;
                    }
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetMaxDaysBetweenPasswordChanges: all users who have passwords have correct number of maximum days (%ld) between changes", days);
    }

    if (0 == (_status = SetPassMaxDays(days, log)))
    {
        OsConfigLogInfo(log, "SetMaxDaysBetweenPasswordChanges: 'PASS_MAX_DAYS' is set to %ld days in '/etc/login.defs'", days);
    }
    else
    {
        OsConfigLogError(log, "SetMaxDaysBetweenPasswordChanges: failed to set 'PASS_MAX_DAYS' to %ld days in '/etc/login.defs' (%d)", days, _status);
    }

    if (_status & (0 == status))
    {
        status = _status;
    }

    return status;
}

int EnsureUsersHaveDatesOfLastPasswordChanges(void* log)
{
    const char* commandTemplate = "chage -d %ld %s";
    char* command = NULL;
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;
    time_t currentTime = 0;
    long currentDate = time(&currentTime) / NUMBER_OF_SECONDS_IN_A_DAY;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else if (userList[i].lastPasswordChange < 0)
            {
                OsConfigLogInfo(log, "EnsureUsersHaveDatesOfLastPasswordChanges: password for user '%s' (%u, %u) was never changed (%lu)",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].lastPasswordChange);

                if (NULL == (command = FormatAllocateString(commandTemplate, currentDate, userList[i].username)))
                {
                    OsConfigLogError(log, "EnsureUsersHaveDatesOfLastPasswordChanges: cannot allocate memory");
                    status = ENOMEM;
                    break;
                }
                else
                {
                    if (0 == (_status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
                    {
                        OsConfigLogInfo(log, "EnsureUsersHaveDatesOfLastPasswordChanges: user '%s' (%u, %u) date of last password change is now set to %ld days since epoch (today)",
                            userList[i].username, userList[i].userId, userList[i].groupId, currentDate);
                    }

                    FREE_MEMORY(command);

                    if (0 == status)
                    {
                        status = _status;
                    }
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "EnsureUsersHaveDatesOfLastPasswordChanges: all users who have passwords have dates of last password changes");
    }

    return status;
}

int CheckPasswordExpirationLessThan(long days, char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    time_t currentTime = 0;
    int status = 0;
    long passwordExpirationDate = 0;
    long currentDate = time(&currentTime) / NUMBER_OF_SECONDS_IN_A_DAY;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else
            {
                if (userList[i].maximumPasswordAge < 0)
                {
                    OsConfigLogError(log, "CheckPasswordExpirationLessThan: password for user '%s' (%u, %u) has no expiration date (%ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge);
                    OsConfigCaptureReason(reason, "Password for user '%s' (%u, %u) has no expiration date (%ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge);
                    status = ENOENT;
                }
                else if (userList[i].lastPasswordChange < 0)
                {
                    OsConfigLogError(log, "CheckPasswordExpirationLessThan: password for user '%s' (%u, %u) has no recorded change date (%ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].lastPasswordChange);
                    OsConfigCaptureReason(reason, "Password for user '%s' (%u, %u) has no recorded last change date (%ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].lastPasswordChange);
                    status = ENOENT;
                }
                else
                {
                    passwordExpirationDate = userList[i].lastPasswordChange + userList[i].maximumPasswordAge;

                    if (passwordExpirationDate >= currentDate)
                    {
                        if ((passwordExpirationDate - currentDate) <= days)
                        {
                            OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: password for user '%s' (%u, %u) will expire in %ld days (requested maximum: %ld)",
                                userList[i].username, userList[i].userId, userList[i].groupId, passwordExpirationDate - currentDate, days);
                            OsConfigCaptureSuccessReason(reason, "Password for user '%s' (%u, %u) will expire in %ld days (requested maximum: %ld)",
                                userList[i].username, userList[i].userId, userList[i].groupId, passwordExpirationDate - currentDate, days);
                        }
                        else
                        {
                            OsConfigLogError(log, "CheckPasswordExpirationLessThan: password for user '%s' (%u, %u) will expire in %ld days, more than requested maximum of %ld days",
                                userList[i].username, userList[i].userId, userList[i].groupId, passwordExpirationDate - currentDate, days);
                            OsConfigCaptureReason(reason, "Password for user '%s' (%u, %u) will expire in %ld days, more than requested maximum of %ld days",
                                userList[i].username, userList[i].userId, userList[i].groupId, passwordExpirationDate - currentDate, days);
                            status = ENOENT;
                        }
                    }
                    else
                    {
                        OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: password for user '%s' (%u, %u) expired %ld days ago (current date: %ld, expiration date: %ld days since the epoch)",
                            userList[i].username, userList[i].userId, userList[i].groupId, currentDate - passwordExpirationDate, currentDate, passwordExpirationDate);
                        OsConfigCaptureSuccessReason(reason, "Password for user '%s' (%u, %u)  expired %ld days ago (current date: %ld, expiration date: %ld days since the epoch)",
                            userList[i].username, userList[i].userId, userList[i].groupId, currentDate - passwordExpirationDate, currentDate, passwordExpirationDate);
                    }
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: passwords for all users who have them will expire in %ld days or less", days);
        OsConfigCaptureSuccessReason(reason, "Passwords for all users who have them will expire in %ld days or less", days);
    }

    return status;
}

int CheckPasswordExpirationWarning(long days, char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;
    long etcLoginDefsDays = GetPassWarnAge(log);

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
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
                    OsConfigCaptureSuccessReason(reason, "User '%s' (%u, %u) has a password expiration warning time of %ld days (requested: %ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].warningPeriod, days);
                }
                else
                {
                    OsConfigLogError(log, "CheckPasswordExpirationWarning: user '%s' (%u, %u) password expiration warning time is %ld days, less than requested %ld days",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].warningPeriod, days);
                    OsConfigCaptureReason(reason, "User '%s' (%u, %u) password expiration warning time is %ld days, less than requested %ld days",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].warningPeriod, days);
                    status = ENOENT;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckPasswordExpirationWarning: all users who have passwords have correct have correct password expiration warning time of %ld days", days);
        OsConfigCaptureSuccessReason(reason, "All users who have passwords have correct have correct password expiration warning time of %ld days", days);
    }

    if (-1 == etcLoginDefsDays)
    {
        OsConfigLogError(log, "CheckMaxDaysBetweenPasswordChanges: there is no configured PASS_WARN_AGE in /etc/login.defs");
        OsConfigCaptureReason(reason, "'PASS_WARN_AGE' is not configured in '/etc/login.defs'");
        status = ENOENT;
    }
    else if (etcLoginDefsDays < days)
    {
        OsConfigLogError(log, "CheckMaxDaysBetweenPasswordChanges: configured PASS_WARN_AGE in /etc/login.defs %ld days is less than requested %ld days", etcLoginDefsDays, days);
        OsConfigCaptureReason(reason, "Configured 'PASS_WARN_AGE' in '/etc/login.defs' of %ld days is less than requested %ld days", etcLoginDefsDays, days);
        status = ENOENT;
    }
    else
    {
        OsConfigCaptureSuccessReason(reason, "'PASS_WARN_AGE' is set to %ld days in '/etc/login.defs' (requested: %ld)", etcLoginDefsDays, days);
    }

    return status;
}

int SetPasswordExpirationWarning(long days, void* log)
{
    const char* commandTemplate = "chage -W %ld %s";
    char* command = NULL;
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else if (userList[i].warningPeriod < days)
            {
                OsConfigLogError(log, "SetPasswordExpirationWarning: user '%s' (%u, %u) password expiration warning time is %ld days, less than requested %ld days",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].warningPeriod, days);

                if (NULL == (command = FormatAllocateString(commandTemplate, days, userList[i].username)))
                {
                    OsConfigLogError(log, "SetPasswordExpirationWarning: cannot allocate memory");
                    status = ENOMEM;
                    break;
                }
                else
                {
                    if (0 == (_status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
                    {
                        userList[i].warningPeriod = days;
                        OsConfigLogInfo(log, "SetPasswordExpirationWarning: user '%s' (%u, %u) password expiration warning time is now set to %ld days",
                            userList[i].username, userList[i].userId, userList[i].groupId, days);
                    }

                    FREE_MEMORY(command);

                    if (0 == status)
                    {
                        status = _status;
                    }
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetPasswordExpirationWarning: all users who have passwords have correct number of maximum days (%ld) between changes", days);
    }

    if (0 == (_status = SetPassWarnAge(days, log)))
    {
        OsConfigLogInfo(log, "SetPasswordExpirationWarning: 'PASS_WARN_AGE' is set to %ld days in '/etc/login.defs'", days);
    }
    else
    {
        OsConfigLogError(log, "SetPasswordExpirationWarning: failed to set 'PASS_WARN_AGE' to %ld days in '/etc/login.defs' (%d)", days, _status);
    }

    if (_status && (0 == status))
    {
        status = _status;
    }

    return status;
}

int CheckUsersRecordedPasswordChangeDates(char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    long timer = 0;
    int status = 0;
    long daysCurrent = time(&timer) / NUMBER_OF_SECONDS_IN_A_DAY;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (false == userList[i].hasPassword)
            {
                continue;
            }
            else
            {
                if (userList[i].lastPasswordChange < 0)
                {
                    OsConfigLogInfo(log, "CheckUsersRecordedPasswordChangeDates: password for user '%s' (%u, %u) has no recorded change date (%ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].lastPasswordChange);
                    OsConfigCaptureSuccessReason(reason, "User '%s' (%u, %u) has no recorded last password change date (%ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].lastPasswordChange);
                }
                else if (userList[i].lastPasswordChange <= daysCurrent)
                {
                    OsConfigLogInfo(log, "CheckUsersRecordedPasswordChangeDates: user '%s' (%u, %u) has %lu days since last password change",
                        userList[i].username, userList[i].userId, userList[i].groupId, daysCurrent - userList[i].lastPasswordChange);
                    OsConfigCaptureSuccessReason(reason, "User '%s' (%u, %u) has %lu days since last password change",
                        userList[i].username, userList[i].userId, userList[i].groupId, daysCurrent - userList[i].lastPasswordChange);
                }
                else
                {
                    OsConfigLogError(log, "CheckUsersRecordedPasswordChangeDates: user '%s' (%u, %u) last recorded password change is in the future (next %ld days)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].lastPasswordChange - daysCurrent);
                    OsConfigCaptureReason(reason, "User '%s' (%u, %u) last recorded password change is in the future (next %ld days)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].lastPasswordChange - daysCurrent);
                    status = ENOENT;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckUsersRecordedPasswordChangeDates: all users who have passwords have dates of last password change in the past");
        OsConfigCaptureSuccessReason(reason, "All users who have passwords have dates of last password change in the past");
    }

    return status;
}

int CheckLockoutAfterInactivityLessThan(long days, char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((false == userList[i].hasPassword) && (true == userList[i].isRoot))
            {
                continue;
            }
            else if (userList[i].inactivityPeriod > days)
            {
                OsConfigLogInfo(log, "CheckLockoutAfterInactivityLessThan: user '%s' (%u, %u) period of inactivity before lockout is %ld days, more than requested %ld days",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].inactivityPeriod, days);
                OsConfigCaptureReason(reason, "User '%s' (%u, %u) password period of inactivity before lockout is %ld days, more than requested %ld days",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].inactivityPeriod, days);
                status = ENOENT;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckLockoutAfterInactivityLessThan: all non-root users who have passwords have correct number of maximum inactivity days (%ld) before lockout", days);
        OsConfigCaptureSuccessReason(reason, "All non-root users who have passwords have correct number of maximum inactivity days (%ld) before lockout", days);
    }

    return status;
}

int SetLockoutAfterInactivityLessThan(long days, void* log)
{
    const char* commandTemplate = "chage -I %ld %s";
    char* command = NULL;
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((false == userList[i].hasPassword) && (true == userList[i].isRoot))
            {
                continue;
            }
            else if (userList[i].inactivityPeriod > days)
            {
                OsConfigLogInfo(log, "SetLockoutAfterInactivityLessThan: user '%s' (%u, %u) is locked out after %ld days of inactivity while requested is %ld days",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].inactivityPeriod, days);

                if (NULL == (command = FormatAllocateString(commandTemplate, days, userList[i].username)))
                {
                    OsConfigLogError(log, "SetLockoutAfterInactivityLessThan: cannot allocate memory");
                    status = ENOMEM;
                    break;
                }
                else
                {
                    if (0 == (_status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
                    {
                        userList[i].inactivityPeriod = days;
                        OsConfigLogInfo(log, "SetLockoutAfterInactivityLessThan: user '%s' (%u, %u) lockout time after inactivity is now set to %ld days",
                            userList[i].username, userList[i].userId, userList[i].groupId, days);
                    }

                    FREE_MEMORY(command);

                    if (0 == status)
                    {
                        status = _status;
                    }
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetLockoutAfterInactivityLessThan: all non-root users who have passwords have correct number of maximum inactivity days (%ld) before lockout", days);
    }

    return status;
}

int CheckSystemAccountsAreNonLogin(char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((userList[i].isLocked || userList[i].noLogin || userList[i].cannotLogin) && userList[i].hasPassword && userList[i].userId)
            {
                OsConfigLogError(log, "CheckSystemAccountsAreNonLogin: user '%s' (%u, %u, '%s', '%s') is either locked, no-login, or cannot-login, "
                    "but can login with password", userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home, userList[i].shell);
                OsConfigCaptureReason(reason, "User '%s' (%u, %u, '%s', '%s') is either locked, no-login, or cannot-login, but can login with password",
                    userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home, userList[i].shell);
                status = ENOENT;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckSystemAccountsAreNonLogin: all system accounts are non-login");
        OsConfigCaptureSuccessReason(reason, "All system accounts are non-login");
    }

    return status;
}

int RemoveSystemAccountsThatCanLogin(void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((userList[i].isLocked || userList[i].noLogin || userList[i].cannotLogin) && userList[i].hasPassword && userList[i].userId)
            {
                OsConfigLogError(log, "RemoveSystemAccountsThatCanLogin: user '%s' (%u, %u, '%s', '%s') is either locked, no-login, or cannot-login, "
                    "but can login with password",  userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home, userList[i].shell);

                if ((0 != (_status = RemoveUser(&(userList[i]), log))) && (0 == status))
                {
                    status = _status;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "RemoveSystemAccountsThatCanLogin: all system accounts are non-login");
    }

    return status;
}

int CheckRootPasswordForSingleUserMode(char** reason, void* log)
{
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    bool usersWithPassword = false;
    bool rootHasPassword = false;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].hasPassword)
            {
                if (userList[i].isRoot)
                {
                    OsConfigLogInfo(log, "CheckRootPasswordForSingleUserMode: root appears to have a password");
                    rootHasPassword = true;
                }
                else
                {
                    OsConfigLogInfo(log, "CheckRootPasswordForSingleUserMode: user '%s' (%u, %u) appears to have a password",
                        userList[i].username, userList[i].userId, userList[i].groupId);
                    usersWithPassword = true;
                }
            }

            if (rootHasPassword && usersWithPassword)
            {
                break;
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        if (rootHasPassword && (false == usersWithPassword))
        {
            OsConfigLogInfo(log, "CheckRootPasswordForSingleUserMode: single user mode, only root user has password");
            OsConfigCaptureSuccessReason(reason, "Single user mode and only root user has password");
        }
        else if (rootHasPassword && usersWithPassword)
        {
            OsConfigLogInfo(log, "CheckRootPasswordForSingleUserMode: multi-user mode, root has password");
            OsConfigCaptureSuccessReason(reason, "Multi-user mode and root has password");
        }
        else if ((false == rootHasPassword) && usersWithPassword)
        {
            OsConfigLogInfo(log, "CheckRootPasswordForSingleUserMode: multi-user mode, root does not have password");
            OsConfigCaptureSuccessReason(reason, "Multi-user mode and root does not have password");
        }
        else if ((false == rootHasPassword) && (false == usersWithPassword))
        {
            OsConfigLogError(log, "CheckRootPasswordForSingleUserMode: single user mode and root does not have password");
            OsConfigCaptureReason(reason, "Single user mode and root does not have a password set, must manually set a password "
                "for root user, automatic remediation is not possible");
            status = ENOENT;
        }
    }

    return status;
}

int CheckOrEnsureUsersDontHaveDotFiles(const char* name, bool removeDotFiles, char** reason, void* log)
{
    const char* templateDotPath = "%s/.%s";

    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    size_t templateLength = 0, length = 0;
    char* dotPath = NULL;
    int status = 0;

    if (NULL == name)
    {
        OsConfigLogError(log, "CheckOrEnsureUsersDontHaveDotFiles called with an invalid argument");
        return EINVAL;
    }

    templateLength = strlen(templateDotPath) + strlen(name) + 1;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((userList[i].noLogin) || (userList[i].isRoot))
            {
                continue;
            }
            else if (DirectoryExists(userList[i].home))
            {
                length = templateLength + strlen(userList[i].home);

                if (NULL == (dotPath = malloc(length)))
                {
                    OsConfigLogError(log, "CheckOrEnsureUsersDontHaveDotFiles: out of memory");
                    status = ENOMEM;
                    break;
                }

                memset(dotPath, 0, length);
                snprintf(dotPath, length, templateDotPath, userList[i].home, name);

                if (FileExists(dotPath))
                {
                    if (removeDotFiles)
                    {
                        remove(dotPath);

                        if (FileExists(dotPath))
                        {
                            OsConfigLogError(log, "CheckOrEnsureUsersDontHaveDotFiles: for user '%s' (%u, %u), '%s' needs to be manually removed",
                                userList[i].username, userList[i].userId, userList[i].groupId, dotPath);
                            status = ENOENT;
                        }
                    }
                    else
                    {
                        OsConfigLogError(log, "CheckOrEnsureUsersDontHaveDotFiles: user '%s' (%u, %u) has file '.%s' ('%s')",
                            userList[i].username, userList[i].userId, userList[i].groupId, name, dotPath);
                        OsConfigCaptureReason(reason, "User '%s' (%u, %u) has file '.%s' ('%s')",
                            userList[i].username, userList[i].userId, userList[i].groupId, name, dotPath);
                        status = ENOENT;
                    }
                }

                FREE_MEMORY(dotPath);
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckOrEnsureUsersDontHaveDotFiles: no users have '.%s' files", name);
        OsConfigCaptureSuccessReason(reason, "No users have '.%s' files", name);
    }

    return status;
}

int CheckUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, char** reason, void* log)
{
    const char* pathTemplate = "%s/%s";

    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0, j = 0;
    DIR* home = NULL;
    struct dirent* entry = NULL;
    char* path = NULL;
    size_t length = 0;
    bool oneGoodMode = false;
    int status = 0;

    if ((NULL == modes) || (0 == numberOfModes))
    {
        OsConfigLogError(log, "CheckUsersRestrictedDotFiles: invalid arguments (%p, %u)", modes, numberOfModes);
        return EINVAL;
    }

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].noLogin || userList[i].cannotLogin || userList[i].isLocked)
            {
                continue;
            }
            else if (DirectoryExists(userList[i].home) && (NULL != (home = opendir(userList[i].home))))
            {
                while (NULL != (entry = readdir(home)))
                {
                    if ((DT_REG == entry->d_type) && ('.' == entry->d_name[0]))
                    {
                        length = strlen(pathTemplate) + strlen(userList[i].home) + strlen(entry->d_name);
                        if (NULL == (path = malloc(length + 1)))
                        {
                            OsConfigLogError(log, "CheckUsersRestrictedDotFiles: out of memory");
                            status = ENOMEM;
                            break;
                        }

                        memset(path, 0, length + 1);
                        snprintf(path, length, pathTemplate, userList[i].home, entry->d_name);

                        oneGoodMode = false;

                        for (j = 0; j < numberOfModes; j++)
                        {
                            if (0 == CheckFileAccess(path, userList[i].userId, userList[i].groupId, modes[j], NULL, log))
                            {
                                OsConfigLogInfo(log, "CheckUsersRestrictedDotFiles: user '%s' (%u, %u) has proper restricted access (%u) for their dot file '%s'",
                                    userList[i].username, userList[i].userId, userList[i].groupId, modes[j], path);
                                oneGoodMode = true;
                                break;
                            }
                        }

                        if (false == oneGoodMode)
                        {
                            OsConfigLogError(log, "CheckUsersRestrictedDotFiles: user '%s' (%u, %u) does not has have proper restricted access for their dot file '%s'",
                                userList[i].username, userList[i].userId, userList[i].groupId, path);
                            OsConfigCaptureReason(reason, "User '%s' (%u, %u) does not has have proper restricted access for their dot file '%s'",
                                userList[i].username, userList[i].userId, userList[i].groupId, path);

                            if (0 == status)
                            {
                                status = ENOENT;
                            }
                        }

                        FREE_MEMORY(path);
                    }
                }

                closedir(home);
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckUserDotFilesAccess: all users who can login have dot files (if any) with proper restricted access");
        OsConfigCaptureSuccessReason(reason, "All users who can login have dot files (if any) with proper restricted access");
    }

    return status;
}

int SetUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, unsigned int mode, void* log)
{
    const char* pathTemplate = "%s/%s";

    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0, j = 0;
    DIR* home = NULL;
    struct dirent* entry = NULL;
    char* path = NULL;
    size_t length = 0;
    bool oneGoodMode = false;
    int status = 0, _status = 0;

    if ((NULL == modes) || (0 == numberOfModes))
    {
        OsConfigLogError(log, "SetUsersRestrictedDotFiles: invalid arguments (%p, %u)", modes, numberOfModes);
        return EINVAL;
    }

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (userList[i].noLogin || userList[i].cannotLogin || userList[i].isLocked)
            {
                continue;
            }
            else if (DirectoryExists(userList[i].home) && (NULL != (home = opendir(userList[i].home))))
            {
                while (NULL != (entry = readdir(home)))
                {
                    if ((DT_REG == entry->d_type) && ('.' == entry->d_name[0]))
                    {
                        length = strlen(pathTemplate) + strlen(userList[i].home) + strlen(entry->d_name);
                        if (NULL == (path = malloc(length + 1)))
                        {
                            OsConfigLogError(log, "SetUsersRestrictedDotFiles: out of memory");
                            status = ENOMEM;
                            break;
                        }

                        memset(path, 0, length + 1);
                        snprintf(path, length, pathTemplate, userList[i].home, entry->d_name);

                        oneGoodMode = false;

                        for (j = 0; j < numberOfModes; j++)
                        {
                            if (0 == CheckFileAccess(path, userList[i].userId, userList[i].groupId, modes[j], NULL, log))
                            {
                                OsConfigLogInfo(log, "SetUsersRestrictedDotFiles: user '%s' (%u, %u) already has proper restricted access (%u) set for their dot file '%s'",
                                    userList[i].username, userList[i].userId, userList[i].groupId, modes[j], path);
                                oneGoodMode = true;
                                break;
                            }
                        }

                        if (false == oneGoodMode)
                        {
                            if (0 == (_status = SetFileAccess(path, userList[i].userId, userList[i].groupId, mode, log)))
                            {
                                OsConfigLogInfo(log, "SetUsersRestrictedDotFiles: user '%s' (%u, %u) now has restricted access (%u) set for their dot file '%s'",
                                    userList[i].username, userList[i].userId, userList[i].groupId, mode, path);
                            }
                            else
                            {
                                OsConfigLogError(log, "SetUsersRestrictedDotFiles: failed to set restricted access (%u) for user '%s' (%u, %u) dot file '%s'",
                                    mode, userList[i].username, userList[i].userId, userList[i].groupId, path);

                                if (0 == status)
                                {
                                    status = _status;
                                }
                            }
                        }

                        FREE_MEMORY(path);
                    }
                }

                closedir(home);
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetUserDotFilesAccess: all users who can login now have proper restricted access to their dot files, if any");
    }

    return status;
}

int CheckUserAccountsNotFound(const char* names, char** reason, void* log)
{
    const char* userTemplate = "%s:";
    size_t namesLength = 0;
    char* name = NULL;
    char* decoratedName = NULL;
    SIMPLIFIED_USER* userList = NULL;
    SIMPLIFIED_GROUP* groupList = NULL;
    unsigned int userListSize = 0, groupListSize = 0, i = 0, j = 0;
    bool found = false;
    int status = 0;

    if (NULL == names)
    {
        OsConfigLogError(log, "CheckUserAccountsNotFound: invalid argument");
        return EINVAL;
    }

    namesLength = strlen(names);

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            for (j = 0; j < namesLength; j++)
            {
                if (NULL == (name = DuplicateString(&(names[j]))))
                {
                    OsConfigLogError(log, "CheckUserAccountsNotFound: failed to duplicate string");
                    status = ENOMEM;
                    break;
                }
                else
                {
                    TruncateAtFirst(name, ',');

                    if (0 == strcmp(userList[i].username, name))
                    {
                        EnumerateUserGroups(&userList[i], &groupList, &groupListSize, reason, log);
                        FreeGroupList(&groupList, groupListSize);

                        OsConfigLogInfo(log, "CheckUserAccountsNotFound: user '%s' found with id %u, gid %u, home '%s' and present in %u group(s)",
                            userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home, groupListSize);

                        if (DirectoryExists(userList[i].home))
                        {
                            OsConfigLogInfo(log, "CheckUserAccountsNotFound: home directory of user '%s' exists ('%s')", name, userList[i].home);
                        }

                        OsConfigCaptureReason(reason, "User '%s' found with id %u, gid %u, home '%s' and present in %u group(s)",
                            userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home, groupListSize);

                        found = true;
                    }
                }

                j += strlen(name);
                FREE_MEMORY(name);
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if ((false == found) && (0 == status))
    {
        OsConfigLogInfo(log, "CheckUserAccountsNotFound: none of the requested user accounts ('%s') were found in the users database", names);
    }

    if (0 == status)
    {
        for (j = 0; j < namesLength; j++)
        {
            if (NULL == (name = DuplicateString(&(names[j]))))
            {
                OsConfigLogError(log, "CheckUserAccountsNotFound: failed to duplicate string");
                status = ENOMEM;
                break;
            }
            else
            {
                TruncateAtFirst(name, ',');

                if (NULL == (decoratedName = FormatAllocateString(userTemplate, name)))
                {
                    OsConfigLogInfo(log, "CheckUserAccountsNotFound: out of memory, unable to check for user '%s' presence in '%s' andr '%s'",
                        name, g_etcPasswd, g_etcShadow);
                }
                else
                {
                    if (0 == FindTextInFile(g_etcPasswd, decoratedName, log))
                    {
                        OsConfigCaptureReason(reason, "Account '%s' found mentioned in '%s'", name, g_etcPasswd);
                        found = true;
                    }

                    if (0 == FindTextInFile(g_etcShadow, decoratedName, log))
                    {
                        OsConfigCaptureReason(reason, "Account '%s' found mentioned in '%s'", name, g_etcShadow);
                        found = true;
                    }

                    FREE_MEMORY(decoratedName);
                }

                j += strlen(name);
                FREE_MEMORY(name);
            }
        }
    }

    if (found)
    {
        status = EEXIST;
    }
    else if (0 == status)
    {
        OsConfigLogInfo(log, "CheckUserAccountsNotFound: none of the requested user accounts ('%s') is present", names);
        OsConfigCaptureSuccessReason(reason, "None of the requested user accounts ('%s') is present", names);
    }
    else
    {
        OsConfigCaptureReason(reason, "Failed to check for presence of the requested user accounts (%d)", status);
    }

    return status;
}

int RemoveUserAccounts(const char* names, void* log)
{
    const char* userTemplate = "%s:";
    size_t namesLength = 0;
    char* name = NULL;
    char* decoratedName = NULL;
    SIMPLIFIED_USER* userList = NULL;
    unsigned int userListSize = 0, i = 0, j = 0;
    bool userAccountsNotFound = false;
    int status = 0, _status = 0;

    if (NULL == names)
    {
        OsConfigLogError(log, "RemoveUserAccounts: invalid argument");
        return EINVAL;
    }

    if (0 == (status = CheckUserAccountsNotFound(names, NULL, log)))
    {
        OsConfigLogInfo(log, "RemoveUserAccounts: user accounts '%s' are not found in the users database", names);
        userAccountsNotFound = true;
    }
    else if (EEXIST != status)
    {
        OsConfigLogError(log, "RemoveUserAccounts: CheckUserAccountsNotFound('%s') failed with %d", names, status);
        return status;
    }

    namesLength = strlen(names);

    if (false == userAccountsNotFound)
    {
        if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
        {
            for (i = 0; i < userListSize; i++)
            {
                for (j = 0; j < namesLength; j++)
                {
                    if (NULL == (name = DuplicateString(&(names[j]))))
                    {
                        OsConfigLogError(log, "RemoveUserAccounts: failed to duplicate string");
                        status = ENOMEM;
                        break;
                    }
                    else
                    {
                        TruncateAtFirst(name, ',');

                        if (0 == strcmp(userList[i].username, name))
                        {
                            if ((0 != (_status = RemoveUser(&(userList[i]), log))) && (0 == status))
                            {
                                status = _status;
                            }
                        }

                        j += strlen(name);
                        FREE_MEMORY(name);
                    }
                }

                if (0 != status)
                {
                    break;
                }
            }
        }

        FreeUsersList(&userList, userListSize);
    }

    if (0 == status)
    {
        for (j = 0; j < namesLength; j++)
        {
            if (NULL == (name = DuplicateString(&(names[j]))))
            {
                OsConfigLogError(log, "RemoveUserAccounts: failed to duplicate string");
                status = ENOMEM;
                break;
            }
            else
            {
                TruncateAtFirst(name, ',');

                if (NULL == (decoratedName = FormatAllocateString(userTemplate, name)))
                {
                    OsConfigLogInfo(log, "RemoveUserAccounts: out of memory");
                }
                else
                {
                    if (false == userAccountsNotFound)
                    {
                        if (0 == FindTextInFile(g_etcPasswd, decoratedName, log))
                        {
                            ReplaceMarkedLinesInFile(g_etcPasswd, decoratedName, NULL, '#', true, log);
                        }

                        if (0 == FindTextInFile(g_etcShadow, decoratedName, log))
                        {
                            ReplaceMarkedLinesInFile(g_etcShadow, decoratedName, NULL, '#', true, log);
                        }
                    }

                    if (0 == FindTextInFile(g_etcPasswdDash, decoratedName, log))
                    {
                        ReplaceMarkedLinesInFile(g_etcPasswdDash, decoratedName, NULL, '#', true, log);
                    }

                    if (0 == FindTextInFile(g_etcShadowDash, decoratedName, log))
                    {
                        ReplaceMarkedLinesInFile(g_etcShadowDash, decoratedName, NULL, '#', true, log);
                    }

                    FREE_MEMORY(decoratedName);
                }

                j += strlen(name);
                FREE_MEMORY(name);
            }
        }
    }

    if (0 == status)
    {
        OsConfigLogInfo(log, "RemoveUserAccounts: the specified user accounts '%s' either do not appear or were completely removed from this system", names);
    }

    return status;
}

int RestrictSuToRootGroup(void* log)
{
    const char* etcPamdSu = "/etc/pam.d/su";
    const char* suRestrictedToRootGroup = "auth required pam_wheel.so use_uid group=root";
    int status = 0;

    if (AppendToFile(etcPamdSu, suRestrictedToRootGroup, strlen(suRestrictedToRootGroup), log))
    {
        OsConfigLogInfo(log, "RestrictSuToRootGroup: '%s' was written to '%s'", suRestrictedToRootGroup, etcPamdSu);
    }
    else
    {
        OsConfigLogError(log, "RestrictSuToRootGroup: failed writing '%s' to '%s' (%d)", suRestrictedToRootGroup, etcPamdSu, errno);
        status = ENOENT;
    }

    return status;
}
