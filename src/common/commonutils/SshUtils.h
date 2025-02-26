// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef SSHUTILS_H
#define SSHUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

int InitializeSshAudit(OSCONFIG_LOG_HANDLE log);
int InitializeSshAuditCheck(const char* name, char* value, OSCONFIG_LOG_HANDLE log);
int ProcessSshAuditCheck(const char* name, char* value, char** reason, OSCONFIG_LOG_HANDLE log);
void SshAuditCleanup(OSCONFIG_LOG_HANDLE log);

#ifdef __cplusplus
}
#endif

#endif // SSHUTILS_H
