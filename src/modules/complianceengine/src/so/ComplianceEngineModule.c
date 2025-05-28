// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ComplianceEngineInterface.h"

#include <Mmi.h>
#include <assert.h>
#include <stddef.h>

static OsConfigLogHandle gLog = NULL;
static const char* gLogFile = "/var/log/osconfig_complianceengine.log";
static const char* gRolledLogFile = "/var/log/osconfig_complianceengine.bak";

void __attribute__((constructor)) InitModule(void)
{
    gLog = OpenLog(gLogFile, gRolledLogFile);
    assert(NULL != gLog);
    ComplianceEngineInitialize(gLog);
}

void __attribute__((destructor)) DestroyModule(void)
{
    CloseLog(&gLog);
    ComplianceEngineShutdown();
}

int MmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return ComplianceEngineMmiGetInfo(clientName, payload, payloadSizeBytes);
}

MMI_HANDLE MmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    return ComplianceEngineMmiOpen(clientName, maxPayloadSizeBytes);
}

void MmiClose(MMI_HANDLE clientSession)
{
    return ComplianceEngineMmiClose(clientSession);
}

int MmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    return ComplianceEngineMmiSet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

int MmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    return ComplianceEngineMmiGet(clientSession, componentName, objectName, payload, payloadSizeBytes);
}

void MmiFree(MMI_JSON_STRING payload)
{
    return ComplianceEngineMmiFree(payload);
}
