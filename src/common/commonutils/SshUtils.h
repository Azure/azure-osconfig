// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef SSHUTILS_H
#define SSHUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

int InitializeSshAudit(void* log);
void SshAuditCleanup(void* log);

char* SshUtilsAuditEnsurePermissionsOnEtcSshSshdConfig(void* log);
char* SshUtilsAuditEnsureSshBestPracticeProtocol(void* log);
char* SshUtilsAuditEnsureSshBestPracticeIgnoreRhosts(void* log);
char* SshUtilsAuditEnsureSshLogLevelIsSet(void* log);
char* SshUtilsAuditEnsureSshMaxAuthTriesIsSet(void* log);
char* SshUtilsAuditEnsureAllowUsersIsConfigured(void* log);
char* SshUtilsAuditEnsureDenyUsersIsConfigured(void* log);
char* SshUtilsAuditEnsureAllowGroupsIsConfigured(void* log);
char* SshUtilsAuditEnsureDenyGroupsConfigured(void* log);
char* SshUtilsAuditEnsureSshHostbasedAuthenticationIsDisabled(void* log);
char* SshUtilsAuditEnsureSshPermitRootLoginIsDisabled(void* log);
char* SshUtilsAuditEnsureSshPermitEmptyPasswordsIsDisabled(void* log);
char* SshUtilsAuditEnsureSshClientIntervalCountMaxIsConfigured(void* log);
char* SshUtilsAuditEnsureSshClientAliveIntervalIsConfigured(void* log);
char* SshUtilsAuditEnsureSshLoginGraceTimeIsSet(void* log);
char* SshUtilsAuditEnsureOnlyApprovedMacAlgorithmsAreUsed(void* log);
char* SshUtilsAuditEnsureSshWarningBannerIsEnabled(void* log);
char* SshUtilsAuditEnsureUsersCannotSetSshEnvironmentOptions(void* log);
char* SshUtilsAuditEnsureAppropriateCiphersForSsh(void* log);

int SshUtilsRemediateEnsurePermissionsOnEtcSshSshdConfig(char* value, void* log);
int SshUtilsRemediateEnsureSshBestPracticeProtocol(char* value, void* log);
int SshUtilsRemediateEnsureSshBestPracticeIgnoreRhosts(char* value, void* log);
int SshUtilsRemediateEnsureSshLogLevelIsSet(char* value, void* log);
int SshUtilsRemediateEnsureSshMaxAuthTriesIsSet(char* value, void* log);
int SshUtilsRemediateEnsureAllowUsersIsConfigured(char* value, void* log);
int SshUtilsRemediateEnsureDenyUsersIsConfigured(char* value, void* log);
int SshUtilsRemediateEnsureAllowGroupsIsConfigured(char* value, void* log);
int SshUtilsRemediateEnsureDenyGroupsConfigured(char* value, void* log);
int SshUtilsRemediateEnsureSshHostbasedAuthenticationIsDisabled(char* value, void* log);
int SshUtilsRemediateEnsureSshPermitRootLoginIsDisabled(char* value, void* log);
int SshUtilsRemediateEnsureSshPermitEmptyPasswordsIsDisabled(char* value, void* log);
int SshUtilsRemediateEnsureSshClientIntervalCountMaxIsConfigured(char* value, void* log);
int SshUtilsRemediateEnsureSshClientAliveIntervalIsConfigured(char* value, void* log);
int SshUtilsRemediateEnsureSshLoginGraceTimeIsSet(char* value, void* log);
int SshUtilsRemediateEnsureOnlyApprovedMacAlgorithmsAreUsed(char* value, void* log);
int SshUtilsRemediateEnsureSshWarningBannerIsEnabled(char* value, void* log);
int SshUtilsRemediateEnsureUsersCannotSetSshEnvironmentOptions(char* value, void* log);
int SshUtilsRemediateEnsureAppropriateCiphersForSsh(char* value, void* log);

#ifdef __cplusplus
}
#endif

#endif // SSHUTILS_H