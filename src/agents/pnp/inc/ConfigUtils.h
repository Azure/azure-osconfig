// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef CONFIGUTILS_H
#define CONFIGUTILS_H

#define REPORTED_NAME "Reported"
#define REPORTED_COMPONENT_NAME "ComponentName"
#define REPORTED_SETTING_NAME "ObjectName"
#define MAX_COMPONENT_NAME 256

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct REPORTED_PROPERTY
{
    char componentName[MAX_COMPONENT_NAME];
    char propertyName[MAX_COMPONENT_NAME];
    size_t lastPayloadHash;
} REPORTED_PROPERTY;

bool IsFullLoggingEnabledInJsonConfig(const char* jsonString);
int GetIntegerFromJsonConfig(const char* valueName, const char* jsonString, int defaultValue, int minValue, int maxValue);
int LoadReportedFromJsonConfig(const char* jsonString, REPORTED_PROPERTY* reportedProperties);
char* GetHttpProxyData(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIGUTILS_H
