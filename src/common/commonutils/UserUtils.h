// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef USERUTILS_H
#define USERUTILS_H

#include <commonutils.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#ifdef __cplusplus
extern "C"
{
#endif

int EnumerateUsers(struct passwd** passwdList, unsigned int* size, void* log);
void FreeUsersList(struct passwd** source, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif // USERUTILS_H