// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// IMPORTANT
//
// This module is only used for testing purposes and does not represent a
// proper implementation of a module. For information on how to author a
// module, please see the documentation and our official samples.
//
// Modules Documentation:
// https://github.com/Azure/azure-osconfig/blob/main/docs/modules.md
//
// Module Samples:
// https://github.com/Azure/azure-osconfig/tree/main/src/modules/samples
//

#include <cstdio>
#include <cstring>

#include <CommonUtils.h>
#include <Mmi.h>

class TestsModuleHandle {};

constexpr const char info[] = R"""({
    "Name": "Valid Test Module",
    "Description": "This is a test module (V1)",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["TestModule_Component_1", "TestModule_Component_2"],
    "Lifetime": 2,
    "UserAccount": 0})""";

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    UNUSED(clientName);

    std::size_t len = ARRAY_SIZE(info) - 1;
    *payloadSizeBytes = len;
    *payload = new char[len];
    std::memcpy(*payload, info, len);

    return MMI_OK;
}

MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes)
{
    UNUSED(clientName);
    UNUSED(maxPayloadSizeBytes);

    TestsModuleHandle* session = new TestsModuleHandle();
    MMI_HANDLE handle = reinterpret_cast<MMI_HANDLE>(session);

    return handle;
}

void MmiClose(MMI_HANDLE clientSession)
{
    TestsModuleHandle* handle = reinterpret_cast<TestsModuleHandle*>(clientSession);
    if (nullptr != handle)
    {
        delete handle;
    }
}

int MmiSet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    UNUSED(clientSession);
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return MMI_OK;
}

int MmiGet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    UNUSED(clientSession);
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return MMI_OK;
}

void MmiFree(MMI_JSON_STRING payload)
{
    if (nullptr != payload)
    {
        delete[] payload;
    }
}