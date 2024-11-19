// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#ifdef __cplusplus
extern "C"
{
#endif

void ConfigurationInitialize(const char* fileName);
void ConfigurationShutdown(void);

MMI_HANDLE ConfigurationMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void ConfigurationMmiClose(MMI_HANDLE clientSession);
int ConfigurationMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int ConfigurationMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int ConfigurationMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
void ConfigurationMmiFree(MMI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // CONFIGURATION_H
