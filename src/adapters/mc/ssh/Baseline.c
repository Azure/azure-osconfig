// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../Common.h"
#include <Asb.h>

int BaselineIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, OsConfigLogHandle log)
{
    return AsbIsValidResourceIdRuleId(resourceId, ruleId, payloadKey, log);
}

int BaselineIsCorrectDistribution(const char* payloadKey, OsConfigLogHandle log)
{
    UNUSED(payloadKey);
    UNUSED(log);
    return 0;
}

void BaselineInitialize(OsConfigLogHandle log)
{
    AsbInitialize(log);
}

void BaselineShutdown(OsConfigLogHandle log)
{
    AsbShutdown(log);
}

int BaselineMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OsConfigLogHandle log)
{
    return AsbMmiGet(componentName, objectName, payload, payloadSizeBytes, maxPayloadSizeBytes, log);
}

int BaselineMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log)
{
    return AsbMmiSet(componentName, objectName, payload, payloadSizeBytes, log);
}
