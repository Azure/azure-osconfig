// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef USERUTILS_H
#define USERUTILS_H

// Include CommonUtils.h in the target source before including this header
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

typedef struct SIMPLIFIED_USER
{
    char* username;
    uid_t userId;
    gid_t groupId;
    char* home;
    char* shell;
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

int EnumerateUsers(SIMPLIFIED_USER** passwdList, unsigned int* size, void* log);
void FreeUsersList(SIMPLIFIED_USER** source, unsigned int size);

int EnumerateUserGroups(SIMPLIFIED_USER* user, SIMPLIFIED_GROUP** groupList, unsigned int* size, void* log);
int EnumerateAllGroups(SIMPLIFIED_GROUP** groupList, unsigned int* size, void* log);
void FreeGroupList(SIMPLIFIED_GROUP** groupList, unsigned int size);

int CheckUserHasPassword(SIMPLIFIED_USER* user, void* log);

#ifdef __cplusplus
}
#endif

#endif // USERUTILS_H