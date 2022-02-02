// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PROXYUTILS_H
#define PROXYUTILS_H

#include "AgentCommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

char* GetHttpProxyData();
bool ParseHttpProxyData(char* proxyData, char** hostAddress, int* port, char**username, char** password);

#ifdef __cplusplus
}
#endif

#endif // PROXYUTILS_H
