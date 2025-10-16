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

typedef struct SimplifiedUser
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
    bool notInShadow;

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
} SimplifiedUser;

typedef struct SimplifiedGroup
{
    char* groupName;
    gid_t groupId;
    bool hasUsers;
} SimplifiedGroup;

#ifdef __cplusplus
extern "C"
{
#endif

int EnumerateUsers(SimplifiedUser** userList, unsigned int* size, char** reason, OsConfigLogHandle log);
void FreeUsersList(SimplifiedUser** source, unsigned int size);

int EnumerateUserGroups(SimplifiedUser* user, SimplifiedGroup** groupList, unsigned int* size, char** reason, OsConfigLogHandle log);
int EnumerateAllGroups(SimplifiedGroup** groupList, unsigned int* size, char** reason, OsConfigLogHandle log);
void FreeGroupList(SimplifiedGroup** groupList, unsigned int size);

int CheckAllEtcPasswdGroupsExistInEtcGroup(char** reason, OsConfigLogHandle log);
int SetAllEtcPasswdGroupsToExistInEtcGroup(OsConfigLogHandle log);
int CheckNoDuplicateUidsExist(char** reason, OsConfigLogHandle log);
int CheckNoDuplicateGidsExist(char** reason, OsConfigLogHandle log);
int CheckNoDuplicateUserNamesExist(char** reason, OsConfigLogHandle log);
int CheckNoDuplicateGroupNamesExist(char** reason, OsConfigLogHandle log);
int CheckShadowGroupIsEmpty(char** reason, OsConfigLogHandle log);
int SetShadowGroupEmpty(OsConfigLogHandle log);
int CheckRootGroupExists(char** reason, OsConfigLogHandle log);
int RepairRootGroup(OsConfigLogHandle log);
int CheckRootIsOnlyUidZeroAccount(char** reason, OsConfigLogHandle log);
int SetRootIsOnlyUidZeroAccount(OsConfigLogHandle log);
int CheckAllUsersHavePasswordsSet(char** reason, OsConfigLogHandle log);
int RemoveUsersWithoutPasswords(OsConfigLogHandle log);
int RemoveUser(SimplifiedUser* user, OsConfigLogHandle log);
int CheckDefaultRootAccountGroupIsGidZero(char** reason, OsConfigLogHandle log);
int SetDefaultRootAccountGroupIsGidZero(OsConfigLogHandle log);
int CheckAllUsersHomeDirectoriesExist(char** reason, OsConfigLogHandle log);
int CheckUsersOwnTheirHomeDirectories(char** reason, OsConfigLogHandle log);
int SetUserHomeDirectories(OsConfigLogHandle log);
int CheckRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, char** reason, OsConfigLogHandle log);
int SetRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, unsigned int modeForRoot, unsigned int modeForOthers, OsConfigLogHandle log);
int CheckPasswordHashingAlgorithm(unsigned int algorithm, char** reason, OsConfigLogHandle log);
int SetPasswordHashingAlgorithm(unsigned int algorithm, OsConfigLogHandle log);
int CheckMinDaysBetweenPasswordChanges(long days, char** reason, OsConfigLogHandle log);
int SetMinDaysBetweenPasswordChanges(long days, OsConfigLogHandle log);
int CheckMaxDaysBetweenPasswordChanges(long days, char** reason, OsConfigLogHandle log);
int SetMaxDaysBetweenPasswordChanges(long days, OsConfigLogHandle log);
int EnsureUsersHaveDatesOfLastPasswordChanges(OsConfigLogHandle log);
int CheckPasswordExpirationLessThan(long days, char** reason, OsConfigLogHandle log);
int CheckPasswordExpirationWarning(long days, char** reason, OsConfigLogHandle log);
int SetPasswordExpirationWarning(long days, OsConfigLogHandle log);
int CheckUsersRecordedPasswordChangeDates(char** reason, OsConfigLogHandle log);
int CheckLockoutAfterInactivityLessThan(long days, char** reason, OsConfigLogHandle log);
int SetLockoutAfterInactivityLessThan(long days, OsConfigLogHandle log);
int CheckSystemAccountsAreNonLogin(char** reason, OsConfigLogHandle log);
int SetSystemAccountsNonLogin(OsConfigLogHandle log);
int CheckRootPasswordForSingleUserMode(char** reason, OsConfigLogHandle log);
int CheckOrEnsureUsersDontHaveDotFiles(const char* name, bool removeDotFiles, char** reason, OsConfigLogHandle log);
int CheckUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, char** reason, OsConfigLogHandle log);
int SetUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, unsigned int mode, OsConfigLogHandle log);
int CheckUserAccountsNotFound(const char* names, char** reason, OsConfigLogHandle log);
int RemoveUserAccounts(const char* names, OsConfigLogHandle log);
int RestrictSuToRootGroup(OsConfigLogHandle log);
bool GroupExists(gid_t groupId, OsConfigLogHandle log);
int CheckGroupExists(const char* name, char** reason, OsConfigLogHandle log);
int CheckUserExists(const char* username, char** reason, OsConfigLogHandle log);
int AddIfMissingSyslogSystemUser(OsConfigLogHandle log);
int AddIfMissingAdmSystemGroup(OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // USERUTILS_H
