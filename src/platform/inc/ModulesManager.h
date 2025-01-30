// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MODULESMANAGER_H
#define MODULESMANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif

void AreModulesLoadedAndLoadIfNot(const char* path, const char* configJson);
void UnloadModules(void);

#ifdef __cplusplus
}
#endif

#endif // MODULESMANAGER_H
