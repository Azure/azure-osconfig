// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../Common.h"
#include <Asb.h>

int BaselineIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, OSCONFIG_LOG_HANDLE log)
{
    return AsbIsValidResourceIdRuleId(resourceId, ruleId, payloadKey, log);
}

void BaselineInitialize(OSCONFIG_LOG_HANDLE log)
{
    AsbInitialize(log);
}

void BaselineShutdown(OSCONFIG_LOG_HANDLE log)
{
    AsbShutdown(log);
}

int BaselineMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OSCONFIG_LOG_HANDLE log)
{
    return AsbMmiGet(componentName, objectName, payload, payloadSizeBytes, maxPayloadSizeBytes, log);
}

int BaselineMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, OSCONFIG_LOG_HANDLE log)
{
    return AsbMmiSet(componentName, objectName, payload, payloadSizeBytes, log);
}
