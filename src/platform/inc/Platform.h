// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PLATFORM_H
#define PLATFORM_H

#include <Mpi.h>

#ifdef __cplusplus
extern "C"
{
#endif

void LoadModules(const char* path);
void UnloadModules(void);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_H