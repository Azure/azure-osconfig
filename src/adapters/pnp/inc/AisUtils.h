// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef AISUTILS_H
#define AISUTILS_H

#include "AgentCommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(IOT)
char* RequestConnectionStringFromAis(char** x509Certificate, char** x509PrivateKeyHandle);

char* FormatAllocateString(const char* format, ...);
#else // !defined(IOT)
static inline char* RequestConnectionStringFromAis(char**, char**) { return NULL; }
static inline int mallocAndStrcpy_s(char**, const char*) { return 1; }
#endif

#ifdef __cplusplus
}
#endif

#endif // AISUTILS_H
