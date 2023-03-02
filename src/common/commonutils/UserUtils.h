// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef USERUTILS_H
#define USERUTILS_H

// Include CommonUtils.h in the target source before including this header
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

enum PasswordEncryptionType
{
    unknown = 0,
    md5 = 1,
    blowfish = 2,
    eksBlowfish = 3,
    unknownBlowfish = 4,
    sha256 = 5,
    sha512 = 6
};
typedef enum PasswordEncryptionType PasswordEncryptionType;

typedef struct SIMPLIFIED_USER
{
    char* username;
    uid_t userId;
    gid_t groupId;
    char* home;
    char* shell;
    
    bool isLocked;
    bool noLogin;
    bool cannotLogin;
    bool hasPassword;

    // Type of encryption used for password
    PasswordEncryptionType passwordEncryptionType;
    
    // Date of last change (measured in days since 1970-01-01 00:00:00 +0000 (UTC)) 
    long passwordLastChange;
    
    // Min number of days between password changes 
    long passwordMinDaysBetweenChanges;
    
    // Max number of days between password changes
    long passwordMaxDaysBetweenChanges;
    
    // Number of days before password expires to warn user to change it 
    long passwordWarnDaysBeforeExpiry;   
    
    // Number of days after password expires until account is disabled 
    long passwordDaysAfterExpiry;        
    
    // Date when user account expires (measured in days since 1970-01-01 00:00:00 +0000 (UTC)) 
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
int CheckAllUsersHavePasswordsSet(void* log);
int CheckNonRootAccountsHaveUniqueUidsGreaterThanZero(void* log);
int CheckNoLegacyPlusEntriesInEtcPasswd(void* log);
int CheckNoLegacyPlusEntriesInEtcShadow(void* log);
int CheckNoLegacyPlusEntriesInEtcGroup(void* log);
int CheckDefaultRootAccountGroupIsGidZero(void* log);
int CheckRootIsOnlyUidZeroAccount(void* log);
int CheckAllUsersHomeDirectoriesExist(void* log);
int CheckUsersOwnTheirHomeDirectories(void* log);

#ifdef __cplusplus
}
#endif

#endif // USERUTILS_H