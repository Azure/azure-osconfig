// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef ASB_H
#define ASB_H

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

void AsbInitialize(void* log);
void AsbShutdown(void* log);

int AsbMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, void* log);
int AsbMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, void* log);

#ifdef __cplusplus
}
#endif

#endif // ASB_H
