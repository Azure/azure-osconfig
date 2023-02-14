// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef ANSIBLEUTILS_H
#define ANSIBLEUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

int AnsibleCheckDependencies(void* log);
int AnsibleCheckCollection(const char* collectionName, void* log);
int AnsibleExecuteModule(const char* collectionName, const char* moduleName, const char* moduleArguments, char** result, void* log);

#ifdef __cplusplus
}
#endif

#endif // ANSIBLEUTILS_H
