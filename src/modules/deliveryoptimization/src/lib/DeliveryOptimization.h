// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef DELIVERYOPTIMIZATION_H
#define DELIVERYOPTIMIZATION_H

#ifdef __cplusplus
extern "C"
{
#endif

void DeliveryOptimizationInitialize(const char* configFile);
void DeliveryOptimizationShutdown(void);

MMI_HANDLE DeliveryOptimizationMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes);
void DeliveryOptimizationMmiClose(MMI_HANDLE clientSession);
int DeliveryOptimizationMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int DeliveryOptimizationMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
int DeliveryOptimizationMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
void DeliveryOptimizationMmiFree(MMI_JSON_STRING payload);

#ifdef __cplusplus
}
#endif

#endif // DELIVERYOPTIMIZATION_H
