// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef AISUTILS_H
#define AISUTILS_H

#include "AgentCommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

char* RequestConnectionStringFromAis(char** x509Certificate, char** x509PrivateKeyHandle);

char* FormatAllocateString(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // AISUTILS_H
