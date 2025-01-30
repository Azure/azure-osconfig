// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef ASB_H
#define ASB_H

#define PRETTY_NAME_AZURE_LINUX "CBL-Mariner/Linux"
#define PRODUCT_NAME_AZURE_COMMODORE "Azure Commodore"
#define PRETTY_NAME_ALMA_LINUX_9 "AlmaLinux 9"
#define PRETTY_NAME_AMAZON_LINUX_2 "Amazon Linux 2"
#define PRETTY_NAME_CENTOS_7 "CentOS Linux 7"
#define PRETTY_NAME_CENTOS_8 "CentOS Stream 8"
#define PRETTY_NAME_DEBIAN_10 "Debian GNU/Linux 10"
#define PRETTY_NAME_DEBIAN_11 "Debian GNU/Linux 11"
#define PRETTY_NAME_DEBIAN_12 "Debian GNU/Linux 12"
#define PRETTY_NAME_ORACLE_LINUX_SERVER_7 "Oracle Linux Server 7"
#define PRETTY_NAME_ORACLE_LINUX_SERVER_8 "Oracle Linux Server 8"
#define PRETTY_NAME_RHEL_7 "Red Hat Enterprise Linux Server 7"
#define PRETTY_NAME_RHEL_8 "Red Hat Enterprise Linux 8"
#define PRETTY_NAME_RHEL_9 "Red Hat Enterprise Linux 9"
#define PRETTY_NAME_ROCKY_LINUX_9 "Rocky Linux 9"
#define PRETTY_NAME_SLES_12 "SUSE Linux Enterprise Server 12"
#define PRETTY_NAME_SLES_15 "SUSE Linux Enterprise Server 15"
#define PRETTY_NAME_UBUNTU_16_04 "Ubuntu 16.04"
#define PRETTY_NAME_UBUNTU_18_04 "Ubuntu 18.04"
#define PRETTY_NAME_UBUNTU_20_04 "Ubuntu 20.04"
#define PRETTY_NAME_UBUNTU_22_04 "Ubuntu 22.04"

#define SECURITY_AUDIT_PASS "PASS"
#define SECURITY_AUDIT_FAIL "FAIL"

#define InternalOsConfigAddReason(reason, format, ...) {\
    char* last = NULL;\
    char* temp = FormatAllocateString("%s, also ", *reason);\
    FREE_MEMORY(*reason);\
    last = FormatAllocateString(format, ##__VA_ARGS__);\
    last[0] = tolower(last[0]);\
    *reason = ConcatenateStrings(temp, last);\
    FREE_MEMORY(temp);\
    FREE_MEMORY(last);\
}\

#define OsConfigCaptureReason(reason, format, ...) {\
    if (NULL != reason) {\
        if ((NULL != *reason) && (0 != strncmp(*reason, SECURITY_AUDIT_PASS, strlen(SECURITY_AUDIT_PASS)))) {\
            InternalOsConfigAddReason(reason, format, ##__VA_ARGS__);\
        } else {\
            FREE_MEMORY(*reason);\
            *reason = FormatAllocateString(format, ##__VA_ARGS__);\
        }\
    }\
}\

#define OsConfigCaptureSuccessReason(reason, format, ...) {\
    char* temp = NULL;\
    if (NULL != reason) {\
        if ((NULL != *reason) && (0 == strncmp(*reason, SECURITY_AUDIT_PASS, strlen(SECURITY_AUDIT_PASS)))) {\
            InternalOsConfigAddReason(reason, format, ##__VA_ARGS__);\
        } else {\
            FREE_MEMORY(*reason);\
            temp = FormatAllocateString(format, ##__VA_ARGS__);\
            *reason = ConcatenateStrings(SECURITY_AUDIT_PASS, temp);\
            FREE_MEMORY(temp);\
        }\
    }\
}\

#define OsConfigIsSuccessReason(reason)\
    (((NULL != reason) && ((NULL == *reason) || (0 == strncmp(*reason, SECURITY_AUDIT_PASS, strlen(SECURITY_AUDIT_PASS))))) ? true : false)\

#define OsConfigResetReason(reason) {\
    if (NULL != reason) {\
        FREE_MEMORY(*reason);\
    }\
}\

#ifdef __cplusplus
extern "C"
{
#endif

int AsbIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, void* log);

void AsbInitialize(void* log);
void AsbShutdown(void* log);

int AsbMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, void* log);
int AsbMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, void* log);

#ifdef __cplusplus
}
#endif

#endif // ASB_H
