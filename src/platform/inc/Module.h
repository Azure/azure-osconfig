// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MODULE_H
#define MODULE_H

#include <Mmi.h>

typedef int (*MMI_GETINFO)(const char*, MMI_JSON_STRING*, int*);
typedef void (*MMI_FREE)(MMI_JSON_STRING);
typedef MMI_HANDLE(*MMI_OPEN)(const char*, const unsigned int);
typedef int (*MMI_SET)(MMI_HANDLE, const char*, const char*, const MMI_JSON_STRING, const int);
typedef int (*MMI_GET)(MMI_HANDLE, const char*, const char*, MMI_JSON_STRING*, int*);
typedef void (*MMI_CLOSE)(MMI_HANDLE);

typedef enum LIFETIME
{
    UNDEFINED = 0,
    KEEPALIVE = 1,
    SHORT = 2
} LIFETIME;

typedef struct VERSION
{
    unsigned int major;
    unsigned int minor;
    unsigned int patch;
    unsigned int tweak;
} VERSION;

typedef struct MODULE_INFO
{
    char* name;
    char* description;
    char* manufacturer;
    VERSION version;
    char* versionInfo;
    char** components;
    unsigned int componentCount;
    LIFETIME lifetime;
    char* licenseUri;
    char* projectUri;
    unsigned int userAccount;
} MODULE_INFO;

typedef struct MODULE
{
    char* name;
    void* handle;
    MODULE_INFO* info;

    MMI_OPEN open;
    MMI_CLOSE close;
    MMI_GETINFO getInfo;
    MMI_SET set;
    MMI_GET get;
    MMI_FREE free;

    struct MODULE* next;
} MODULE;

MODULE* LoadModule(const char* client, const char* path);
void UnloadModule(MODULE* module);

#endif // MODULE_H