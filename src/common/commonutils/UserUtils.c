// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"
#include "UserUtils.h"

#define MAX_GROUPS_USER_CAN_BE_IN 32
#define NUMBER_OF_SECONDS_IN_A_DAY 86400

static const char* g_root = "root";
static const char* g_shadow = "shadow";
static const char* g_passwdFile = "/etc/passwd";
static const char* g_redacted = "***";

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
        target->notInShadow = false;
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
            OsConfigLogError(log, "CopyUserEntry: out of memory copying pw_name for user %u", source->pw_uid);
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

// For logging purposes, we identify an user account as a system account if either has name "root", or has a no-loging shell,
// or has an UID below 1000. For non-system accounts we redact usernames and home names, for system accounts we log everything.
// We do this in order to log in full clear deviant accounts (that for example use a no-login shell while having UID above 1000)
static bool IsSystemAccount(SimplifiedUser* user)
{
    return (user && ((user->username && (0 == strcmp(user->username, g_root))) || IsUserNonLogin(user) || (user->userId < 1000))) ? true : false;
}

// Similar to determining if an user account is system, we identify a group to be system if either
// has name "root" or has a GID below 1000 (and all such system groups get logged in full)
static bool IsSystemGroup(SimplifiedGroup* group)
{
    return (group && ((group->groupName && (0 == strcmp(group->groupName, g_root))) || (group->groupId < 1000))) ? true : false;
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
        OsConfigLogInfo(log, "SetUserNonLogin: user %u is already set to be non-login", user->userId);
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
                OsConfigLogInfo(log, "SetUserNonLogin: usermod for user %u failed with %d (errno: %d)", user->userId, result, errno);
            }
            else
            {
                OsConfigLogInfo(log, "SetUserNonLogin: user %u is now set to be non-login", user->userId);
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
        OsConfigLogInfo(log, "SetUserNonLogin: no suitable 'no login shell' found (to make user %u non-login)", user->userId);
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
                OsConfigLogInfo(log, "CheckIfUserHasPassword: user %u appears to be missing password ('%c')", user->userId, control);
                user->hasPassword = false;
        }
    }
    else if (0 == errno)
    {
        OsConfigLogInfo(log, "CheckIfUserHasPassword: user %u is not found in shadow database (/etc/shadow), this may indicate a remote or federated user, we cannot check if this user has a password", user->userId);
        user->hasPassword = false;
        user->notInShadow = true;
    }
    else
    {
        OsConfigLogInfo(log, "CheckIfUserHasPassword: getspnam for user %u failed with %d (%s)", user->userId, errno, strerror(errno));
        status = ENOENT;
    }

    endspent();

    return status;
}

int EnumerateUsers(SimplifiedUser** userList, unsigned int* size, char** reason, OsConfigLogHandle log)
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

    if (0 != (*size = GetNumberOfLinesInFile(g_passwdFile)))
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
        OsConfigLogInfo(log, "EnumerateUsers: cannot read %s", g_passwdFile);
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
            OsConfigLogDebug(log, "EnumerateUsers(user %u): uid %d, name '%s', gid %d, home '%s', shell '%s'", i, (*userList)[i].userId,
                IsSystemAccount(&(*userList)[i]) ? (*userList)[i].username : g_redacted, (*userList)[i].groupId,
                IsSystemAccount(&(*userList)[i]) ? (*userList)[i].home : g_redacted, (*userList)[i].shell);
        }
    }

    return status;
}

void FreeGroupList(SimplifiedGroup** groupList, unsigned int size)
{
    unsigned int i = 0;

    if ((NULL != groupList) && ((NULL != *groupList)))
    {
        for (i = 0; i < size; i++)
        {
            {
                FREE_MEMORY(((*groupList)[i]).groupName);
            }
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
    size_t listSize = numberOfGroups * sizeof(gid_t);
    int i = 0, j = 0;
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

    if (NULL == (groupIds = malloc(listSize)))
    {
        OsConfigLogError(log, "EnumerateUserGroups: out of memory allocating list of %d group identifiers", numberOfGroups);
        numberOfGroups = 0;
        status = ENOMEM;
    }
    else
    {
        memset(groupIds, 0, listSize);
        if (-1 == (getGroupListResult = getgrouplist(user->username, user->groupId, groupIds, &numberOfGroups)))
        {
            OsConfigLogDebug(log, "EnumerateUserGroups: first call to getgrouplist for user %u (%u) returned %d and %d", user->userId, user->groupId, getGroupListResult, numberOfGroups);
            FREE_MEMORY(groupIds);

            if (0 < numberOfGroups)
            {
                listSize = numberOfGroups * sizeof(gid_t);

                if (NULL != (groupIds = malloc(listSize)))
                {
                    memset(groupIds, 0, listSize);
                    getGroupListResult = getgrouplist(user->username, user->groupId, groupIds, &numberOfGroups);
                    OsConfigLogDebug(log, "EnumerateUserGroups: second call to getgrouplist for user '%u' (%u) returned %d and %d", user->userId, user->groupId, getGroupListResult, numberOfGroups);
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
                OsConfigLogInfo(log, "EnumerateUserGroups: first call to getgrouplist for user %u (%u) returned -1 and %d groups", user->userId, user->groupId, numberOfGroups);
                status = ENOENT;
            }
        }
    }

    if ((0 == status) && (0 < numberOfGroups))
    {
        OsConfigLogDebug(log, "EnumerateUserGroups: user %u ('%s', gid: %u) is in %d group%s",
            user->userId, IsSystemAccount(user) ? user->username : g_redacted, user->groupId, numberOfGroups, (1 == numberOfGroups) ? "" : "s");

        listSize = sizeof(SimplifiedGroup)* numberOfGroups;

        if (NULL == (*groupList = malloc(listSize)))
        {
            OsConfigLogError(log, "EnumerateUserGroups: out of memory");
            status = ENOMEM;
        }
        else
        {
            memset(*groupList, 0, listSize);

            *size = numberOfGroups;

            for (i = 0, j = 0; i < numberOfGroups; i++)
            {
                errno = 0;

                if (NULL != (groupEntry = getgrgid(groupIds[i])))
                {
                    (*groupList)[j].groupId = groupEntry->gr_gid;
                    (*groupList)[j].groupName = NULL;
                    (*groupList)[j].hasUsers = true;

                    if (0 < (groupNameLength = (groupEntry->gr_name ? strlen(groupEntry->gr_name) : 0)))
                    {
                        if (NULL != ((*groupList)[j].groupName = malloc(groupNameLength + 1)))
                        {
                            memset((*groupList)[j].groupName, 0, groupNameLength + 1);
                            memcpy((*groupList)[j].groupName, groupEntry->gr_name, groupNameLength);

                            OsConfigLogDebug(log, "EnumerateUserGroups: user %u ('%s', gid: %u) is in group %u ('%s')",
                                user->userId, IsSystemAccount(user) ? user->username : g_redacted, user->groupId,
                                (*groupList)[j].groupId, IsSystemGroup(&(*groupList)[j]) ? (*groupList)[j].groupName : g_redacted);

                            j += 1;
                        }
                        else
                        {
                            OsConfigLogError(log, "EnumerateUserGroups: out of memory");
                            status = ENOMEM;
                            break;
                        }
                    }
                }
                else
                {
                    if (0 == errno)
                    {
                        OsConfigLogInfo(log, "EnumerateUserGroups: group %u does not exist (errno: %d)", (unsigned int)groupIds[i], errno);
                        *size -= 1;
                    }
                    else
                    {
                        OsConfigLogInfo(log, "EnumerateUserGroups: getgrgid(for gid: %u) failed (errno: %d)", (unsigned int)groupIds[i], errno);
                        status = errno ? errno : ENOENT;
                        break;
                    }
                }
            }
        }
    }

    // If none of the groups were found
    if (0 == *size)
    {
        FREE_MEMORY(*groupList);
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
                            IsSystemGroup(&(*groupList)[i]) ? (*groupList)[i].groupName : g_redacted, (*groupList)[i].groupId, (*groupList)[i].hasUsers ? "has users" : "empty");
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
                            OsConfigLogDebug(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: group %u of user %u found in '/etc/group'", userGroupList[j].groupId, userList[i].userId);
                            found = true;
                            break;
                        }
                    }

                    if (false == found)
                    {
                        OsConfigLogInfo(log, "CheckAllEtcPasswdGroupsExistInEtcGroup: group %u of user %u not found in '/etc/group'", userGroupList[j].groupId, userList[i].userId);
                        OsConfigCaptureReason(reason, "Group %u of user %u not found in '/etc/group'", userGroupList[j].groupId, userList[i].userId);
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
                            OsConfigLogDebug(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: group '%s' (%u) of user %u found in '/etc/group'",
                                userGroupList[j].groupName, userGroupList[j].groupId, userList[i].userId);
                            found = true;
                            break;
                        }
                    }

                    if (false == found)
                    {
                        OsConfigLogInfo(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: group '%s' (%u) of user %u not found in '/etc/group'",
                            userGroupList[j].groupName, userGroupList[j].groupId, userList[i].userId);

                        if (NULL != (command = FormatAllocateString(commandTemplate, userList[i].userId, userGroupList[j].groupId)))
                        {
                            if (0 == (_status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
                            {
                                OsConfigLogInfo(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: user %u was removed from group %u ('%s')",
                                    userList[i].userId, userGroupList[j].groupId, IsSystemGroup(&userGroupList[j]) ? userGroupList[j].groupName : g_redacted);
                            }
                            else
                            {
                                OsConfigLogInfo(log, "SetAllEtcPasswdGroupsToExistInEtcGroup: 'gpasswd -d %u %u' failed with %d (%s)",
                                    userList[i].userId, userGroupList[j].groupId, _status, strerror(_status));
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

int RemoveUser(SimplifiedUser* user, OsConfigLogHandle log)
{
    const char* commandTemplate = "userdel -f %s";
    char* command = NULL;
    int status = 0;

    if (NULL == user)
    {
        OsConfigLogError(log, "RemoveUser: invalid argument");
        return EINVAL;
    }
    else if (0 == user->userId)
    {
        OsConfigLogInfo(log, "RemoveUser: cannot remove user with uid 0 (%u, %u)", user->userId, user->groupId);
        return EPERM;
    }
    else if (user->notInShadow)
    {
        OsConfigLogInfo(log, "RemoveUser: cannot remove an user account that does not exist in the shadow database (%u)", user->userId);
        return EPERM;
    }

    if (NULL != (command = FormatAllocateString(commandTemplate, user->username)))
    {
        if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogInfo(log, "RemoveUser: removed user %u", user->userId);

            if (DirectoryExists(user->home))
            {
                OsConfigLogWarning(log, "RemoveUser: home directory of user %u remains and needs to be manually deleted", user->userId);
            }
            else
            {
                OsConfigLogInfo(log, "RemoveUser: home directory of user %u successfully removed", user->userId);
            }
        }
        else
        {
            OsConfigLogInfo(log, "RemoveUser: cannot remove user %u, userdel failed with %d (%s)", user->userId, status, strerror(status));
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
                        OsConfigLogInfo(log, "CheckNoDuplicateUserNamesExist: user %u appears more than a single time in '/etc/passwd'", userList[i].userId);
                        OsConfigCaptureReason(reason, "User %u appears more than a single time in '/etc/passwd'", userList[i].userId);
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
                        OsConfigLogInfo(log, "CheckNoDuplicateGroupNamesExist: group %u ('%s') appears more than a single time in '/etc/group'",
                            groupList[i].groupId, IsSystemGroup(&groupList[i]) ? groupList[i].groupName : g_redacted);
                        OsConfigCaptureReason(reason, "Group %u ('%s') appears more than a single time in '/etc/group'",
                            groupList[i].groupId, IsSystemGroup(&groupList[i]) ? groupList[i].groupName : g_redacted);
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
            if (0 == strcmp(groupList[i].groupName, g_shadow))
            {
                found = true;
                OsConfigLogInfo(log, "CheckShadowGroupIsEmpty: group 'shadow' (%u) exists", groupList[i].groupId);

                if (true == groupList[i].hasUsers)
                {
                    OsConfigLogInfo(log, "CheckShadowGroupIsEmpty: group 'shadow' (%u) is not empty", groupList[i].groupId);
                    OsConfigCaptureReason(reason, "Group 'shadow' is not empty: %u", groupList[i].groupId);
                    status = ENOENT;
                }

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
                        OsConfigLogInfo(log, "SetShadowGroupEmpty: user %u is a member of group '%s' (%u)",
                            userList[i].userId, g_shadow, userGroupList[j].groupId);

                        if (NULL != (command = FormatAllocateString(commandTemplate, userList[i].username, g_shadow)))
                        {
                            if (0 == (_status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
                            {
                                OsConfigLogInfo(log, "SetShadowGroupEmpty: user %u was removed from group %u ('%s')",
                                    userList[i].userId, userGroupList[j].groupId, IsSystemGroup(&userGroupList[j]) ? userGroupList[j].groupName : g_redacted);
                            }
                            else if ((ESRCH == _status) || (ENOENT== _status))
                            {
                                OsConfigLogInfo(log, "SetShadowGroupEmpty: gpasswd returned %d (%s) which means group '%s' is not found",
                                    _status, strerror(_status), g_shadow);
                                _status = 0;
                            }
                            else
                            {
                                OsConfigLogInfo(log, "SetShadowGroupEmpty: gpasswd failed with %d (%s)", _status, strerror(_status));
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
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user %u ('%s') appears to have a password set",
                    userList[i].userId, IsSystemAccount(&userList[i]) ? userList[i].username : g_redacted);
            }
            else if (userList[i].noLogin)
            {
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user %u ('%s') is no login",
                    userList[i].userId, IsSystemAccount(&userList[i]) ? userList[i].username : g_redacted);
            }
            else if (userList[i].isLocked)
            {
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user %u ('%s') is locked",
                    userList[i].userId, IsSystemAccount(&userList[i]) ? userList[i].username : g_redacted);
            }
            else if (userList[i].cannotLogin)
            {
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user %u ('%s') cannot login with password",
                    userList[i].userId, IsSystemAccount(&userList[i]) ? userList[i].username : g_redacted);
            }
            else if (userList[i].notInShadow)
            {
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user %u ('%s') does not exist in the shadow database",
                    userList[i].userId, IsSystemAccount(&userList[i]) ? userList[i].username : g_redacted);
            }
            else
            {
                OsConfigLogInfo(log, "CheckAllUsersHavePasswordsSet: user %u ('%s')  not found to have a password set",
                    userList[i].userId, IsSystemAccount(&userList[i]) ? userList[i].username : g_redacted);
                OsConfigCaptureReason(reason, "User %u ('%s')  not found to have a password set",
                    userList[i].userId, IsSystemAccount(&userList[i]) ? userList[i].username : g_redacted);
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
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user %u appears to have a password set", userList[i].userId);
            }
            else if (userList[i].noLogin)
            {
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user %u is no login", userList[i].userId);
            }
            else if (userList[i].isLocked)
            {
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user %u is locked", userList[i].userId);
            }
            else if (userList[i].cannotLogin)
            {
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user %u cannot login with password", userList[i].userId);
            }
            else if (userList[i].notInShadow)
            {
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user %u does not exist in the shadow database", userList[i].userId);
            }
            else
            {
                OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: user %u can login and has no password set", userList[i].userId);

                if (0 == userList[i].userId)
                {
                    OsConfigLogInfo(log, "RemoveUsersWithoutPasswords: the root account's password must be manually fixed");
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
                    IsSystemAccount(&userList[i]) ? userList[i].username : g_redacted, userList[i].userId, userList[i].groupId);
                OsConfigCaptureReason(reason, "User '%s' (%u, %u) is not root but has uid 0",
                    IsSystemAccount(&userList[i]) ? userList[i].username : g_redacted, userList[i].userId, userList[i].groupId);
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
                    IsSystemAccount(&userList[i]) ? userList[i].username : g_redacted, userList[i].userId, userList[i].groupId);

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
                OsConfigLogInfo(log, "CheckAllUsersHomeDirectoriesExist: the home directory for user %u is not found or is not a directory", userList[i].userId);
                OsConfigCaptureReason(reason, "The home directory for user %u is not found or is not a directory", userList[i].userId);
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
                    OsConfigLogInfo(log, "SetUserHomeDirectories: user %u home directory is not found", userList[i].userId);

                    if (0 == (_status = mkdir(userList[i].home, defaultHomeDirAccess)))
                    {
                        OsConfigLogInfo(log, "SetUserHomeDirectories: user %u has now the home directory set", userList[i].userId);
                    }
                    else
                    {
                        _status = (0 == errno) ? EACCES : errno;
                        OsConfigLogInfo(log, "SetUserHomeDirectories: cannot create home directory for user %u,  %d (%s)", userList[i].userId, _status, strerror(_status));
                    }
                }

                // If the home directory does not have correct ownership and access, correct this
                if (true == DirectoryExists(userList[i].home))
                {
                    if (0 != (_status = SetDirectoryAccess(userList[i].home, userList[i].userId, userList[i].groupId, defaultHomeDirAccess, log)))
                    {
                        OsConfigLogInfo(log, "SetUserHomeDirectories: cannot set access and ownership for home directory of user %u (%d, errno: %d, %s)",
                            userList[i].userId, _status, errno, strerror(errno));
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
                    OsConfigLogInfo(log, "CheckUsersOwnTheirHomeDirectories: user %u cannot login and their assigned home directory is owned by root", userList[i].userId);
                }
                else if (0 == CheckHomeDirectoryOwnership(&userList[i], log))
                {
                    OsConfigLogInfo(log, "CheckUsersOwnTheirHomeDirectories: user %u owns their assigned home directory", userList[i].userId);
                }
                else
                {
                    OsConfigLogInfo(log, "CheckUsersOwnTheirHomeDirectories: user %u does not own their assigned home directory", userList[i].userId);
                    OsConfigCaptureReason(reason, "User %u does not own their assigned home directory", userList[i].userId);
                    status = ENOENT;
                }
            }
            else
            {
                OsConfigLogInfo(log, "CheckUsersOwnTheirHomeDirectories: user %u assigned home directory does not exist", userList[i].userId);
                OsConfigCaptureReason(reason, "User '%u assigned home directory does not exist", userList[i].userId);
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
                    if (0 == CheckDirectoryAccess(userList[i].home, userList[i].userId, userList[i].groupId, modes[j], NULL, log))
                    {
                        OsConfigLogInfo(log, "CheckRestrictedUserHomeDirectories: user %u has proper restricted access (%03o) for their assigned home directory",
                            userList[i].userId, modes[j]);
                        oneGoodMode = true;
                        break;
                    }
                }

                if (false == oneGoodMode)
                {
                    OsConfigLogInfo(log, "CheckRestrictedUserHomeDirectories: user %u does not have proper restricted access for their assigned home directory",
                        userList[i].userId);
                    OsConfigCaptureReason(reason, "User %u does not have proper restricted access for their assigned home directory", userList[i].userId);

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
                    if (0 == CheckDirectoryAccess(userList[i].home, userList[i].userId, userList[i].groupId, modes[j], NULL, log))
                    {
                        OsConfigLogInfo(log, "SetRestrictedUserHomeDirectories: user %u already has proper restricted access (%03o) for their assigned home directory",
                            userList[i].userId, modes[j]);
                        oneGoodMode = true;
                        break;
                    }
                }

                if (false == oneGoodMode)
                {
                    if (0 == (_status = SetDirectoryAccess(userList[i].home, userList[i].userId, userList[i].groupId, userList[i].isRoot ? modeForRoot : modeForOthers, log)))
                    {
                        OsConfigLogInfo(log, "SetRestrictedUserHomeDirectories: user %u has now proper restricted access (%03o) for their assigned home directory",
                            userList[i].userId, userList[i].isRoot ? modeForRoot : modeForOthers);
                    }
                    else
                    {
                        OsConfigLogInfo(log, "SetRestrictedUserHomeDirectories: cannot set restricted access (%03o) for user %u assigned home directory (%d, %s)",
                            userList[i].userId, userList[i].isRoot ? modeForRoot : modeForOthers, _status, strerror(_status));

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
                    OsConfigLogInfo(log, "CheckMinDaysBetweenPasswordChanges: user %u has a minimum time between password changes of %ld days (requested: %ld)",
                        userList[i].userId, userList[i].minimumPasswordAge, days);
                    OsConfigCaptureSuccessReason(reason, "User %u has a minimum time between password changes of %ld days (requested: %ld)",
                        userList[i].userId, userList[i].minimumPasswordAge, days);
                }
                else
                {
                    OsConfigLogInfo(log, "CheckMinDaysBetweenPasswordChanges: user %u minimum time between password changes of %ld days is less than requested %ld days",
                        userList[i].userId, userList[i].minimumPasswordAge, days);
                    OsConfigCaptureReason(reason, "User %u minimum time between password changes of %ld days is less than requested %ld days",
                        userList[i].userId, userList[i].minimumPasswordAge, days);
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
                OsConfigLogInfo(log, "SetMinDaysBetweenPasswordChanges: user %u minimum time between password changes of %ld days is less than requested %ld days",
                    userList[i].userId, userList[i].minimumPasswordAge, days);

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
                        OsConfigLogInfo(log, "SetMinDaysBetweenPasswordChanges: user %u minimum time between password changes is now set to %ld days", userList[i].userId, days);
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
                    OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: user %u has unlimited time between password changes of %ld days (requested: %ld)",
                        userList[i].userId, userList[i].maximumPasswordAge, days);
                    OsConfigCaptureReason(reason, "User %u has unlimited time between password changes of %ld days(requested: %ld)",
                        userList[i].userId, userList[i].maximumPasswordAge, days);
                    status = ENOENT;
                }
                else if (userList[i].maximumPasswordAge <= days)
                {
                    OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: user %u has a maximum time between password changes of %ld days (requested: %ld)",
                        userList[i].userId, userList[i].maximumPasswordAge, days);
                    OsConfigCaptureSuccessReason(reason, "User %u has a maximum time between password changes of %ld days(requested: %ld)",
                        userList[i].userId, userList[i].maximumPasswordAge, days);
                }
                else
                {
                    OsConfigLogInfo(log, "CheckMaxDaysBetweenPasswordChanges: user %u maximum time between password changes of %ld days is more than requested %ld days",
                        userList[i].userId, userList[i].maximumPasswordAge, days);
                    OsConfigCaptureReason(reason, "User %u maximum time between password changes of %ld days is more than requested %ld days",
                        userList[i].userId, userList[i].maximumPasswordAge, days);
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
                OsConfigLogInfo(log, "SetMaxDaysBetweenPasswordChanges: user %u has maximum time between password changes of %ld days while requested is %ld days",
                    userList[i].userId, userList[i].maximumPasswordAge, days);

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
                        OsConfigLogInfo(log, "SetMaxDaysBetweenPasswordChanges: user %u maximum time between password changes is now set to %ld days", userList[i].userId, days);
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
                OsConfigLogInfo(log, "EnsureUsersHaveDatesOfLastPasswordChanges: password for user %u was never changed (%lu)", userList[i].userId, userList[i].lastPasswordChange);

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
                        OsConfigLogInfo(log, "EnsureUsersHaveDatesOfLastPasswordChanges: user %u date of last password change is now set to %ld days since epoch (today)",
                            userList[i].userId, currentDate);
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
                    OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: password for user %u has no expiration date (%ld)", userList[i].userId, userList[i].maximumPasswordAge);
                    OsConfigCaptureReason(reason, "Password for user %u has no expiration date (%ld)", userList[i].userId, userList[i].maximumPasswordAge);
                    status = ENOENT;
                }
                else if (userList[i].lastPasswordChange < 0)
                {
                    OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: password for user %u has no recorded change date (%ld)", userList[i].userId, userList[i].lastPasswordChange);
                    OsConfigCaptureReason(reason, "Password for user %u has no recorded last change date (%ld)", userList[i].userId, userList[i].lastPasswordChange);
                    status = ENOENT;
                }
                else
                {
                    passwordExpirationDate = userList[i].lastPasswordChange + userList[i].maximumPasswordAge;

                    if (passwordExpirationDate >= currentDate)
                    {
                        if ((passwordExpirationDate - currentDate) <= days)
                        {
                            OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: password for user %u will expire in %ld days (requested maximum: %ld)",
                                userList[i].userId, passwordExpirationDate - currentDate, days);
                            OsConfigCaptureSuccessReason(reason, "Password for user %u will expire in %ld days (requested maximum: %ld)",
                                userList[i].userId, passwordExpirationDate - currentDate, days);
                        }
                        else
                        {
                            OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: password for user %u will expire in %ld days, more than requested maximum of %ld days",
                                userList[i].userId, passwordExpirationDate - currentDate, days);
                            OsConfigCaptureReason(reason, "Password for user %u will expire in %ld days, more than requested maximum of %ld days",
                                userList[i].userId, passwordExpirationDate - currentDate, days);
                            status = ENOENT;
                        }
                    }
                    else
                    {
                        OsConfigLogInfo(log, "CheckPasswordExpirationLessThan: password for user %u expired %ld days ago (current date: %ld, expiration date: %ld days since the epoch)",
                            userList[i].userId, currentDate - passwordExpirationDate, currentDate, passwordExpirationDate);
                        OsConfigCaptureSuccessReason(reason, "Password for user '%u)  expired %ld days ago (current date: %ld, expiration date: %ld days since the epoch)",
                            userList[i].userId, currentDate - passwordExpirationDate, currentDate, passwordExpirationDate);
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
                    OsConfigLogInfo(log, "CheckPasswordExpirationWarning: user %u has a password expiration warning time of %ld days (requested: %ld)",
                        userList[i].userId, userList[i].warningPeriod, days);
                    OsConfigCaptureSuccessReason(reason, "User %u has a password expiration warning time of %ld days (requested: %ld)",
                        userList[i].userId, userList[i].warningPeriod, days);
                }
                else
                {
                    OsConfigLogInfo(log, "CheckPasswordExpirationWarning: user %u password expiration warning time is %ld days, less than requested %ld days",
                        userList[i].userId, userList[i].warningPeriod, days);
                    OsConfigCaptureReason(reason, "User %u password expiration warning time is %ld days, less than requested %ld days",
                        userList[i].userId, userList[i].warningPeriod, days);
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
                OsConfigLogInfo(log, "SetPasswordExpirationWarning: user %u password expiration warning time is %ld days, less than requested %ld days",
                    userList[i].userId, userList[i].warningPeriod, days);

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
                        OsConfigLogInfo(log, "SetPasswordExpirationWarning: user %u password expiration warning time is now set to %ld days", userList[i].userId, days);
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
                    OsConfigLogInfo(log, "CheckUsersRecordedPasswordChangeDates: password for user %u has no recorded change date (%ld)",
                        userList[i].userId, userList[i].lastPasswordChange);
                    OsConfigCaptureSuccessReason(reason, "User %u has no recorded last password change date (%ld)",
                        userList[i].userId, userList[i].lastPasswordChange);
                }
                else if (userList[i].lastPasswordChange <= daysCurrent)
                {
                    OsConfigLogInfo(log, "CheckUsersRecordedPasswordChangeDates: user %u has %lu days since last password change",
                        userList[i].userId, daysCurrent - userList[i].lastPasswordChange);
                    OsConfigCaptureSuccessReason(reason, "User %u has %lu days since last password change",
                        userList[i].userId, daysCurrent - userList[i].lastPasswordChange);
                }
                else
                {
                    OsConfigLogInfo(log, "CheckUsersRecordedPasswordChangeDates: user %u last recorded password change is in the future (next %ld days)",
                        userList[i].userId, userList[i].lastPasswordChange - daysCurrent);
                    OsConfigCaptureReason(reason, "User %u last recorded password change is in the future (next %ld days)",
                        userList[i].userId, userList[i].lastPasswordChange - daysCurrent);
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
                OsConfigLogInfo(log, "CheckLockoutAfterInactivityLessThan: user %u period of inactivity before lockout is %ld days, more than requested %ld days",
                    userList[i].userId, userList[i].inactivityPeriod, days);
                OsConfigCaptureReason(reason, "User %u password period of inactivity before lockout is %ld days, more than requested %ld days",
                    userList[i].userId, userList[i].inactivityPeriod, days);
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
                OsConfigLogInfo(log, "SetLockoutAfterInactivityLessThan: user %u is locked out after %ld days of inactivity while requested is %ld days",
                    userList[i].userId, userList[i].inactivityPeriod, days);

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
                        OsConfigLogInfo(log, "SetLockoutAfterInactivityLessThan: user %u lockout time after inactivity is now set to %ld days", userList[i].userId, days);
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
                OsConfigLogInfo(log, "CheckSystemAccountsAreNonLogin: user %u is either locked, no-login, or cannot-login, but can login with password ('%s')", userList[i].userId, userList[i].shell);
                OsConfigCaptureReason(reason, "User %u is either locked, no-login, or cannot-login, but can login with password", userList[i].userId, userList[i].shell);
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
                OsConfigLogInfo(log, "SetSystemAccountsNonLogin: user %u is either locked, non-login, or cannot-login, but can login with password ('%s')",  userList[i].userId, userList[i].shell);

                // If the account is not already true non-login, try to make it non-login and if that does not work, remove the account
                if (0 != (_status = SetUserNonLogin(&(userList[i]), log)))
                {
                    _status = RemoveUser(&(userList[i]), log);
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
                    OsConfigLogInfo(log, "CheckRootPasswordForSingleUserMode: user %u appears to have a password", userList[i].userId);
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
                            OsConfigLogInfo(log, "CheckOrEnsureUsersDontHaveDotFiles: for user %u, '%s' needs to be manually removed", userList[i].userId, dotPath);
                            status = ENOENT;
                        }
                    }
                    else
                    {
                        OsConfigLogInfo(log, "CheckOrEnsureUsersDontHaveDotFiles: user %u has file '.%s' ('%s')", userList[i].userId, name, dotPath);
                        OsConfigCaptureReason(reason, "User %u has file '.%s' ('%s')", userList[i].userId, name, dotPath);
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
                                OsConfigLogInfo(log, "CheckUsersRestrictedDotFiles: user %u has proper restricted access (%03o) for their dot file '%s'",
                                    userList[i].userId, modes[j], entry->d_name);
                                oneGoodMode = true;
                                break;
                            }
                        }

                        if (false == oneGoodMode)
                        {
                            OsConfigLogInfo(log, "CheckUsersRestrictedDotFiles: user %u does not has have proper restricted access for their dot file '%s'",
                                userList[i].userId, entry->d_name);
                            OsConfigCaptureReason(reason, "User %u does not has have proper restricted access for their dot file '%s'",
                                userList[i].userId, entry->d_name);

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
                                OsConfigLogInfo(log, "SetUsersRestrictedDotFiles: user %u already has proper restricted access (%03o) set for their dot file '%s'",
                                    userList[i].userId, modes[j], path);
                                oneGoodMode = true;
                                break;
                            }
                        }

                        if (false == oneGoodMode)
                        {
                            if (0 == (_status = SetFileAccess(path, userList[i].userId, userList[i].groupId, mode, log)))
                            {
                                OsConfigLogInfo(log, "SetUsersRestrictedDotFiles: user %u now has restricted access (%03o) set for their dot file '%s'",
                                    userList[i].userId, mode, path);
                            }
                            else
                            {
                                OsConfigLogInfo(log, "SetUsersRestrictedDotFiles: cannot set restricted access (%u) for user %u dot file '%s'",
                                    mode, userList[i].userId, path);

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
    struct passwd* userEntry = NULL;
    unsigned int i = 0, j = 0;
    unsigned int numberOfPasswdLines = 0;
    unsigned int namesLength = 0;
    char* name = NULL;
    bool found = false;
    int status = 0;

    if (NULL == names)
    {
        OsConfigLogError(log, "CheckUserAccountsNotFound: invalid argument");
        return EINVAL;
    }

    namesLength = strlen(names);

    if (0 != (numberOfPasswdLines = GetNumberOfLinesInFile(g_passwdFile)))
    {
        setpwent();

        while ((NULL != (userEntry = getpwent())) && (i < numberOfPasswdLines))
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

                    if (0 == strcmp(userEntry->pw_name, name))
                    {
                        OsConfigLogInfo(log, "CheckUserAccountsNotFound: user %u is present", userEntry->pw_uid);
                        OsConfigCaptureReason(reason, "User %u is present", userEntry->pw_uid);
                        found = true;
                    }
                }

                j += strlen(name);
                FREE_MEMORY(name);
            }

            i += 1;
        }

        endpwent();
    }
    else
    {
        OsConfigLogInfo(log, "CheckUserAccountsNotFound: cannot read from '%s'", g_passwdFile);
        status = EPERM;
    }

    if (found && (0 == status))
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
    SimplifiedUser simplifiedUser = {0};
    size_t namesLength = 0;
    char* name = NULL;
    struct passwd* userEntry = NULL;
    unsigned int numberOfPasswdLines = 0;
    unsigned int i = 0, j = 0;
    int status = 0, _status = 0;

    if (NULL == names)
    {
        OsConfigLogError(log, "RemoveUserAccounts: invalid argument");
        return EINVAL;
    }
    else if (0 == (numberOfPasswdLines = GetNumberOfLinesInFile(g_passwdFile)))
    {
        OsConfigLogError(log, "RemoveUserAccounts: cannot read from '%s'", g_passwdFile);
        return EPERM;
    }
    else if (0 == CheckUserAccountsNotFound(names, NULL, log))
    {
        OsConfigLogInfo(log, "RemoveUserAccounts: the requested user accounts '%s' appear already removed", names);
        return 0;
    }

    status = 0;

    namesLength = strlen(names);

    setpwent();

    while ((NULL != (userEntry = getpwent())) && (i < numberOfPasswdLines) && (0 == status))
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

                if (0 == strcmp(userEntry->pw_name, name))
                {
                    if (0 != (status = CopyUserEntry(&simplifiedUser, userEntry, log)))
                    {
                        OsConfigLogError(log, "RemoveUserAccounts: failed making copy of user entry (%d)", status);
                        break;
                    }
                    else if ((0 != (_status = RemoveUser(&simplifiedUser, log))) && (0 == status))
                    {
                        status = _status;
                    }

                    ResetUserEntry(&simplifiedUser);
                }
            }

            j += strlen(name);
            FREE_MEMORY(name);
        }

        i += 1;
    }

    endpwent();

    if (0 == status)
    {
        OsConfigLogInfo(log, "RemoveUserAccounts: the requested user accounts ('%s') were removed from this system", names);
    }

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

bool GroupExists(gid_t groupId, OsConfigLogHandle log)
{
    bool result = false;

    errno = 0;

    if (NULL != getgrgid(groupId))
    {
        OsConfigLogInfo(log, "GroupExists: group %u exists", (unsigned int)groupId);
        result = true;
    }
    else if (0 == errno)
    {
        OsConfigLogInfo(log, "GroupExists: group %u does not exist (errno: %d)", (unsigned int)groupId, errno);
    }
    else
    {
        OsConfigLogInfo(log, "GroupExists: getgrgid(for gid: %u) failed (errno: %d, %s)", (unsigned int)groupId, errno, strerror(errno));
    }

    return result;
}

int CheckGroupExists(const char* name, char** reason, OsConfigLogHandle log)
{
    struct group* groupEntry = NULL;
    int result = ENOENT;

    errno = 0;

    if ((NULL != name) && (NULL != (groupEntry = getgrnam(name))))
    {
        OsConfigLogInfo(log, "CheckGroupExists: group %u exists", (unsigned int)groupEntry->gr_gid);
        OsConfigCaptureSuccessReason(reason, "Group %u exists", (unsigned int)groupEntry->gr_gid);
        result = 0;
    }

    if (0 != result)
    {
        OsConfigLogInfo(log, "CheckGroupExists: group '%s' does not exist (errno: %d)", name, errno);
        OsConfigCaptureReason(reason, "Group '%s' does not exist (%d)", name, errno);
    }

    return result;
}

int CheckUserExists(const char* username, char** reason, OsConfigLogHandle log)
{
    struct passwd* userEntry = NULL;
    int result = ENOENT;

    if (NULL != username)
    {
        setpwent();

        while (NULL != (userEntry = getpwent()))
        {
            if (NULL == userEntry->pw_name)
            {
                continue;
            }
            else if (0 == strcmp(userEntry->pw_name, username))
            {
                OsConfigLogInfo(log, "UserExists: user %u exists", userEntry->pw_uid);
                OsConfigCaptureSuccessReason(reason, "User %u exists", userEntry->pw_uid);
                result = 0;
                break;
            }
        }

        endpwent();
    }

    if (0 != result)
    {
        OsConfigLogInfo(log, "UserExists: user '%s' does not exist", username);
        OsConfigCaptureReason(reason, "User '%s' does not exist", username);
    }

    return result;
}

int AddIfMissingSyslogSystemUser(OsConfigLogHandle log)
{
    const char* command = "useradd -r -s /usr/sbin/nologin -d /nonexistent syslog";
    int result = 0;

    if (0 != (result = CheckUserExists("syslog", NULL, log)))
    {
        if (0 != (result = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogInfo(log, "AddMissingSyslogSystemUser: useradd for user 'syslog' failed with %d (errno: %d, %s)", result, errno, strerror(errno));
        }
        else
        {
            OsConfigLogInfo(log, "AddMissingSyslogSystemUser: user 'syslog' successfully created");
        }
    }

    return result;
}

int AddIfMissingAdmSystemGroup(OsConfigLogHandle log)
{
    const char* command = "groupadd -r adm";
    int result = 0;

    if (0 != (result = CheckGroupExists("adm", NULL, log)))
    {
        if (0 != (result = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogInfo(log, "AddMissingAdmSystemGroup: groupadd for group 'adm' failed with %d (errno: %d, %s)", result, errno, strerror(errno));
        }
        else
        {
            OsConfigLogInfo(log, "AddMissingAdmSystemGroup: group 'adm' successfully created");
        }
    }

    return result;
}
