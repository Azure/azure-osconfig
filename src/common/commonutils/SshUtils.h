// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMMONUTILS_H
#define COMMONUTILS_H

// Include CommonUtils.h in the target source before including this header

#define OPEN_SSH_SSHD "sshd"
#define OPEN_SSH_ETC_SSH_SSHD_CONFIG "/etc/ssh/sshd_config"
#define OPEN_SSH_SSHD_T "sshd -T"
#define OPEN_SSH_CONFIG_PROTOCOL "Protocol"
#define OPEN_SSH_CONFIG_IGNORE_HOSTS "IgnoreRhosts"
#define OPEN_SSH_CONFIG_LOG_LEVEL "LogLevel"
#define OPEN_SSH_CONFIG_MAX_AUTH_TRIES "MaxAuthTries"
#define OPEN_SSH_CONFIG_ALLOW_USERS "AllowUsers"
#define OPEN_SSH_CONFIG_DENY_USERS "DenyUsers"
#define OPEN_SSH_CONFIG_ALLOW_GROUPS "AllowGroups"
#define OPEN_SSH_CONFIG_DENY_GROUPS "DenyGroups"
#define OPEN_SSH_CONFIG_HOST_BASED_AUTH "HostBasedAuthentication"
#define OPEN_SSH_CONFIG_PERMIT_ROOT_LOGIN "PermitRootLogin"
#define OPEN_SSH_CONFIG_PERMIT_EMPTY_PASSWORDS "PermitEmptyPasswords"
#define OPEN_SSH_CONFIG_CLIENT_ALIVE_COUNT_MAX "ClientAliveCountMax"
#define OPEN_SSH_CONFIG_CLIENT_ALIVE_INTERVAL "ClientAliveInterval"
#define OPEN_SSH_CONFIG_LOGIN_GRACE_TIME "LoginGraceTime"
#define OPEN_SSH_CONFIG_MACS "MACs" 
#define OPEN_SSH_CONFIG_PERMIT_USER_ENVIRONMENT "PermitUserEnvironment"
#define OPEN_SSH_CONFIG_CIPHERS "Ciphers"
#define OPEN_SSH_CONFIG_BANNER "Banner"

#define OPEN_SSH_CONFIG_ROOT "root"
#define OPEN_SSH_CONFIG_ALL_USERS "*@*"
#define OPEN_SSH_CONFIG_ALL_GROUPS "*"
#define OPEN_SSH_CONFIG_PROTOCOL_2 "Protocol 2"
#define OPEN_SSH_CONFIG_DEFAULT_MAC_VALUES "hmac-sha2-256,hmac-sha2-256-etm@openssh.com,hmac-sha2-512,hmac-sha2-512-etm@openssh.com"
#define OPEN_SSH_CONFIG_DEFAULT_CIPHERS "aes128-ctr,aes192-ctr,aes256-ctr"

#define OPEN_SSH_CONFIG_BANNER_TEXT \
"#######################################################################\n\n"\
"Authorized access only!\n\n"\
"If you are not authorized to access or use this system, disconnect now!\n\n"\
"#######################################################################\n"

#define OPEN_SSHD_T_IGNORE_HOSTS "ignorerhosts"
#define OPEN_SSHD_T_LOG_LEVEL "loglevel"
#define OPEN_SSHD_T_INFO "INFO"
#define OPEN_SSHD_T_MAX_AUTH_TRIES "maxauthtries"
#define OPEN_SSHD_T_ALLOW_USERS "allowusers"
#define OPEN_SSHD_T_DENY_USERS "denyusers"
#define OPEN_SSHD_T_ALLOW_GROUPS "allowgroups"
#define OPEN_SSHD_T_DENY_GROUPS "denygroups"
#define OPEN_SSHD_T_HOST_BASED_AUTH "hostbasedauthentication"
#define OPEN_SSHD_T_PERMIT_ROOT_LOGIN "permitrootlogin"
#define OPEN_SSHD_T_PERMIT_EMPTY_PASSWORDS "permitemptypasswords"
#define OPEN_SSHD_T_CLIENT_ALIVE_COUNT_MAX "clientalivecountmax"
#define OPEN_SSHD_T_LOGIN_GRACE_TIME "logingracetime"
#define OPEN_SSHD_T_CLIENT_ALIVE_INTERVAL "clientaliveinterval"
#define OPEN_SSHD_T_MACS "macs"
#define OPEN_SSHD_T_PERMIT_USER_ENVIRONMENT "permituserenvironment"
#define OPEN_SSHD_T_CIPHERS "ciphers"
#define OPEN_SSHD_T_BANNER "banner"

#define OPEN_SSHD_T_YES "yes"
#define OPEN_SSHD_T_NO "no"
#define OPEN_SSHD_T_DEFAULT_BANNER_FILE "/etc/azsec/banner.txt"

#ifdef __cplusplus
extern "C"
{
#endif

int CheckLockoutForFailedPasswordAttempts(const char* fileName, void* log);
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

#endif // COMMONUTILS_H