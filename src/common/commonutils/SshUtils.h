// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef SSHUTILS_H
#define SSHUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

int InitializeSshAudit(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int InitializeSshAuditCheck(const char* name, char* value, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int ProcessSshAuditCheck(const char* name, char* value, char** reason, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
void SshAuditCleanup(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

#ifdef __cplusplus
}
#endif

#endif // SSHUTILS_H
