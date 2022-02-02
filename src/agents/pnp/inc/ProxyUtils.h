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
HTTP_PROXY_OPTIONS* ParseHttpProxyData(char* proxyData);

#ifdef __cplusplus
}
#endif

#endif // PROXYUTILS_H
