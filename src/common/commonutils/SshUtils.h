// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef SSHUTILS_H
#define SSHUTILS_H

// Include CommonUtils.h in the target source before including this header

#define DEFAULT_SSH_SSHD_CONFIG_ACCESS "600"
#define DEFAULT_SSH_PROTOCOL "2"
#define DEFAULT_SSH_YES "yes"
#define DEFAULT_SSH_NO "no"
#define DEFAULT_SSH_LOG_LEVEL "INFO"
#define DEFAULT_SSH_MAX_AUTH_TRIES "6"
#define DEFAULT_SSH_ALLOW_USERS "*@*"
#define DEFAULT_SSH_DENY_USERS "root"
#define DEFAULT_SSH_ALLOW_GROUPS "*"
#define DEFAULT_SSH_DENY_GROUPS "root"
#define DEFAULT_SSH_CLIENT_INTERVAL_COUNT_MAX "0"
#define DEFAULT_SSH_CLIENT_ALIVE_INTERVAL "3600"
#define DEFAULT_SSH_LOGIN_GRACE_TIME "60"
#define DEFAULT_SSH_MACS "hmac-sha2-256,hmac-sha2-256-etm@openssh.com,hmac-sha2-512,hmac-sha2-512-etm@openssh.com"
#define DEFAULT_SSH_CIPHERS "aes128-ctr,aes192-ctr,aes256-ctr"
#define DEFAULT_SSH_BANNER_TEXT \
    "#######################################################################\n\n"\
    "Authorized access only!\n\n"\
    "If you are not authorized to access or use this system, disconnect now!\n\n"\
    "#######################################################################\n"

#ifdef __cplusplus
extern "C"
{
#endif

int CheckOnlyApprovedMacAlgorithmsAreUsed(const char** macs, unsigned int numberOfMacs, char** reason, void* log);
int CheckAppropriateCiphersForSsh(const char** ciphers, unsigned int numberOfCiphers, char** reason, void* log);
int CheckSshOptionIsSet(const char* option, const char* expectedValue, char** actualValue, char** reason, void* log);
int CheckSshClientAliveInterval(char** reason, void* log);
int CheckSshLoginGraceTime(char** reason, void* log);
int SetSshOption(const char* option, const char* value, void* log);
int SetSshWarningBanner(unsigned int desiredBannerFileAccess, const char* bannerText, void* log);

#ifdef __cplusplus
}
#endif

#endif // SSHUTILS_H