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

static const char* g_noLoginShell[] = { "/usr/sbin/nologin", "/sbin/nologin", "/bin/false", "/bin/true", "/usr/bin/true", "/usr/bin/false", "/dev/null", "" };

static void ResetUserEntry(SimplifiedUser* target)
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

void FreeUsersList(SimplifiedUser** source, unsigned int size)
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

static int CopyUserEntry(SimplifiedUser* destination, struct passwd* source, OsConfigLogHandle log)
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

static bool IsUserNonLogin(SimplifiedUser* user)
{
    int index = ARRAY_SIZE(g_noLoginShell);
    bool noLogin = false;

    if (user && user->shell)
    {
        while (index > 0)
        {
            if (0 == strcmp(user->shell, g_noLoginShell[index - 1]))
            {
                noLogin = true;
                break;
            }

            index--;
        }
    }

    return noLogin;
}

static int SetUserNonLogin(SimplifiedUser* user, OsConfigLogHandle log)
{
    const char* commandTemplate = "usermod -s %s %s";
    char* command = NULL;
    int noLoginShells = ARRAY_SIZE(g_noLoginShell), i = 0, result = 0;

    if ((NULL == user) || (NULL == user->username))
    {
        OsConfigLogError(log, "SetUserNonLogin: invalid argument");
        return EINVAL;
    }

    if (true == (user->noLogin = IsUserNonLogin(user)))
    {
        OsConfigLogInfo(log, "SetUserNonLogin: user '%s' (%u) is already set to be non-login", user->username, user->userId);
        return 0;
    }

    result = ENOENT;

    for (i = 0; i < noLoginShells; i++)
    {
        if (true == FileExists(g_noLoginShell[i]))
        {
            if (NULL == (command = FormatAllocateString(commandTemplate, g_noLoginShell[i], user->username)))
            {
                OsConfigLogError(log, "SetUserNonLogin: out of memory");
                result = ENOMEM;
            }
            else if (0 != (result = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
            {
                OsConfigLogInfo(log, "SetUserNonLogin: '%s' failed with %d (errno: %d)", command, result, errno);
            }
            else
            {
                OsConfigLogInfo(log, "SetUserNonLogin: user '%s' (%u) is now set to be non-login", user->username, user->userId);
            }

            FREE_MEMORY(command);

            if ((0 == result) || (ENOMEM == result))
            {
                break;
            }
        }
    }

    if (ENOENT == result)
    {
        OsConfigLogInfo(log, "SetUserNonLogin: no suitable no login shell found (to make user '%s' (%u) non-login)", user->username, user->userId);
    }

    return result;
}

static int CheckIfUserHasPassword(SimplifiedUser* user, OsConfigLogHandle log)
{
    struct spwd* shadowEntry = NULL;
    char control = 0;
    int status = 0;

    if ((NULL == user) || (NULL == user->username))
    {
        OsConfigLogError(log, "CheckIfUserHasPassword: invalid argument");
        return EINVAL;
    }

    if (true == (user->noLogin = IsUserNonLogin(user)))
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
                OsConfigLogInfo(log, "CheckIfUserHasPassword: user '%s' (%u, %u) appears to be missing password ('%c')",
                    user->username, user->userId, user->groupId, control);
                user->hasPassword = false;
        }
    }
    else
    {
        OsConfigLogInfo(log, "CheckIfUserHasPassword: getspnam('%s') failed (%d)", user->username, errno);
        status = ENOENT;
    }

    endspent();

    return status;
}

int EnumerateUsers(SimplifiedUser** userList, unsigned int* size, char** reason, OsConfigLogHandle log)
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
        listSize = (*size) * sizeof(SimplifiedUser);
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
                    OsConfigLogInfo(log, "EnumerateUsers: cannot check user's login and password (%d)", status);
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
        OsConfigLogInfo(log, "EnumerateUsers: cannot read %s", passwdFile);
        status = EPERM;
    }


    if (0 != status)
    {
        OsConfigLogInfo(log, "EnumerateUsers failed with %d", status);
        OsConfigCaptureReason(reason, "Failed to enumerate users (%d). User database may be corrupt. Automatic remediation is not possible", status);
    }
    else if (IsDebugLoggingEnabled())
    {
        OsConfigLogDebug(log, "EnumerateUsers: %u users found", *size);

        for (i = 0; i < *size; i++)
        {
            OsConfigLogDebug(log, "EnumerateUsers(user %u): name '%s', uid %d, gid %d, home '%s', shell '%s'", i,
                (*userList)[i].username, (*userList)[i].userId, (*userList)[i].groupId, (*userList)[i].home, (*userList)[i].shell);
        }
    }

    return status;
}

void FreeGroupList(SimplifiedGroup** groupList, unsigned int size)
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

int EnumerateUserGroups(SimplifiedUser* user, SimplifiedGroup** groupList, unsigned int* size, char** reason, OsConfigLogHandle log)
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
        OsConfigLogDebug(log, "EnumerateUserGroups: first call to getgrouplist for user '%s' (%u) returned %d and %d",
            user->username, user->groupId, getGroupListResult, numberOfGroups);
        FREE_MEMORY(groupIds);

        if (0 < numberOfGroups)
        {
            if (NULL != (groupIds = malloc(numberOfGroups * sizeof(gid_t))))
            {
                getGroupListResult = getgrouplist(user->username, user->groupId, groupIds, &numberOfGroups);
                OsConfigLogDebug(log, "EnumerateUserGroups: second call to getgrouplist for user '%s' (%u) returned %d and %d",
                    user->username, user->groupId, getGroupListResult, numberOfGroups);
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
            OsConfigLogInfo(log, "EnumerateUserGroups: first call to getgrouplist for user '%s' (%u) returned -1 and %d groups",
                user->username, user->groupId, numberOfGroups);
            status = ENOENT;
        }
    }

    if ((0 == status) && (0 < numberOfGroups))
    {
        OsConfigLogDebug(log, "EnumerateUserGroups: user '%s' (%u) is in %d group%s", user->username, user->groupId, numberOfGroups, (1 == numberOfGroups) ? "" : "s");

        if (NULL == (*groupList = malloc(sizeof(SimplifiedGroup) * numberOfGroups)))
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
                    OsConfigLogInfo(log, "EnumerateUserGroups: getgrgid(for gid: %u) failed (errno: %d)", (unsigned int)groupIds[i], errno);
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

                        OsConfigLogDebug(log, "EnumerateUserGroups: user '%s' (%u) is in group '%s' (%u)", user->username, user->groupId, (*groupList)[i].groupName, (*groupList)[i].groupId);
                    }
                    else
                    {
                        OsConfigLogError(log, "EnumerateUserGroups: out of memory");
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

int EnumerateAllGroups(SimplifiedGroup** groupList, unsigned int* size, char** reason, OsConfigLogHandle log)
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
        listSize = (*size) * sizeof(SimplifiedGroup);
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

                        OsConfigLogDebug(log, "EnumerateAllGroups(group %d): group name '%s', gid %u, %s", i,
                            (*groupList)[i].groupName, (*groupList)[i].groupId, (*groupList)[i].hasUsers ? "has users" : "empty");
                    }
                    else
                    {
                        OsConfigLogError(log, "EnumerateAllGroups: out of memory");
                        status = ENOMEM;
                        break;
                    }
                }

                i += 1;
            }

            endgrent();

            OsConfigLogDebug(log, "EnumerateAllGroups: found %u groups (expected %u)", i, *size);

            *size = i;
        }
        else
        {
            OsConfigLogError(log, "EnumerateAllGroups: out of memory");
            status = ENOMEM;
        }
    }
    else
    {
        OsConfigLogInfo(log, "EnumerateGroups: cannot read %s", groupFile);
        status = EPERM;
    }

    if (status)
    {
        OsConfigCaptureReason(reason, "Failed to enumerate user groups (%d). User group database may be corrupt. Automatic remediation is not possible", status);
    }

    return status;
}

int CheckAllEtcPasswdGroupsExistInEtcGroup(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
    unsigned int userListSize = 0;
    struct SimplifiedGroup* userGroupList = NULL;
    unsigned int userGroupListSize = 0;
    struct SimplifiedGroup* groupList = NULL;
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
                            OsConfigLogDebug(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: group '%s' (%u) of user '%s' (%u) found in '/etc/group'",
                                userList[i].username, userList[i].userId, userGroupList[j].groupName, userGroupList[j].groupId);
                            found = true;
                            break;
                        }
                    }

                    if (false == found)
                    {
                        OsConfigLogInfo(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: group '%s' (%u) of user '%s' (%u) not found in '/etc/group'",
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

int SetAllEtcPasswdGroupsToExistInEtcGroup(OsConfigLogHandle log)
{
    const char* commandTemplate = "gpasswd -d %u %u";
    char* command = NULL;
    SimplifiedUser* userList = NULL;
    unsigned int userListSize = 0;
    struct SimplifiedGroup* userGroupList = NULL;
    unsigned int userGroupListSize = 0;
    struct SimplifiedGroup* groupList = NULL;
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
                            OsConfigLogDebug(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: group '%s' (%u) of user '%s' (%u) found in '/etc/group'",
                                userGroupList[j].groupName, userGroupList[j].groupId, userList[i].username, userList[i].userId);
                            found = true;
                            break;
                        }
                    }

                    if (false == found)
                    {
                        OsConfigLogInfo(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: group '%s' (%u) of user '%s' (%u) not found in '/etc/group'",
                            userGroupList[j].groupName, userGroupList[j].groupId, userList[i].username, userList[i].userId);

                        if (NULL != (command = FormatAllocateString(commandTemplate, userList[i].userId, userGroupList[j].groupId)))
                        {
                            if (0 == (_status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
                            {
                                OsConfigLogInfo(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: user '%s' (%u) was removed from group '%s' (%u)",
                                    userList[i].username, userList[i].userId, userGroupList[j].groupName, userGroupList[j].groupId);
                            }
                            else
                            {
                                OsConfigLogInfo(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: 'gpasswd -d %u %u' failed with %d",
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

int CheckNoDuplicateUidsExist(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                        OsConfigLogInfo(log, "CheckNoDuplicateUidsExist: uid %u appears more than a single time in '/etc/passwd'", userList[i].userId);
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

int RemoveUser(SimplifiedUser* user, bool removeHome, OsConfigLogHandle log)
{
    const char* commandTemplate = "userdel %s %s";
    char* command = NULL;
    int status = 0, _status = 0;

    if (NULL == user)
    {
        OsConfigLogError(log, "RemoveUser: invalid argument");
        return EINVAL;
    }
    else if (0 == user->userId)
    {
        OsConfigLogInfo(log, "RemoveUser: cannot remove user with uid 0 ('%s' %u, %u)", user->username, user->userId, user->groupId);
        return EPERM;
    }

    if (NULL != (command = FormatAllocateString(commandTemplate, removeHome ? "-f -r" : "-f", user->username)))
    {
        if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogInfo(log, "RemoveUser: removed user '%s' (%u, %u, '%s')", user->username, user->userId, user->groupId, user->home);

            if (DirectoryExists(user->home))
            {
                OsConfigLogInfo(log, "RemoveUser: home directory of user '%s' remains ('%s') and needs to be manually deleted", user->username, user->home);
            }
            else
            {
                OsConfigLogInfo(log, "RemoveUser: home directory of user '%s' successfully removed ('%s')", user->username, user->home);
            }
        }
        else
        {
            OsConfigLogInfo(log, "RemoveUser: cannot remove user '%s' (%u, %u) (%d)", user->username, user->userId, user->groupId, _status);
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

int RemoveUser2(const char* username, OsConfigLogHandle log)
{
    const char* commandTemplate = "userdel -f %s";
    char command[64] = { 0 };
    int status = 0;
    int commandLen = 0;

    if (NULL == username)
    {
        OsConfigLogError(log, "RemoveUser: invalid argument");
        return EINVAL;
    }

    commandLen = snprintf(command, sizeof(command), commandTemplate, username);
    if((int)sizeof(command) <= commandLen || 0 > commandLen)
    {
        OsConfigLogError(log, "RemoveUser: command buffer too small");
        return ENOMEM;
    }

    if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogInfo(log, "RemoveUser: removed user '%s'", username);
    }
    else
    {
        OsConfigLogInfo(log, "RemoveUser: cannot remove user '%s' (%d)", username, status);
    }

    return status;
}

int CheckNoDuplicateGidsExist(char** reason, OsConfigLogHandle log)
{
    SimplifiedGroup* groupList = NULL;
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
                        OsConfigLogInfo(log, "CheckNoDuplicateGidsExist: gid %u appears more than a single time in '/etc/group'", groupList[i].groupId);
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

int CheckNoDuplicateUserNamesExist(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                        OsConfigLogInfo(log, "CheckNoDuplicateUserNamesExist: username '%s' appears more than a single time in '/etc/passwd'", userList[i].username);
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

int CheckNoDuplicateGroupNamesExist(char** reason, OsConfigLogHandle log)
{
    SimplifiedGroup* groupList = NULL;
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
                        OsConfigLogInfo(log, "CheckNoDuplicateGroupNamesExist: group name '%s' appears more than a single time in '/etc/group'", groupList[i].groupName);
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

int CheckShadowGroupIsEmpty(char** reason, OsConfigLogHandle log)
{
    SimplifiedGroup* groupList = NULL;
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
                OsConfigLogInfo(log, "CheckShadowGroupIsEmpty: group 'shadow' (%u) is not empty", groupList[i].groupId);
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

int SetShadowGroupEmpty(OsConfigLogHandle log)
{
    const char* commandTemplate = "gpasswd -d %s %s";
    char* command = NULL;
    SimplifiedUser* userList = NULL;
    unsigned int userListSize = 0;
    struct SimplifiedGroup* userGroupList = NULL;
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
                                OsConfigLogInfo(log, "SetShadowGroupEmpty: user '%s' (%u) was removed from group '%s' (%u)",
                                    userList[i].username, userList[i].userId, userGroupList[j].groupName, userGroupList[j].groupId);
                            }
                            else
                            {
                                OsConfigLogInfo(log, "SetShadowGroupEmpty: 'gpasswd -d %s %s' failed with %d", userList[i].username, g_shadow, _status);
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

int CheckRootGroupExists(char** reason, OsConfigLogHandle log)
{
    SimplifiedGroup* groupList = NULL;
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
        OsConfigLogInfo(log, "CheckRootGroupExists: root group with gid 0 not found");
        OsConfigCaptureReason(reason, "Root group with gid 0 not found");
        status = ENOENT;
    }

    return status;
}

int RepairRootGroup(OsConfigLogHandle log)
{
    const char* etcGroup = "/etc/group";
    const char* rootLine = "root:x:0:\n";
    const char* tempFileName = "/etc/~group";
    SimplifiedGroup* groupList = NULL;
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
                                    OsConfigLogInfo(log, "RepairRootGroup:  RenameFileWithOwnerAndAccess('%s' to '%s') returned %d",
                                        tempFileName, etcGroup, status);
                                }
                            }
                            else
                            {
                                OsConfigLogInfo(log, "RepairRootGroup: cannot append to to temp file '%s' (%d)", tempFileName, errno);
                                status = ENOENT;
                            }

                            remove(tempFileName);
                        }
                    }
                    else
                    {
                        OsConfigLogInfo(log, "RepairRootGroup: cannot read from '%s' (%d)", tempFileName, errno);
                        status = EACCES;
                    }
                }
                else
                {
                    OsConfigLogInfo(log, "RepairRootGroup: cannot remove potentially corrupted root entries from '%s' (%d)", etcGroup, errno);
                }
            }
            else
            {
                OsConfigLogInfo(log, "RepairRootGroup: cannot save to temp file '%s' (%d)", tempFileName, errno);
                status = EPERM;
            }

            FREE_MEMORY(original);
        }
        else
        {
            OsConfigLogInfo(log, "RepairRootGroup: cannot read from '%s' (%d)", etcGroup, errno);
            status = EACCES;
        }
    }

    if (0 == status)
    {
        OsConfigLogInfo(log, "RepairRootGroup: root group exists with gid 0");
    }

    return status;
}

int CheckAllUsersHavePasswordsSet(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user '%s' (%u, %u) not found to have a password set",
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

int RemoveUsersWithoutPasswords(OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user '%s' (%u, %u) can login and has no password set",
                    userList[i].username, userList[i].userId, userList[i].groupId);

                if (0 == userList[i].userId)
                {
                    OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: the root account's password must be manually fixed");
                    status = EPERM;
                }
                else if ((0 != (_status = RemoveUser(&(userList[i]), false, log))) && (0 == status))
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

int CheckRootIsOnlyUidZeroAccount(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (((NULL == userList[i].username) || (0 != strcmp(userList[i].username, g_root))) && (0 == userList[i].userId))
            {
                OsConfigLogInfo(log, "CheckRootIsOnlyUidZeroAccount: user '%s' (%u, %u) is not root but has uid 0",
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

int SetRootIsOnlyUidZeroAccount(OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if (((NULL == userList[i].username) || (0 != strcmp(userList[i].username, g_root))) && (0 == userList[i].userId))
            {
                OsConfigLogInfo(log, "SetRootIsOnlyUidZeroAccount: user '%s' (%u, %u) is not root but has uid 0",
                    userList[i].username, userList[i].userId, userList[i].groupId);

                if ((0 != (_status = RemoveUser(&(userList[i]), false, log))) && (0 == status))
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

int CheckDefaultRootAccountGroupIsGidZero(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
    unsigned int userListSize = 0;
    unsigned int i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((0 == strcmp(userList[i].username, g_root)) && (0 == userList[i].userId) && (0 != userList[i].groupId))
            {
                OsConfigLogInfo(log, "CheckDefaultRootAccountuserIsGidZero: root user '%s' (%u) has default gid %u instead of gid 0",
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

int SetDefaultRootAccountGroupIsGidZero(OsConfigLogHandle log)
{
    int status = 0;

    if (0 != (status = CheckDefaultRootAccountGroupIsGidZero(NULL, log)))
    {
        status = RepairRootGroup(log);
    }

    return status;
}

int CheckAllUsersHomeDirectoriesExist(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                OsConfigLogInfo(log, "CheckAllUsersHomeDirectoriesExist: user '%s' (%u, %u) home directory '%s' not found or is not a directory",
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

int SetUserHomeDirectories(OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    unsigned int defaultHomeDirAccess = 0750;
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
                    OsConfigLogInfo(log, "SetUserHomeDirectories: user '%s' (%u, %u) home directory '%s' not found",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);

                    if (0 == (_status = mkdir(userList[i].home, defaultHomeDirAccess)))
                    {
                        OsConfigLogInfo(log, "SetUserHomeDirectories: user '%s' (%u, %u) has now home directory '%s'",
                            userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                    }
                    else
                    {
                        _status = (0 == errno) ? EACCES : errno;
                        OsConfigLogInfo(log, "SetUserHomeDirectories: cannot create home directory '%s' for user '%s' (%u, %u) (%d)",
                            userList[i].home, userList[i].username, userList[i].userId, userList[i].groupId, _status);
                    }
                }

                // If the home directory does not have correct ownership and access, correct this
                if (true == DirectoryExists(userList[i].home))
                {
                    if (0 != (_status = SetDirectoryAccess(userList[i].home, userList[i].userId, userList[i].groupId, defaultHomeDirAccess, log)))
                    {
                        OsConfigLogInfo(log, "SetUserHomeDirectories: cannot set access and ownership for home directory '%s' of user '%s' (%u, %u) (%d, errno: %d)",
                            userList[i].home, userList[i].username, userList[i].userId, userList[i].groupId, _status, errno);
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

static int CheckHomeDirectoryOwnership(SimplifiedUser* user, OsConfigLogHandle log)
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
            OsConfigLogInfo(log, "CheckHomeDirectoryOwnership: stat('%s') failed with %d", user->home, errno);
        }
    }
    else
    {
        OsConfigLogInfo(log, "CheckHomeDirectoryOwnership: directory '%s' is not found, nothing to check", user->home);
    }

    return status;
}

int CheckUsersOwnTheirHomeDirectories(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                    OsConfigLogInfo(log, "CheckUsersOwnTheirHomeDirectories: user '%s' (%u, %u) does not own their assigned home directory '%s'",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                    OsConfigCaptureReason(reason, "User '%s' (%u, %u) does not own their assigned home directory '%s'",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home);
                    status = ENOENT;
                }
            }
            else
            {
                OsConfigLogInfo(log, "CheckUsersOwnTheirHomeDirectories: user '%s' (%u, %u) assigned home directory '%s' does not exist",
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

int CheckRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                    OsConfigLogInfo(log, "CheckRestrictedUserHomeDirectories: user '%s' (%u, %u) does not have proper restricted access for their assigned home directory '%s'",
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

int SetRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, unsigned int modeForRoot, unsigned int modeForOthers, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                        OsConfigLogInfo(log, "SetRestrictedUserHomeDirectories: cannot set restricted access (%u) for user '%s' (%u, %u) assigned home directory '%s' (%d)",
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

int CheckPasswordHashingAlgorithm(unsigned int algorithm, char** reason, OsConfigLogHandle log)
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
            OsConfigLogInfo(log, "CheckPasswordHashingAlgorithm: the user password encryption algorithm currently set in '/etc/login.defs' to '%s' is different from the required '%s' (%d) ",
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

        OsConfigLogInfo(log, "CheckPasswordHashingAlgorithm: cannot read 'ENCRYPT_METHOD' from '/etc/login.defs' (%d)", status);
        OsConfigCaptureReason(reason, "Failed to read 'ENCRYPT_METHOD' from '/etc/login.defs' (%d)", status);
    }

    return status;
}

int SetPasswordHashingAlgorithm(unsigned int algorithm, OsConfigLogHandle log)
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
            OsConfigLogInfo(log, "SetPasswordHashingAlgorithm: cannot set 'ENCRYPT_METHOD' to '%s' in '/etc/login.defs' (%d)", encryption, status);
        }
    }

    return status;
}

int CheckMinDaysBetweenPasswordChanges(long days, char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                    OsConfigLogInfo(log, "CheckMinDaysBetweenPasswordChanges: user '%s' (%u, %u) minimum time between password changes of %ld days is less than requested %ld days",
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
        OsConfigLogInfo(log, "CheckMinDaysBetweenPasswordChanges: there is no configured PASS_MIN_DAYS in /etc/login.defs");
        OsConfigCaptureReason(reason, "There is no configured 'PASS_MIN_DAYS' in '/etc/login.defs'");
        status = ENOENT;
    }
    else if (0 == etcLoginDefsDays)
    {
        OsConfigLogInfo(log, "CheckMinDaysBetweenPasswordChanges: PASS_MIN_DAYS is configured to default 0 in /etc/login.defs meaning disabled restriction");
        OsConfigCaptureReason(reason, "'PASS_MIN_DAYS' is configured to default 0 in '/etc/login.defs' meaning disabled restriction");
        status = ENOENT;
    }
    else if (etcLoginDefsDays < days)
    {
        OsConfigLogInfo(log, "CheckMinDaysBetweenPasswordChanges: configured PASS_MIN_DAYS in /etc/login.defs %ld days is less than requested %ld days", etcLoginDefsDays, days);
        OsConfigCaptureReason(reason, "Configured 'PASS_MIN_DAYS' in '/etc/login.defs' of %ld days is less than requested %ld days", etcLoginDefsDays, days);
        status = ENOENT;
    }
    else
    {
        OsConfigCaptureSuccessReason(reason, "'PASS_MIN_DAYS' is set to %ld days in '/etc/login.defs' (requested: %ld)", etcLoginDefsDays, days);
    }

    return status;
}

int SetMinDaysBetweenPasswordChanges(long days, OsConfigLogHandle log)
{
    const char* commandTemplate = "chage -m %ld %s";
    char* command = NULL;
    SimplifiedUser* userList = NULL;
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
        OsConfigLogInfo(log, "SetMinDaysBetweenPasswordChanges: cannot set 'PASS_MIN_DAYS' to %ld days in '/etc/login.defs' (%d)", days, _status);
    }

    if (_status && (0 == status))
    {
        status = _status;
    }

    return status;
}

int CheckMaxDaysBetweenPasswordChanges(long days, char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                    OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: user '%s' (%u, %u) has unlimited time between password changes of %ld days (requested: %ld)",
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
                    OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: user '%s' (%u, %u) maximum time between password changes of %ld days is more than requested %ld days",
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
        OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: there is no configured PASS_MAX_DAYS in /etc/login.defs");
        OsConfigCaptureReason(reason, "'PASS_MAX_DAYS' is not configured in '/etc/login.defs'");
        status = ENOENT;
    }
    else if (etcLoginDefsDays > days)
    {
        OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: configured PASS_MAX_DAYS in /etc/login.defs %ld days is more than requested %ld days", etcLoginDefsDays, days);
        OsConfigCaptureReason(reason, "Configured 'PASS_MAX_DAYS' in '/etc/login.defs' of %ld days is more than requested %ld days", etcLoginDefsDays, days);
        status = ENOENT;
    }
    else
    {
        OsConfigCaptureSuccessReason(reason, "'PASS_MAX_DAYS' is set to %ld days in '/etc/login.defs' (requested: %ld)", etcLoginDefsDays, days);
    }

    return status;
}

int SetMaxDaysBetweenPasswordChanges(long days, OsConfigLogHandle log)
{
    const char* commandTemplate = "chage -M %ld %s";
    char* command = NULL;
    SimplifiedUser* userList = NULL;
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
        OsConfigLogInfo(log, "SetMaxDaysBetweenPasswordChanges: cannot set 'PASS_MAX_DAYS' to %ld days in '/etc/login.defs' (%d)", days, _status);
    }

    if (_status & (0 == status))
    {
        status = _status;
    }

    return status;
}

int EnsureUsersHaveDatesOfLastPasswordChanges(OsConfigLogHandle log)
{
    const char* commandTemplate = "chage -d %ld %s";
    char* command = NULL;
    SimplifiedUser* userList = NULL;
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

int CheckPasswordExpirationLessThan(long days, char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                    OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: password for user '%s' (%u, %u) has no expiration date (%ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge);
                    OsConfigCaptureReason(reason, "Password for user '%s' (%u, %u) has no expiration date (%ld)",
                        userList[i].username, userList[i].userId, userList[i].groupId, userList[i].maximumPasswordAge);
                    status = ENOENT;
                }
                else if (userList[i].lastPasswordChange < 0)
                {
                    OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: password for user '%s' (%u, %u) has no recorded change date (%ld)",
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
                            OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: password for user '%s' (%u, %u) will expire in %ld days, more than requested maximum of %ld days",
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

int CheckPasswordExpirationWarning(long days, char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                    OsConfigLogInfo(log, "CheckPasswordExpirationWarning: user '%s' (%u, %u) password expiration warning time is %ld days, less than requested %ld days",
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
        OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: there is no configured PASS_WARN_AGE in /etc/login.defs");
        OsConfigCaptureReason(reason, "'PASS_WARN_AGE' is not configured in '/etc/login.defs'");
        status = ENOENT;
    }
    else if (etcLoginDefsDays < days)
    {
        OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: configured PASS_WARN_AGE in /etc/login.defs %ld days is less than requested %ld days", etcLoginDefsDays, days);
        OsConfigCaptureReason(reason, "Configured 'PASS_WARN_AGE' in '/etc/login.defs' of %ld days is less than requested %ld days", etcLoginDefsDays, days);
        status = ENOENT;
    }
    else
    {
        OsConfigCaptureSuccessReason(reason, "'PASS_WARN_AGE' is set to %ld days in '/etc/login.defs' (requested: %ld)", etcLoginDefsDays, days);
    }

    return status;
}

int SetPasswordExpirationWarning(long days, OsConfigLogHandle log)
{
    const char* commandTemplate = "chage -W %ld %s";
    char* command = NULL;
    SimplifiedUser* userList = NULL;
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
                OsConfigLogInfo(log, "SetPasswordExpirationWarning: user '%s' (%u, %u) password expiration warning time is %ld days, less than requested %ld days",
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
        OsConfigLogInfo(log, "SetPasswordExpirationWarning: cannot set 'PASS_WARN_AGE' to %ld days in '/etc/login.defs' (%d)", days, _status);
    }

    if (_status && (0 == status))
    {
        status = _status;
    }

    return status;
}

int CheckUsersRecordedPasswordChangeDates(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
                    OsConfigLogInfo(log, "CheckUsersRecordedPasswordChangeDates: user '%s' (%u, %u) last recorded password change is in the future (next %ld days)",
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

int CheckLockoutAfterInactivityLessThan(long days, char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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

int SetLockoutAfterInactivityLessThan(long days, OsConfigLogHandle log)
{
    const char* commandTemplate = "chage -I %ld %s";
    char* command = NULL;
    SimplifiedUser* userList = NULL;
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

int CheckSystemAccountsAreNonLogin(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, reason, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((userList[i].isLocked || userList[i].noLogin || userList[i].cannotLogin) && userList[i].hasPassword && userList[i].userId)
            {
                OsConfigLogInfo(log, "CheckSystemAccountsAreNonLogin: user '%s' (%u, %u, '%s', '%s') is either locked, no-login, or cannot-login, "
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

int SetSystemAccountsNonLogin(OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
    unsigned int userListSize = 0, i = 0;
    int status = 0, _status = 0;

    if (0 == (status = EnumerateUsers(&userList, &userListSize, NULL, log)))
    {
        for (i = 0; i < userListSize; i++)
        {
            if ((userList[i].isLocked || userList[i].noLogin || userList[i].cannotLogin) && userList[i].hasPassword && userList[i].userId)
            {
                OsConfigLogInfo(log, "SetSystemAccountsNonLogin: user '%s' (%u, %u, '%s', '%s') is either locked, non-login, or cannot-login, "
                    "but can login with password",  userList[i].username, userList[i].userId, userList[i].groupId, userList[i].home, userList[i].shell);

                // If the account is not already true non-login, try to make it non-login and if that does not work, remove the account
                if (0 != (_status = SetUserNonLogin(&(userList[i]), log)))
                {
                    _status = RemoveUser(&(userList[i]), false, log);
                }

                // Do not overwrite a previous non zero status value if any
                if (_status && (0 == status))
                {
                    status = _status;
                }
            }
        }
    }

    FreeUsersList(&userList, userListSize);

    if (0 == status)
    {
        OsConfigLogInfo(log, "SetSystemAccountsNonLogin: all system accounts are non-login");
    }

    return status;
}

int CheckRootPasswordForSingleUserMode(char** reason, OsConfigLogHandle log)
{
    SimplifiedUser* userList = NULL;
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
            OsConfigLogInfo(log, "CheckRootPasswordForSingleUserMode: single user mode and root does not have password");
            OsConfigCaptureReason(reason, "Single user mode and root does not have a password set, must manually set a password "
                "for root user, automatic remediation is not possible");
            status = ENOENT;
        }
    }

    return status;
}

int CheckOrEnsureUsersDontHaveDotFiles(const char* name, bool removeDotFiles, char** reason, OsConfigLogHandle log)
{
    const char* templateDotPath = "%s/.%s";

    SimplifiedUser* userList = NULL;
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
                            OsConfigLogInfo(log, "CheckOrEnsureUsersDontHaveDotFiles: for user '%s' (%u, %u), '%s' needs to be manually removed",
                                userList[i].username, userList[i].userId, userList[i].groupId, dotPath);
                            status = ENOENT;
                        }
                    }
                    else
                    {
                        OsConfigLogInfo(log, "CheckOrEnsureUsersDontHaveDotFiles: user '%s' (%u, %u) has file '.%s' ('%s')",
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

int CheckUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, char** reason, OsConfigLogHandle log)
{
    const char* pathTemplate = "%s/%s";

    SimplifiedUser* userList = NULL;
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
                            OsConfigLogInfo(log, "CheckUsersRestrictedDotFiles: user '%s' (%u, %u) does not has have proper restricted access for their dot file '%s'",
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

int SetUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, unsigned int mode, OsConfigLogHandle log)
{
    const char* pathTemplate = "%s/%s";

    SimplifiedUser* userList = NULL;
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
                                OsConfigLogInfo(log, "SetUsersRestrictedDotFiles: cannot set restricted access (%u) for user '%s' (%u, %u) dot file '%s'",
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

int CheckUserAccountsNotFound(const char* names, char** reason, OsConfigLogHandle log)
{
    const char* userTemplate = "%s:";
    size_t namesLength = 0;
    char* name = NULL;
    char* decoratedName = NULL;
    SimplifiedUser* userList = NULL;
    SimplifiedGroup* groupList = NULL;
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
                    OsConfigLogError(log, "CheckUserAccountsNotFound: out of memory, unable to check for user '%s' presence in '%s' andr '%s'",
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

int RemoveUserAccounts(const char* names, OsConfigLogHandle log)
{
    int status = 0, _status = 0;
    struct passwd* pw;

    if (NULL == names)
    {
        OsConfigLogError(log, "RemoveUserAccounts: invalid argument");
        return EINVAL;
    }

    setpwent();
    for (errno = 0, pw = getpwent(); pw != NULL; errno = 0, pw = getpwent())
    {
        char* usernames = NULL;
        char* token = NULL;

        usernames = strdup(names);
        if (NULL == usernames)
        {
            OsConfigLogError(log, "RemoveUserAccounts: failed to duplicate string");
            status = ENOMEM;
            break;
        }

        for (token = strtok(usernames, ","); token != NULL; token = strtok(NULL, ","))
        {
            // Skip root user
            if (pw->pw_uid == 0)
            {
                continue;
            }

            if (0 == strcmp(pw->pw_name, token))
            {
                if ((0 != (_status = RemoveUser2(pw->pw_name, log))) && (0 == status))
                {
                    status = _status;
                }
            }
        }

        FREE_MEMORY(usernames);
    }
    if (status == 0 && 0 != errno)
    {
        status = errno;
        OsConfigLogError(log, "RemoveUserAccounts: getpwent() failed with error %d", status);
    }
    endpwent();

    if (0 != status)
    {
        OsConfigLogError(log, "RemoveUserAccounts: failed to remove user accounts '%s' from '%s'", names, g_etcPasswd);
        return status;
    }

    OsConfigLogInfo(log, "RemoveUserAccounts: the specified user accounts '%s' either do not appear or were completely removed from this system", names);
    return status;
}

int RestrictSuToRootGroup(OsConfigLogHandle log)
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
        OsConfigLogInfo(log, "RestrictSuToRootGroup: cannot write '%s' to '%s' (%d)", suRestrictedToRootGroup, etcPamdSu, errno);
        status = ENOENT;
    }

    return status;
}
