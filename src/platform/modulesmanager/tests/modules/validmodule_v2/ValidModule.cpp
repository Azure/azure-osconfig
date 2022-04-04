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
#include <string>

#include <CommonUtils.h>
#include <Mmi.h>

// Object names
static const std::string g_string = "string";
static const std::string g_integer = "integer";
static const std::string g_boolean = "boolean";
static const std::string g_integerEnum = "integerEnum";
static const std::string g_integerArray = "integerArray";
static const std::string g_stringArray = "stringArray";
static const std::string g_integerMap = "integerMap";
static const std::string g_stringMap = "stringMap";
static const std::string g_object = "object";
static const std::string g_objectArray = "objectArray";

// Object payloads
static const char g_stringPayload[] = "\"string\"";
static const char g_integerPayload[] = "123";
static const char g_booleanPayload[] = "true";
static const char g_integerEnumPayload[] = "1";
static const char g_integerArrayPayload[] = "[1, 2, 3]";
static const char g_stringArrayPayload[] = "[\"a\", \"b\", \"c\"]";
static const char g_integerMapPayload[] = "{\"key1\": 1, \"key2\": 2}";
static const char g_stringMapPayload[] = "{\"key1\": \"a\", \"key2\": \"b\"}";
static const char g_objectPayload[] = R"""({
        "string": "value",
        "integer": 1,
        "boolean": true,
        "integerEnum": 1,
        "integerArray": [1, 2, 3],
        "stringArray": ["a", "b", "c"],
        "integerMap": { "key1": 1, "key2": 2 },
        "stringMap": { "key1": "a", "key2": "b" }
    })""";
static const char g_objectArrayPayload[] = R"""([
        {
            "string": "value",
            "integer": 1,
            "boolean": true,
            "integerEnum": 1,
            "integerArray": [1, 2, 3],
            "stringArray": ["a", "b", "c"],
            "integerMap": { "key1": 1, "key2": 2 },
            "stringMap": { "key1": "a", "key2": "b" }
        },
        {
            "string": "value",
            "integer": 1,
            "boolean": true,
            "integerEnum": 1,
            "integerArray": [1, 2, 3],
            "stringArray": ["a", "b", "c"],
            "integerMap": { "key1": 1, "key2": 2 },
            "stringMap": { "key1": "a", "key2": "b" }
        }
    ])""";

static constexpr const char g_info[] = R"""({
    "Name": "Valid Test Module",
    "Description": "This is a test module (V2)",
    "Manufacturer": "Microsoft",
    "VersionMajor": 2,
    "VersionMinor": 0,
    "VersionInfo": "",
    "Components": ["TestModule_Component_1", "TestModule_Component_2"],
    "Lifetime": 2,
    "UserAccount": 0})""";

class TestsModuleHandle {};

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    UNUSED(clientName);

    std::size_t len = ARRAY_SIZE(g_info) - 1;
    *payloadSizeBytes = len;
    *payload = new char[len];
    std::memcpy(*payload, g_info, len);

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

int CopyPayloadString(const char* payloadJson, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    std::size_t len = strlen(payloadJson);
    *payload = new (std::nothrow) char[len];

    if (nullptr != *payload)
    {
        std::memcpy(*payload, payloadJson, len);
        *payloadSizeBytes = len;
    }
    else
    {
        status = ENOMEM;
    }

    return status;
}

int MmiGet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;

    // Ignore the client name and component name
    UNUSED(clientSession);
    UNUSED(componentName);

    *payload = nullptr;
    *payloadSizeBytes = 0;

    if (0 == g_string.compare(objectName))
    {
        status = CopyPayloadString(g_stringPayload, payload, payloadSizeBytes);
    }
    else if (0 == g_integer.compare(objectName))
    {
        status = CopyPayloadString(g_integerPayload, payload, payloadSizeBytes);
    }
    else if (0 == g_boolean.compare(objectName))
    {
        status = CopyPayloadString(g_booleanPayload, payload, payloadSizeBytes);
    }
    else if (0 == g_integerEnum.compare(objectName))
    {
        status = CopyPayloadString(g_integerEnumPayload, payload, payloadSizeBytes);
    }
    else if (0 == g_integerArray.compare(objectName))
    {
        status = CopyPayloadString(g_integerArrayPayload, payload, payloadSizeBytes);
    }
    else if (0 == g_stringArray.compare(objectName))
    {
        status = CopyPayloadString(g_stringArrayPayload, payload, payloadSizeBytes);
    }
    else if (0 == g_integerMap.compare(objectName))
    {
        status = CopyPayloadString(g_integerMapPayload, payload, payloadSizeBytes);
    }
    else if (0 == g_stringMap.compare(objectName))
    {
        status = CopyPayloadString(g_stringMapPayload, payload, payloadSizeBytes);
    }
    else if (0 == g_object.compare(objectName))
    {
        status = CopyPayloadString(g_objectPayload, payload, payloadSizeBytes);
    }
    else if (0 == g_objectArray.compare(objectName))
    {
       status =  CopyPayloadString(g_objectArrayPayload, payload, payloadSizeBytes);
    }
    else
    {
        status = EINVAL;
    }

    return status;
}

void MmiFree(MMI_JSON_STRING payload)
{
    if (nullptr != payload)
    {
        delete[] payload;
    }
}