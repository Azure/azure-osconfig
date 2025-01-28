// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef SSHUTILS_H
#define SSHUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

int InitializeSshAudit(void* log);
int InitializeSshAuditCheck(const char* name, char* value, void* log);
int ProcessSshAuditCheck(const char* name, char* value, char** reason, void* log);
void SshAuditCleanup(void* log);

#ifdef __cplusplus
}
#endif

#endif // SSHUTILS_H
