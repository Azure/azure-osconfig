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

int EnumerateUsers(SIMPLIFIED_USER** userList, unsigned int* size, void* log);
void FreeUsersList(SIMPLIFIED_USER** source, unsigned int size);

int EnumerateUserGroups(SIMPLIFIED_USER* user, SIMPLIFIED_GROUP** groupList, unsigned int* size, void* log);
int EnumerateAllGroups(SIMPLIFIED_GROUP** groupList, unsigned int* size, void* log);
void FreeGroupList(SIMPLIFIED_GROUP** groupList, unsigned int size);

int CheckAllEtcPasswdGroupsExistInEtcGroup(void* log);
int CheckNoDuplicateUidsExist(void* log);
int CheckNoDuplicateGidsExist(void* log);
int CheckNoDuplicateUserNamesExist(void* log);
int CheckNoDuplicateGroupsExist(void* log);
int CheckShadowGroupIsEmpty(void* log);
int CheckRootGroupExists(void* log);
int CheckRootIsOnlyUidZeroAccount(void* log);
int CheckAllUsersHavePasswordsSet(void* log);
int CheckDefaultRootAccountGroupIsGidZero(void* log);
int CheckAllUsersHomeDirectoriesExist(void* log);
int CheckUsersOwnTheirHomeDirectories(void* log);
int CheckRestrictedUserHomeDirectories(unsigned int mode, void* log);
int CheckPasswordHashingAlgorithm(unsigned int algorithm, void* log);
int CheckMinDaysBetweenPasswordChanges(long days, void* log);
int CheckMaxDaysBetweenPasswordChanges(long days, void* log);
int CheckPasswordExpirationLessThan(long days, void* log);
int CheckPasswordExpirationWarning(long days, void* log);
int CheckUsersRecordedPasswordChangeDates(void* log);
int CheckSystemAccountsAreNonLogin(void* log);
int CheckRootPasswordForSingleUserMode(void* log);
int CheckUsersDontHaveDotFiles(const char* name, void* log);
int CheckUsersRestrictedDotFiles(unsigned int mode, void* log);

#ifdef __cplusplus
}
#endif

#endif // USERUTILS_H