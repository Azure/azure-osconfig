// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef USERUTILS_H
#define USERUTILS_H

// Include CommonUtils.h in the target source before including this header
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

enum PasswordEncryption
{
    unknown = 0,
    md5 = 1,
    blowfish = 2,
    eksBlowfish = 3,
    unknownBlowfish = 4,
    sha256 = 5,
    sha512 = 6
};
typedef enum PasswordEncryption PasswordEncryption;

typedef struct SIMPLIFIED_USER
{
    char* username;
    uid_t userId;
    gid_t groupId;
    char* home;
    char* shell;

    bool isRoot;
    bool isLocked;
    bool noLogin;
    bool cannotLogin;
    bool hasPassword;

    // Encryption algorithm (cypher) used for password
    PasswordEncryption passwordEncryption;

    // Date of last change (measured in days since 1970-01-01 00:00:00 +0000 UTC)
    long lastPasswordChange;

    // Minimum number of days between password changes
    long minimumPasswordAge;

    // Maximum number of days between password changes
    long maximumPasswordAge;

    // Number of days before password expires to warn user to change it
    long warningPeriod;

    // Number of days after password expires until account is disabled
    long inactivityPeriod;

    // Date when user account expires (measured in days since 1970-01-01 00:00:00 +0000 UTC)
    long expirationDate;
} SIMPLIFIED_USER;

typedef struct SIMPLIFIED_GROUP
{
    char* groupName;
    gid_t groupId;
    bool hasUsers;
} SIMPLIFIED_GROUP;

#ifdef __cplusplus
extern "C"
{
#endif

int EnumerateUsers(SIMPLIFIED_USER** userList, unsigned int* size, char** reason, OSCONFIG_LOG_HANDLE log);
void FreeUsersList(SIMPLIFIED_USER** source, unsigned int size);

int EnumerateUserGroups(SIMPLIFIED_USER* user, SIMPLIFIED_GROUP** groupList, unsigned int* size, char** reason, OSCONFIG_LOG_HANDLE log);
int EnumerateAllGroups(SIMPLIFIED_GROUP** groupList, unsigned int* size, char** reason, OSCONFIG_LOG_HANDLE log);
void FreeGroupList(SIMPLIFIED_GROUP** groupList, unsigned int size);

int CheckAllEtcPasswdGroupsExistInEtcGroup(char** reason, OSCONFIG_LOG_HANDLE log);
int SetAllEtcPasswdGroupsToExistInEtcGroup(OSCONFIG_LOG_HANDLE log);
int CheckNoDuplicateUidsExist(char** reason, OSCONFIG_LOG_HANDLE log);
int CheckNoDuplicateGidsExist(char** reason, OSCONFIG_LOG_HANDLE log);
int CheckNoDuplicateUserNamesExist(char** reason, OSCONFIG_LOG_HANDLE log);
int CheckNoDuplicateGroupNamesExist(char** reason, OSCONFIG_LOG_HANDLE log);
int CheckShadowGroupIsEmpty(char** reason, OSCONFIG_LOG_HANDLE log);
int SetShadowGroupEmpty(OSCONFIG_LOG_HANDLE log);
int CheckRootGroupExists(char** reason, OSCONFIG_LOG_HANDLE log);
int RepairRootGroup(OSCONFIG_LOG_HANDLE log);
int CheckRootIsOnlyUidZeroAccount(char** reason, OSCONFIG_LOG_HANDLE log);
int SetRootIsOnlyUidZeroAccount(OSCONFIG_LOG_HANDLE log);
int CheckAllUsersHavePasswordsSet(char** reason, OSCONFIG_LOG_HANDLE log);
int RemoveUsersWithoutPasswords(OSCONFIG_LOG_HANDLE log);
int RemoveUser(SIMPLIFIED_USER* user, bool removeHomeDir, OSCONFIG_LOG_HANDLE log);
int RemoveGroup(SIMPLIFIED_GROUP* group, bool removeHomeDirs, OSCONFIG_LOG_HANDLE log);
int CheckDefaultRootAccountGroupIsGidZero(char** reason, OSCONFIG_LOG_HANDLE log);
int SetDefaultRootAccountGroupIsGidZero(OSCONFIG_LOG_HANDLE log);
int CheckAllUsersHomeDirectoriesExist(char** reason, OSCONFIG_LOG_HANDLE log);
int CheckUsersOwnTheirHomeDirectories(char** reason, OSCONFIG_LOG_HANDLE log);
int SetUserHomeDirectories(OSCONFIG_LOG_HANDLE log);
int CheckRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, char** reason, OSCONFIG_LOG_HANDLE log);
int SetRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, unsigned int modeForRoot, unsigned int modeForOthers, OSCONFIG_LOG_HANDLE log);
int CheckPasswordHashingAlgorithm(unsigned int algorithm, char** reason, OSCONFIG_LOG_HANDLE log);
int SetPasswordHashingAlgorithm(unsigned int algorithm, OSCONFIG_LOG_HANDLE log);
int CheckMinDaysBetweenPasswordChanges(long days, char** reason, OSCONFIG_LOG_HANDLE log);
int SetMinDaysBetweenPasswordChanges(long days, OSCONFIG_LOG_HANDLE log);
int CheckMaxDaysBetweenPasswordChanges(long days, char** reason, OSCONFIG_LOG_HANDLE log);
int SetMaxDaysBetweenPasswordChanges(long days, OSCONFIG_LOG_HANDLE log);
int EnsureUsersHaveDatesOfLastPasswordChanges(OSCONFIG_LOG_HANDLE log);
int CheckPasswordExpirationLessThan(long days, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckPasswordExpirationWarning(long days, char** reason, OSCONFIG_LOG_HANDLE log);
int SetPasswordExpirationWarning(long days, OSCONFIG_LOG_HANDLE log);
int CheckUsersRecordedPasswordChangeDates(char** reason, OSCONFIG_LOG_HANDLE log);
int CheckLockoutAfterInactivityLessThan(long days, char** reason, OSCONFIG_LOG_HANDLE log);
int SetLockoutAfterInactivityLessThan(long days, OSCONFIG_LOG_HANDLE log);
int CheckSystemAccountsAreNonLogin(char** reason, OSCONFIG_LOG_HANDLE log);
int SetSystemAccountsNonLogin(OSCONFIG_LOG_HANDLE log);
int CheckRootPasswordForSingleUserMode(char** reason, OSCONFIG_LOG_HANDLE log);
int CheckOrEnsureUsersDontHaveDotFiles(const char* name, bool removeDotFiles, char** reason, OSCONFIG_LOG_HANDLE log);
int CheckUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, char** reason, OSCONFIG_LOG_HANDLE log);
int SetUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, unsigned int mode, OSCONFIG_LOG_HANDLE log);
int CheckUserAccountsNotFound(const char* names, char** reason, OSCONFIG_LOG_HANDLE log);
int RemoveUserAccounts(const char* names, bool removeHomeDirs, OSCONFIG_LOG_HANDLE log);
int RestrictSuToRootGroup(OSCONFIG_LOG_HANDLE log);

#ifdef __cplusplus
}
#endif

#endif // USERUTILS_H
