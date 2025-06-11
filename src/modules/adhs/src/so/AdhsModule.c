// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Mmi.h>
#include <Adhs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define ADHS_DIRECTORY "/etc/azure-device-health-services/"
#define ADHS_CONFIG_FILE ADHS_DIRECTORY "config.toml"

void __attribute__((constructor)) InitModule(void)
{
    struct stat st = {0};
    if (-1 == stat(ADHS_DIRECTORY, &st))
    {
        mkdir(ADHS_DIRECTORY, 0700);
    }
    AdhsInitialize(ADHS_CONFIG_FILE);
}

void __attribute__((destructor)) DestroyModule(void)
{
    AdhsShutdown();
}

int MmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return AdhsMmiGetInfo(clientName, payload, payloadSizeBytes);
}

MMI_HANDLE MmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    return AdhsMmiOpen(clientName, maxPayloadSizeBytes);
}

void MmiClose(MMI_HANDLE clientSession)
{
    return AdhsMmiClose(clientSession);
}

int MmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    return AdhsMmiSet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

int MmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return AdhsMmiGet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

void MmiFree(MMI_JSON_STRING payload)
{
    return AdhsMmiFree(payload);
}
