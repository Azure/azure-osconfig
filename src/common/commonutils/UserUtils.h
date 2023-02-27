// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef USERUTILS_H
#define USERUTILS_H

// Include CommonUtils.h in the target source before including this header
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

typedef struct SIMPLIFIED_GROUP
{
    char *groupName;
    gid_t groupId;
    bool hasUsers;
} SIMPLIFIED_GROUP;

#ifdef __cplusplus
extern "C"
{
#endif

int EnumerateUsers(struct passwd** passwdList, unsigned int* size, void* log);
void FreeUsersList(struct passwd** source, unsigned int size);

int EnumerateUserGroups(struct passwd* user, SIMPLIFIED_GROUP** groupList, unsigned int* size, void* log);
int EnumerateAllGroups(SIMPLIFIED_GROUP** groupList, unsigned int* size, void* log);
void FreeGroupList(SIMPLIFIED_GROUP** groupList, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif // USERUTILS_H