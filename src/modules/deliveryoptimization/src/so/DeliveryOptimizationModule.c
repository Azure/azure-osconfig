// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Mmi.h>
#include <DeliveryOptimization.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DELIVERY_OPTIMZATION_DIRECTORY "/etc/deliveryoptimization-agent/"
#define DELIVERY_OPTIMZATION_CONFIG_FILE DELIVERY_OPTIMZATION_DIRECTORY "admin-config.json"

void __attribute__((constructor)) InitModule(void)
{
    struct stat st = {0};
    if (-1 == stat(DELIVERY_OPTIMZATION_DIRECTORY, &st))
    {
        mkdir(DELIVERY_OPTIMZATION_DIRECTORY, 0700);
    }
    DeliveryOptimizationInitialize(DELIVERY_OPTIMZATION_CONFIG_FILE);
}

void __attribute__((destructor)) DestroyModule(void)
{
    DeliveryOptimizationShutdown();
}

int MmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return DeliveryOptimizationMmiGetInfo(clientName, payload, payloadSizeBytes);
}

MMI_HANDLE MmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    return DeliveryOptimizationMmiOpen(clientName, maxPayloadSizeBytes);
}

void MmiClose(MMI_HANDLE clientSession)
{
    return DeliveryOptimizationMmiClose(clientSession);
}

int MmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    return DeliveryOptimizationMmiSet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

int MmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return DeliveryOptimizationMmiGet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

void MmiFree(MMI_JSON_STRING payload)
{
    return DeliveryOptimizationMmiFree(payload);
}
