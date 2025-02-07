// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../Common.h"
#include <ComplianceInterface.hpp>

static MMI_HANDLE gCompliance = NULL;

static const char gComponentName[] = "Compliance";

int BaselineIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, void* log)
{
    (void)resourceId;
    (void)ruleId;
    (void)payloadKey;
    (void)log;
    return 0;
}

void BaselineInitialize(void* log)
{
    (void)log;
    ComplianceInitialize(log);
    gCompliance = ComplianceMmiOpen(gComponentName, -1);
}

void BaselineShutdown(void* log)
{
    (void)log;
    if(NULL == gCompliance)
    {
        return;
    }

    ComplianceMmiClose(gCompliance);
    ComplianceShutdown();
    gCompliance = NULL;
}

int BaselineMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, void* log)
{
    (void)log;
    (void)maxPayloadSizeBytes;

    if (0 != strcmp(componentName, gComponentName))
    {
        return EINVAL;
    }

    if (NULL == gCompliance)
    {
        return EINVAL;
    }

    return ComplianceMmiGet(gCompliance, componentName, objectName, payload, payloadSizeBytes);
}

int BaselineMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, void* log)
{
    (void)log;

    if (0 != strcmp(componentName, gComponentName))
    {
        return EINVAL;
    }

    return ComplianceMmiSet(gCompliance, componentName, objectName, payload, payloadSizeBytes);
}
