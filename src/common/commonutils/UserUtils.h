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

int EnumerateUsers(SIMPLIFIED_USER** userList, unsigned int* size, char** reason, void* log);
void FreeUsersList(SIMPLIFIED_USER** source, unsigned int size);

int EnumerateUserGroups(SIMPLIFIED_USER* user, SIMPLIFIED_GROUP** groupList, unsigned int* size, char** reason, void* log);
int EnumerateAllGroups(SIMPLIFIED_GROUP** groupList, unsigned int* size, char** reason, void* log);
void FreeGroupList(SIMPLIFIED_GROUP** groupList, unsigned int size);

int CheckAllEtcPasswdGroupsExistInEtcGroup(char** reason, void* log);
int SetAllEtcPasswdGroupsToExistInEtcGroup(void* log);
int CheckNoDuplicateUidsExist(char** reason, void* log);
int SetNoDuplicateUids(void* log);
int CheckNoDuplicateGidsExist(char** reason, void* log);
int SetNoDuplicateGids(void* log);
int CheckNoDuplicateUserNamesExist(char** reason, void* log);
int SetNoDuplicateUserNames(void* log);
int CheckNoDuplicateGroupNamesExist(char** reason, void* log);
int SetNoDuplicateGroupNames(void* log);
int CheckShadowGroupIsEmpty(char** reason, void* log);
int SetShadowGroupEmpty(void* log);
int CheckRootGroupExists(char** reason, void* log);
int RepairRootGroup(void* log);
int CheckRootIsOnlyUidZeroAccount(char** reason, void* log);
int SetRootIsOnlyUidZeroAccount(void* log);
int CheckAllUsersHavePasswordsSet(char** reason, void* log);
int RemoveUsersWithoutPasswords(void* log);
int CheckDefaultRootAccountGroupIsGidZero(char** reason, void* log);
int SetDefaultRootAccountGroupIsGidZero(void* log);
int CheckAllUsersHomeDirectoriesExist(char** reason, void* log);
int CheckUsersOwnTheirHomeDirectories(char** reason, void* log);
int SetUserHomeDirectories(void* log);
int CheckRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, char** reason, void* log);
int SetRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, unsigned int modeForRoot, unsigned int modeForOthers, void* log);
int CheckPasswordHashingAlgorithm(unsigned int algorithm, char** reason, void* log);
int SetPasswordHashingAlgorithm(unsigned int algorithm, void* log);
int CheckMinDaysBetweenPasswordChanges(long days, char** reason, void* log);
int SetMinDaysBetweenPasswordChanges(long days, void* log);
int CheckMaxDaysBetweenPasswordChanges(long days, char** reason, void* log);
int SetMaxDaysBetweenPasswordChanges(long days, void* log);
int EnsureUsersHaveDatesOfLastPasswordChanges(void* log);
int CheckPasswordExpirationLessThan(long days, char** reason, void* log);
int CheckPasswordExpirationWarning(long days, char** reason, void* log);
int SetPasswordExpirationWarning(long days, void* log);
int CheckUsersRecordedPasswordChangeDates(char** reason, void* log);
int CheckLockoutAfterInactivityLessThan(long days, char** reason, void* log);
int SetLockoutAfterInactivityLessThan(long days, void* log);
int CheckSystemAccountsAreNonLogin(char** reason, void* log);
int RemoveSystemAccountsThatCanLogin(void* log);
int CheckRootPasswordForSingleUserMode(char** reason, void* log);
int CheckOrEnsureUsersDontHaveDotFiles(const char* name, bool removeDotFiles, char** reason, void* log);
int CheckUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, char** reason, void* log);
int SetUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, unsigned int mode, void* log);
int CheckUserAccountsNotFound(const char* names, char** reason, void* log);
int RemoveUserAccounts(const char* names, void* log);
int RestrictSuToRootGroup(void* log);

#ifdef __cplusplus
}
#endif

#endif // USERUTILS_H
