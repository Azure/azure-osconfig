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

int EnumerateUsers(SimplifiedUser** userList, unsigned int* size, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
void FreeUsersList(SimplifiedUser** source, unsigned int size);

int EnumerateUserGroups(SimplifiedUser* user, SimplifiedGroup** groupList, unsigned int* size, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int EnumerateAllGroups(SimplifiedGroup** groupList, unsigned int* size, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
void FreeGroupList(SimplifiedGroup** groupList, unsigned int size);

int CheckAllEtcPasswdGroupsExistInEtcGroup(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetAllEtcPasswdGroupsToExistInEtcGroup(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckNoDuplicateUidsExist(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckNoDuplicateGidsExist(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckNoDuplicateUserNamesExist(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckNoDuplicateGroupNamesExist(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckShadowGroupIsEmpty(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetShadowGroupEmpty(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckRootGroupExists(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int RepairRootGroup(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckRootIsOnlyUidZeroAccount(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetRootIsOnlyUidZeroAccount(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckAllUsersHavePasswordsSet(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int RemoveUsersWithoutPasswords(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int RemoveUser(SimplifiedUser* user, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckDefaultRootAccountGroupIsGidZero(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetDefaultRootAccountGroupIsGidZero(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckAllUsersHomeDirectoriesExist(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckUsersOwnTheirHomeDirectories(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetUserHomeDirectories(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetRestrictedUserHomeDirectories(unsigned int* modes, unsigned int numberOfModes, unsigned int modeForRoot, unsigned int modeForOthers, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckPasswordHashingAlgorithm(unsigned int algorithm, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetPasswordHashingAlgorithm(unsigned int algorithm, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckMinDaysBetweenPasswordChanges(long days, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetMinDaysBetweenPasswordChanges(long days, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckMaxDaysBetweenPasswordChanges(long days, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetMaxDaysBetweenPasswordChanges(long days, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int EnsureUsersHaveDatesOfLastPasswordChanges(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckPasswordExpirationLessThan(long days, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckPasswordExpirationWarning(long days, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetPasswordExpirationWarning(long days, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckUsersRecordedPasswordChangeDates(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckLockoutAfterInactivityLessThan(long days, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetLockoutAfterInactivityLessThan(long days, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckSystemAccountsAreNonLogin(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetSystemAccountsNonLogin(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckRootPasswordForSingleUserMode(char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckOrEnsureUsersDontHaveDotFiles(const char* name, bool removeDotFiles, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int SetUsersRestrictedDotFiles(unsigned int* modes, unsigned int numberOfModes, unsigned int mode, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int CheckUserAccountsNotFound(const char* names, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int RemoveUserAccounts(const char* names, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int RestrictSuToRootGroup(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

#ifdef __cplusplus
}
#endif

#endif // USERUTILS_H
