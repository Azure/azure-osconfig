// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PNPUTILS_H
#define PNPUTILS_H

#include "AgentCommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void(*IOTHUB_CLIENT_REPORTED_STATE_CALLBACK)(int status_code, void* userContextCallback);

IOTHUB_DEVICE_CLIENT_LL_HANDLE IotHubInitialize(const char* modelId, const char* productInfo, const char* connectionString, bool traceOn,
    const char* x509Certificate, const char* x509PrivateKeyHandle, const HTTP_PROXY_OPTIONS* proxyName, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol);
void IotHubDeInitialize(void);
void IotHubDoWork(void);

// IOTHUB_CLIENT_RESULT includes values such as:
// - IOTHUB_CLIENT_OK
// - IOTHUB_CLIENT_INVALID_ARG
// - IOTHUB_CLIENT_ERROR
// - IOTHUB_CLIENT_INVALID_SIZE
// - IOTHUB_CLIENT_INDEFINITE_TIME
IOTHUB_CLIENT_RESULT UpdatePropertyFromIotHub(const char* componentName, const char* propertyName, const JSON_Value* propertyValue, int version);
IOTHUB_CLIENT_RESULT ReportPropertyToIotHub(const char* componentName, const char* propertyName, size_t* lastPayloadHash);
IOTHUB_CLIENT_RESULT AckPropertyUpdateToIotHub(const char* componentName, const char* propertyName, char* propertyValue, int valueLength, int version, int propertyUpdateResult);

void ProcessDesiredTwinUpdates();

#ifdef __cplusplus
}
#endif

#endif // PNPUTILS_H
